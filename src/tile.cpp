#include <node.h>
#include <node_buffer.h>

#include "tile.hpp"
#include "clipper.hpp"
#include "font_set.hpp"
#include "font_engine_freetype.hpp"
#include "harfbuzz_shaper.hpp"
// #include "globals.hpp"

#include "distmap.h"
#include <set>
#include <algorithm>
#include <iostream>

using namespace ClipperLib;

struct SimplifyBaton {
    v8::Persistent<v8::Function> callback;
    Tile *tile;
};

struct ShapeBaton {
    v8::Persistent<v8::Function> callback;
    Tile *tile;
    std::string fontstack;
};

v8::Persistent<v8::FunctionTemplate> Tile::constructor;

Tile::Tile(const char *data, size_t length)
    : node::ObjectWrap() { 
    tile.ParseFromArray(data, length);
    pthread_mutex_init(&mutex, NULL);
}

Tile::~Tile() {
    pthread_mutex_destroy(&mutex);
}

void Tile::Init(v8::Handle<v8::Object> target) {
    v8::HandleScope scope;

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    v8::Local<v8::String> name = v8::String::NewSymbol("Tile");

    constructor = v8::Persistent<v8::FunctionTemplate>::New(tpl);

    // node::ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    constructor->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("length"), Length);
    NODE_SET_PROTOTYPE_METHOD(constructor, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor, "simplify", Simplify);
    NODE_SET_PROTOTYPE_METHOD(constructor, "shape", Shape);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

v8::Handle<v8::Value> Tile::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() < 1 || !node::Buffer::HasInstance(args[0])) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("First argument must be a buffer")));
    }

    v8::Local<v8::Object> buffer = args[0]->ToObject();

    Tile* tile = new Tile(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    tile->Wrap(args.This());

    return args.This();
}

bool Tile::HasInstance(v8::Handle<v8::Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

v8::Handle<v8::Value> Tile::Length(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    v8::HandleScope scope;
    Tile* tile = node::ObjectWrap::Unwrap<Tile>(info.This());
    v8::Local<v8::Number> length = v8::Uint32::New(tile->tile.layers_size());
    return scope.Close(length);
}

v8::Handle<v8::Value> Tile::Serialize(const v8::Arguments& args) {
    v8::HandleScope scope;
    llmr::vector::tile& tile = node::ObjectWrap::Unwrap<Tile>(args.This())->tile;
    std::string serialized = tile.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}


template <typename T>
Polygons load_geometry(const T& vertices)
{
    Polygons polygons;
    Polygon polygon;

    int cmd = -1;
    const int cmd_bits = 3;
    unsigned length = 0;

    int x = 0, y = 0;
    for (int k = 0; k < vertices.size();) {
        if (!length) {
            unsigned cmd_length = vertices.Get(k++);
            cmd = cmd_length & ((1 << cmd_bits) - 1);
            length = cmd_length >> cmd_bits;
        }

        if (length > 0) {
            length--;

            if (cmd == 1 || cmd == 7) { // moveto or closepolygon
                if (polygon.size()) {
                    polygons.push_back(polygon);
                    polygon.clear();
                }
            }

            if (cmd == 1 || cmd == 2) {
                int32_t dx = vertices.Get(k++);
                int32_t dy = vertices.Get(k++);
                dx = ((dx >> 1) ^ (-(dx & 1)));
                dy = ((dy >> 1) ^ (-(dy & 1)));
                polygon.push_back(IntPoint(x += dx, y += dy));
            } else if (cmd == 7) {

            } else {
                fprintf(stderr, "Unknown command type: %d\n", cmd);
            }
        }
    }

    if (polygon.size()) {
        polygons.push_back(polygon);
    }

    return polygons;
}


void encode_point(llmr::vector::feature& feature, const IntPoint& pt, const IntPoint& pos) {
    // Compute delta to the previous coordinate.
    int32_t dx = pt.X - pos.X;
    int32_t dy = pt.Y - pos.Y;

    // Manual zigzag encoding.
    feature.add_geometry((dx << 1) ^ (dx >> 31));
    feature.add_geometry((dy << 1) ^ (dy >> 31));
}

void encode_command(llmr::vector::feature& feature, int cmd, unsigned length) {
    const int cmd_bits = 3;
    const int cmd_mask = (1 << cmd_bits) - 1;
    feature.add_geometry((length << cmd_bits) | (cmd & cmd_mask));
}

uint32_t encode_geometry(llmr::vector::feature& feature, const Polygons& polygons, bool is_polygon = false)
{
    IntPoint pos(0, 0);
    uint32_t count = 0;
    for (Polygons::const_iterator it = polygons.begin(); it != polygons.end(); it++) {
        const Polygon& polygon = *it;
        if (!polygon.size()) {
            continue;
        }

        // Encode moveTo first point in the polygon.
        encode_command(feature, 1, 1);
        encode_point(feature, polygon[0], pos);
        count++;
        pos = polygon[0];

        // Encode lineTo for the rest of the points.
        encode_command(feature, 2, polygon.size() - 1);

        for (size_t i = 1; i < polygon.size(); i++) {
            encode_point(feature, polygon[i], pos);
            count++;
            pos = polygon[i];
        }

        if (is_polygon) {
            // Encode closepolygon
            encode_command(feature, 7, 1);
        }
    }

    return count;
}


v8::Handle<v8::Value> Tile::Simplify(const v8::Arguments& args) {
    v8::HandleScope scope;

    if (args.Length() < 1 || !args[0]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("First argument must be a callback function")));
    }
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[0]);

    Tile *tile = node::ObjectWrap::Unwrap<Tile>(args.This());

    SimplifyBaton* baton = new SimplifyBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->tile = tile;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncSimplify, (uv_after_work_cb)SimplifyAfter);
    assert(status == 0);

    return v8::Undefined();
}

