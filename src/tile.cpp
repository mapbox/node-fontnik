#include <node.h>
#include <node_buffer.h>

#include "globals.hpp"
#include "tile.hpp"
#include "clipper.hpp"
#include "tile_face.hpp"
#include "font_face_set.hpp"
#include "harfbuzz_shaper.hpp"

#include "distmap.h"
#include <set>
#include <algorithm>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
// #include FT_STROKER_H
}

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

// Init pthread fontmap keys for font_face_set.
pthread_once_t font_face_set::init = PTHREAD_ONCE_INIT;
pthread_key_t font_face_set::map_key = 0;

Tile::Tile(const char *data, size_t length) : node::ObjectWrap(),
    font_engine_(),
    font_manager(font_engine_) { 
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

    int status = uv_queue_work(uv_default_loop(), req, AsyncShapeWrapper, (uv_after_work_cb)ShapeAfter);
    assert(status == 0);

    return v8::Undefined();
}

void Tile::AsyncShapeWrapper(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);
    baton->tile->AsyncShape(req);
}

void Tile::AsyncShape(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    pthread_mutex_lock(&baton->tile->mutex);

    typedef std::map<std::string, llmr::vector::face *> Faces;
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
        typedef std::vector<llmr::vector::face *> LayerFaces;
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
                // Clear cluster widths.
                width_map_.clear();

                const double scale_factor = 1.0;
                std::vector<glyph_info> glyphs;

                // Shape the text.
                HarfbuzzShaper shaper;
                glyphs = shaper.Shape(text,
                                      baton->fontstack,
                                      width_map_,
                                      font_manager,
                                      scale_factor);

                llmr::vector::label *label = mutable_layer->add_labels();
                label->set_text(key);
                label->set_stack(0); // TODO: support multiple font stacks

                // Add all glyphs for this labels and add new font faces as they
                // appear.
                for (size_t j = 0; j < glyphs.size(); j++) {
                    glyph_info glyph = glyphs[j];
                    // std::cout<<glyph->format<<'\n';

                    TileFace tile_face = glyph.face;
                    std::string name = tile_face.name();

                    // Try to find whether this font has already been
                    // used in this tile.
                    Faces::const_iterator global_pos = faces.find(name);
                    if (global_pos == faces.end()) {
                        llmr::vector::face *mutable_face = tile.add_faces();
                        mutable_face->set_family(tile_face.family);
                        mutable_face->set_style(tile_face.style);

                        // Add a textual representation of the font so that we can figure out
                        // later what font we need to use.
                        mutable_layer->add_faces(name);

                        std::pair<std::string, llmr::vector::face *> keyed(name, mutable_face);
                        global_pos = faces.insert(keyed).first;
                    }

                    llmr::vector::face *mutable_face = global_pos->second;

                    // Find out whether this font has been used in this tile
                    // before; and get its position ID.s
                    std::vector<llmr::vector::face *>::iterator pos = std::find(layer_faces.begin(), layer_faces.end(), mutable_face);
                    if (pos == layer_faces.end()) {
                        // Do not ref this font object here since we
                        // already ref'ed it for the global font list.
                        layer_faces.push_back(mutable_face);
                        pos = layer_faces.end() - 1;
                    }
                    int layer_face_id = pos - layer_faces.begin();
                    std::cout<<name<<' '<<layer_face_id<<'\n';

                    label->add_faces(layer_face_id);
                    label->add_glyphs(glyph.glyph_index);
                    label->add_x(glyph.x);
                    label->add_y(glyph.offset.y);

                    // Insert SDF glyphs + bitmaps
                    llmr::vector::glyph *mutable_glyph = mutable_face->add_glyphs();
                    mutable_glyph->set_id(glyph.glyph_index);
                    mutable_glyph->set_width(glyph.width);
                    mutable_glyph->set_height(glyph.height);
                    mutable_glyph->set_left(glyph.left);
                    mutable_glyph->set_top(glyph.top);
                    mutable_glyph->set_advance(glyph.advance);
                    if (glyph.width > 0) {
                        mutable_glyph->set_bitmap(glyph.bitmap);
                    }
                }
            }
        }

        mutable_layer->add_stacks(baton->fontstack);
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
