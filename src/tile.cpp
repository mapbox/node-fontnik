#include <node.h>
#include <node_buffer.h>

#include "tile.hpp"
#include "globals.hpp"
#include <pango/pangoft2.h>
#include "sdf_renderer.hpp"

#include "distmap.h"
#include <set>
#include <algorithm>

using namespace v8;

struct ShapeBaton {
    Persistent<Function> callback;
    Tile *tile;
    std::string fontstack;
};

Persistent<FunctionTemplate> Tile::constructor;

void Tile::Init(Handle<Object> target) {
    HandleScope scope;

    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    Local<String> name = String::NewSymbol("Tile");

    constructor = Persistent<FunctionTemplate>::New(tpl);

    // ObjectWrap uses the first internal field to store the wrapped pointer.
    constructor->InstanceTemplate()->SetInternalFieldCount(1);
    constructor->SetClassName(name);

    // Add all prototype methods, getters and setters here.
    constructor->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("length"), Length);
    NODE_SET_PROTOTYPE_METHOD(constructor, "serialize", Serialize);
    NODE_SET_PROTOTYPE_METHOD(constructor, "shape", Shape);
    // constructor->PrototypeTemplate()->SetIndexedPropertyHandler(GetGlyph);

    // This has to be last, otherwise the properties won't show up on the
    // object in JavaScript.
    target->Set(name, constructor->GetFunction());
}

Tile::Tile(const char *data, size_t length)
    : ObjectWrap() {
    tile.ParseFromArray(data, length);
    pthread_mutex_init(&mutex, NULL);
}

Tile::~Tile() {
    pthread_mutex_destroy(&mutex);
}

Handle<Value> Tile::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(Exception::TypeError(String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() < 1 || !node::Buffer::HasInstance(args[0])) {
        return ThrowException(Exception::TypeError(String::New("First argument must be a buffer")));
    }

    Local<Object> buffer = args[0]->ToObject();

    Tile* tile = new Tile(node::Buffer::Data(buffer), node::Buffer::Length(buffer));
    tile->Wrap(args.This());

    return args.This();
}

bool Tile::HasInstance(Handle<Value> val) {
    if (!val->IsObject()) return false;
    return constructor->HasInstance(val->ToObject());
}

Handle<Value> Tile::Length(Local<String> property, const AccessorInfo &info) {
    HandleScope scope;
    Tile* tile = ObjectWrap::Unwrap<Tile>(info.This());
    Local<Number> length = Uint32::New(tile->tile.layers_size());
    return scope.Close(length);
}

Handle<Value> Tile::Serialize(const v8::Arguments& args) {
    HandleScope scope;
    llmr::vector::tile& tile = ObjectWrap::Unwrap<Tile>(args.This())->tile;
    std::string serialized = tile.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}

Handle<Value> Tile::Shape(const v8::Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(Exception::TypeError(
            String::New("First argument must be a font stack")));
    }

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return ThrowException(Exception::TypeError(
            String::New("Second argument must be a callback function")));
    }
    Local<Function> callback = Local<Function>::Cast(args[1]);
    String::Utf8Value fontstack(args[0]->ToString());

    Tile *tile = ObjectWrap::Unwrap<Tile>(args.This());

    ShapeBaton* baton = new ShapeBaton();
    baton->callback = Persistent<Function>::New(callback);
    baton->tile = tile;
    baton->fontstack = *fontstack;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncShape, (uv_after_work_cb)ShapeAfter);
    assert(status == 0);

    return Undefined();
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
    Face(PangoFont *font) : font(font) {
        g_object_ref(font);
    }
    ~Face() {
        g_object_unref(font);
    }

    const Glyph& glyph(uint32_t glyph_id) {
        const std::map<uint32_t, Glyph>::iterator pos = glyphs.find(glyph_id);
        if (pos != glyphs.end()) {
            return pos->second;
        } else {
            Glyph& glyph = glyphs[glyph_id];
            glyph.id = glyph_id;

            PangoFcFont *fc_font = PANGO_FC_FONT(font);
            FT_Face ft_face = pango_fc_font_lock_face(fc_font);

            int size = 24;
            int buffer = 3;
            FT_Set_Char_Size(ft_face, 0, size * 64, 72, 72);

            FT_Error error = FT_Load_Glyph(ft_face, glyph_id, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
            if (error) {
                fprintf(stderr, "glyph load error %d\n", error);
                return glyph;
            }

            glyph.width = ft_face->glyph->bitmap.width;
            glyph.height = ft_face->glyph->bitmap.rows;
            glyph.left = ft_face->glyph->bitmap_left;
            glyph.top = ft_face->glyph->bitmap_top;
            glyph.advance = ft_face->glyph->metrics.horiAdvance / 64;

            FT_GlyphSlot slot = ft_face->glyph;
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

            pango_fc_font_unlock_face(fc_font);

            return glyph;
        }
    }

private:
    PangoFont *font;
    std::map<uint32_t, Glyph> glyphs;

    typedef std::map<PangoFont *, Face *> map;
    static pthread_once_t init;
    static pthread_key_t map_key;
    static void init_map() {
        pthread_key_create(&map_key, delete_map);
    }
    static void delete_map(void *fontmap) {
        delete (map *)fontmap;
    }

public:
    static Face *face(PangoFont *font)
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
    TileFace(PangoFont *font) : font(font) {
        g_object_ref(font);
        PangoFcFont *fc_font = PANGO_FC_FONT(font);
        FT_Face ft_face = pango_fc_font_lock_face(fc_font);
        family = ft_face->family_name;
        style = ft_face->style_name;
        pango_fc_font_unlock_face(fc_font);
    }
    ~TileFace() {
        g_object_unref(font);
    }
    inline void add_glyph(uint32_t glyph_id) {
        glyphs.insert(glyph_id);
    }

    PangoFont *font;
    std::string family;
    std::string style;
    std::set<uint32_t> glyphs;
};

void Tile::AsyncShape(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    pthread_mutex_lock(&baton->tile->mutex);

    PangoRenderer *renderer = pango_renderer();

    PangoFontDescription *desc = pango_font_description_from_string(baton->fontstack.c_str());
    pango_font_description_set_absolute_size(desc, 24 * 1024);

    PangoLayout *layout = pango_layout_new(pango_context());
    pango_layout_set_font_description(layout, desc);


    typedef std::map<PangoFont *, TileFace *> Faces;
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

                    // Try to find whether this font has already been used
                    // in this tile.

                    Faces::const_iterator global_pos = faces.find(glyph.font);
                    if (global_pos == faces.end()) {
                        TileFace *face = new TileFace(glyph.font);
                        std::pair<PangoFont *, TileFace *> keyed(glyph.font, face);
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
        const std::pair<PangoFont *, TileFace *>& pair = *it;
        TileFace *face = pair.second;
        llmr::vector::face *mutable_face = tile.add_faces();
        PangoFcFont *fc_font = PANGO_FC_FONT(face->font);
        FT_Face ft_face = pango_fc_font_lock_face(fc_font);
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

        pango_fc_font_unlock_face(fc_font);

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
    HandleScope scope;
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    const unsigned argc = 1;
    Local<Value> argv[argc] = { Local<Value>::New(Null()) };

    TryCatch try_catch;
    baton->callback->Call(Context::GetCurrent()->Global(), argc, argv);
    if (try_catch.HasCaught()) {
        node::FatalException(try_catch);
    }

    baton->callback.Dispose();

    delete baton;
    delete req;
}