void Tile::AsyncSimplify(uv_work_t* req) {
    SimplifyBaton* baton = static_cast<SimplifyBaton*>(req->data);

    pthread_mutex_lock(&baton->tile->mutex);

    llmr::vector::tile& tile = baton->tile->tile;
    llmr::vector::tile new_tile;

    for (int i = 0; i < tile.layers_size(); i++) {
        const llmr::vector::layer& layer = tile.layers(i);
        if (!layer.features_size()) {
            continue;
        }

        llmr::vector::layer *new_layer = new_tile.add_layers();
        new_layer->set_version(2);

        // Copy over information
        new_layer->set_name(layer.name());
        *new_layer->mutable_keys() = layer.keys();
        *new_layer->mutable_values() = layer.values();
        if (layer.has_extent()) {
            new_layer->set_extent(layer.extent());
        }

        // Our water polygons contain lots of duplicates
        if (layer.name() == "water") {
            Clipper outerClipper;
            outerClipper.ForceSimple(true);

            for (int j = 0; j < layer.features_size(); j++) {
                const llmr::vector::feature& feature = layer.features(j);
                Polygons polygons = load_geometry(feature.geometry());

                Clipper clipper;
                clipper.AddPolygons(polygons, ptSubject);
                clipper.Execute(ctUnion, polygons);
                outerClipper.AddPolygons(polygons, ptSubject);
            }


            Polygons polygons;
            outerClipper.Execute(ctUnion, polygons, pftNonZero, pftNonZero);

            if (polygons.size()) {
                llmr::vector::feature *new_feature = new_layer->add_features();
                new_feature->set_type(llmr::vector::Polygon);
                // encode polygons into feature
                uint32_t count = encode_geometry(*new_feature, polygons, true);
                new_feature->set_vertex_count(count);
            }
        } else {
            for (int j = 0; j < layer.features_size(); j++) {
                const llmr::vector::feature& feature = layer.features(j);

                // Copy point features verbatim.
                if (feature.type() == 1) {
                    llmr::vector::feature *new_feature = new_layer->add_features();
                    *new_feature = feature;
                    continue;
                }

                Polygons polygons = load_geometry(feature.geometry());
                bool is_polygon = feature.type() == llmr::vector::Polygon;

                if (is_polygon) {
                    // CleanPolygons(polygons, polygons, 0.001);

                    Clipper clipper;
                    clipper.ForceSimple(true);
                    clipper.AddPolygons(polygons, ptSubject);
                    clipper.Execute(ctUnion, polygons);

                    // SimplifyPolygons(polygons);
                }

                if (polygons.size()) {
                    llmr::vector::feature *new_feature = new_layer->add_features();

                    // Copy over information
                    if (feature.has_id()) new_feature->set_id(feature.id());
                    if (feature.has_type()) new_feature->set_type(feature.type());
                    *new_feature->mutable_tags() = feature.tags();

                    // encode polygons into feature
                    encode_geometry(*new_feature, polygons, is_polygon);
                }
            }
        }
    }

    tile = new_tile;

    pthread_mutex_unlock(&baton->tile->mutex);
}

void Tile::SimplifyAfter(uv_work_t* req) {
    v8::HandleScope scope;
    SimplifyBaton* baton = static_cast<SimplifyBaton*>(req->data);

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { v8::Local<v8::Value>::New(v8::Null()) };

    v8::TryCatch try_catch;
    baton->callback->Call(v8::Context::GetCurrent()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
    delete req;
}



v8::Handle<v8::Value> Tile::Shape(const v8::Arguments& args) {
    v8::HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("First argument must be a font stack")));
    }

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Second argument must be a callback function")));
    }
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[1]);
    v8::String::Utf8Value fontstack(args[0]->ToString());

    Tile *tile = node::ObjectWrap::Unwrap<Tile>(args.This());

    ShapeBaton* baton = new ShapeBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->tile = tile;
    baton->fontstack = *fontstack;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncShape, (uv_after_work_cb)ShapeAfter);
    assert(status == 0);

    return v8::Undefined();
}

struct Glyph {
    uint32_t id;
    std::string bitmap;

    uint32_t width;
    uint32_t height;
    int32_t left;
    int32_t top;
    uint32_t advance;
};

class Face {
public:
    Face(FT_Face font) : font(font) {
        FT_Reference_Face(font);
    }
    ~Face() {
        FT_Done_Face(font);
    }

    const Glyph& glyph(uint32_t glyph_id) {
        const std::map<uint32_t, Glyph>::iterator pos = glyphs.find(glyph_id);
        if (pos != glyphs.end()) {
            return pos->second;
        } else {
            Glyph& glyph = glyphs[glyph_id];
            glyph.id = glyph_id;

            int size = 24;
            int buffer = 3;
            FT_Set_Char_Size(font, 0, size * 64, 72, 72);

            FT_Error error = FT_Load_Glyph(font, glyph_id, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
            if (error) {
                fprintf(stderr, "glyph load error %d\n", error);
                return glyph;
            }

            glyph.width = font->glyph->bitmap.width;
            glyph.height = font->glyph->bitmap.rows;
            glyph.left = font->glyph->bitmap_left;
            glyph.top = font->glyph->bitmap_top;
            glyph.advance = font->glyph->metrics.horiAdvance / 64;

            FT_GlyphSlot slot = font->glyph;
            int width = slot->bitmap.width;
            int height = slot->bitmap.rows;

            // Create a signed distance field for the glyph bitmap.
            if (width > 0) {
                unsigned int buffered_width = width + 2 * buffer;
                unsigned int buffered_height = height + 2 * buffer;

                unsigned char *distance = make_distance_map((unsigned char *)slot->bitmap.buffer, width, height, buffer);

                glyph.bitmap.resize(buffered_width * buffered_height);
                for (unsigned int y = 0; y < buffered_height; y++) {
                    memcpy((unsigned char *)glyph.bitmap.data() + buffered_width * y, distance + y * distmap_size, buffered_width);
                }
                free(distance);
            }

            return glyph;
        }
    }

private:
    FT_Face font;
    std::map<uint32_t, Glyph> glyphs;

    typedef std::map<FT_Face, Face *> map;
    static pthread_once_t init;
    static pthread_key_t map_key;
    static void init_map() {
        pthread_key_create(&map_key, delete_map);
    }
    static void delete_map(void *fontmap) {
        delete (map *)fontmap;
    }

public:
    static Face *face(FT_Face font)
    {
        // Get a thread-specific font-/glyphmap.
        pthread_once(&init, init_map);
        map *fontmap = (map *)pthread_getspecific(map_key);
        if (fontmap == NULL) {
            pthread_setspecific(map_key, fontmap = new map);
        }

        map::const_iterator pos = fontmap->find(font);
        if (pos != fontmap->end()) {
            return pos->second;
        } else {
            Face *face = new Face(font);
            (*fontmap)[font] = face;
            return face;
        }
    }

};

pthread_once_t Face::init = PTHREAD_ONCE_INIT;
pthread_key_t Face::map_key = 0;


class TileFace {
public:
    TileFace(FT_Face font) : font(font) {
        FT_Reference_Face(font);
        family = font->family_name;
        style = font->style_name;
    }
    ~TileFace() {
        FT_Done_Face(font);
    }
    inline void add_glyph(uint32_t glyph_id) {
        glyphs.insert(glyph_id);
    }

    FT_Face font;
    std::string family;
    std::string style;
    std::set<uint32_t> glyphs;
};

void Tile::AsyncShape(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    pthread_mutex_lock(&baton->tile->mutex);

    /*
    PangoRenderer *renderer = pango_renderer();

    PangoFontDescription *desc = pango_font_description_from_string(baton->fontstack.c_str());
    pango_font_description_set_absolute_size(desc, 24 * 1024);

    PangoLayout *layout = pango_layout_new(pango_context());
    pango_layout_set_font_description(layout, desc);
    */


    typedef std::map<FT_Face, TileFace *> Faces;
    Faces faces;

    llmr::vector::tile& tile = baton->tile->tile;

    // for every label
    for (int i = 0; i < tile.layers_size(); i++) {
        const llmr::vector::layer& layer = tile.layers(i);

        typedef std::set<int> Strings;
        Strings strings;

        // Compile a set of all strings we need to shape.
        for (int j = 0; j < layer.features_size(); j++) {
            const llmr::vector::feature& feature = layer.features(j);

            for (int k = 1; k < feature.tags_size(); k += 2) {
                const std::string& key = layer.keys(feature.tags(k - 1));
                if (key == "name") {
                    // TODO: handle multiple fonts stacks
                    strings.insert(feature.tags(k));
                }
                // TODO: extract all keys we need to shape
            }
        }

        llmr::vector::layer* mutable_layer = tile.mutable_layers(i);
        typedef std::vector<TileFace *> LayerFaces;
        LayerFaces layer_faces;

        // Process strings per layer.
        for (Strings::const_iterator it = strings.begin(); it != strings.end(); it++) {
            int key = *it;
            const llmr::vector::value& value = layer.values(key);
            std::string text;
            if (value.has_string_value()) {
                text = value.string_value();
            }

            if (text.size()) {
                HarfbuzzShaper shaper;
                shaper.Shape(text,
                             baton->fontstack);

                /*
                // Shape the text.
                pango_layout_set_text(layout, text.data(), text.size());

                pango_sdf_renderer_reset(PANGO_SDF_RENDERER(renderer));
                pango_renderer_draw_layout (renderer, layout, 0, 0);
                const PangoSDFGlyphs& glyphs = pango_sdf_renderer_get_glyphs(PANGO_SDF_RENDERER(renderer));

                llmr::vector::label *label = mutable_layer->add_labels();
                label->set_text(key);
                label->set_stack(0); // TODO: support multiple font stacks

                // Add all glyphs for this labels and add new font faces as they
                // appear.
                for (size_t j = 0; j < glyphs.size(); j++) {
                    const PangoSDFGlyph& glyph = glyphs[j];
                    std::cout<<pango_font_description_to_string(pango_font_describe(glyph.font))<<'\n';
                    std::cout<<glyph.x<<'\n';
                    std::cout<<glyph.y<<'\n'<<'\n';

                    // Try to find whether this font has already been used
                    // in this tile.

                    PangoFcFont *fc_font = PANGO_FC_FONT(glyph.font);
                    FT_Face ft_face = pango_fc_font_lock_face(fc_font);
                    pango_fc_font_unlock_face(fc_font);

                    Faces::const_iterator global_pos = faces.find(ft_face);
                    if (global_pos == faces.end()) {
                        TileFace *face = new TileFace(ft_face);
                        std::pair<FT_Face, TileFace *> keyed(ft_face, face);
                        global_pos = faces.insert(keyed).first;
                    }

                    TileFace *face = global_pos->second;

                    // Find out whether this font has been used in this tile
                    // before; and get its position ID.s
                    std::vector<TileFace *>::iterator pos = std::find(layer_faces.begin(), layer_faces.end(), face);
                    if (pos == layer_faces.end()) {
                        // Do not ref this font object here since we already ref'ed
                        // it for the global font list.
                        layer_faces.push_back(face);
                        pos = layer_faces.end() - 1;
                    }
                    int layer_face_id = pos - layer_faces.begin();

                    face->add_glyph(glyph.glyph);

                    label->add_faces(layer_face_id);
                    label->add_glyphs(glyph.glyph);
                    label->add_x(glyph.x);
                    label->add_y(glyph.y);
                }
            */
            }
        }

        // Add a textual representation of the font so that we can figure out
        // later what font we need to use.
        for (LayerFaces::const_iterator it = layer_faces.begin(); it != layer_faces.end(); it++) {
            TileFace *face = *it;
            mutable_layer->add_faces(face->family + " " + face->style);
            // note: we don't delete the TileFace objects here because they
            // are 'owned' by the global faces map and deleted later on.
        }

        // Insert FAKE stacks
        mutable_layer->add_stacks(baton->fontstack);
    }

    // Insert SDF glyphs + bitmaps
    for (Faces::const_iterator it = faces.begin(); it != faces.end(); it++) {
        const std::pair<FT_Face, TileFace *>& pair = *it;
        TileFace *face = pair.second;
        llmr::vector::face *mutable_face = tile.add_faces();
        FT_Face ft_face = face->font;
        mutable_face->set_family(ft_face->family_name);
        mutable_face->set_style(ft_face->style_name);

        // Determine ASCII glyphs
        // std::set<uint32_t> omit;
        // FT_UInt glyph_index;
        // FT_ULong char_code = FT_Get_First_Char(ft_face, &glyph_index);
        // while (glyph_index != 0 && char_code < 256) {
        //     omit.insert(glyph_index);
        //     char_code = FT_Get_Next_Char(ft_face, char_code, &glyph_index);
        // }

        Face *_face = Face::face(face->font);

        for (std::set<uint32_t>::const_iterator it = face->glyphs.begin(); it != face->glyphs.end(); it++) {
            uint32_t id = *it;

            // Omit ASCII glyphs we determined earlier
            // if (omit.find(id) != omit.end()) {
            //     continue;
            // }

            const Glyph& gl = _face->glyph(id);

            llmr::vector::glyph *glyph = mutable_face->add_glyphs();
            glyph->set_id(id);
            glyph->set_width(gl.width);
            glyph->set_height(gl.height);
            glyph->set_left(gl.left);
            glyph->set_top(gl.top);
            glyph->set_advance(gl.advance);
            if (gl.width > 0) {
                glyph->set_bitmap(gl.bitmap);
            }
        }

        delete face;
    }


    pthread_mutex_unlock(&baton->tile->mutex);
}

void Tile::ShapeAfter(uv_work_t* req) {
    v8::HandleScope scope;
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { v8::Local<v8::Value>::New(v8::Null()) };

    v8::TryCatch try_catch;
    baton->callback->Call(v8::Context::GetCurrent()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
    delete req;
}
