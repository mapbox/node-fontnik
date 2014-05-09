#include "tile.hpp"
#include "font_face_set.hpp"
#include "harfbuzz_shaper.hpp"

// node
#include <node_buffer.h>

// stl
#include <algorithm>
#include <memory>
#include <iostream>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
// #include FT_STROKER_H
}

struct ShapeBaton {
    v8::Persistent<v8::Function> callback;
    Tile *tile;
    std::string fontstack;
};

struct RangeBaton {
    v8::Persistent<v8::Function> callback;
    Tile *tile;
    std::string fontstack;
    uint32_t start;
    uint32_t end;
};

v8::Persistent<v8::FunctionTemplate> Tile::constructor;

Tile::Tile(const char *data, size_t length) : node::ObjectWrap() {
    tile.ParseFromArray(data, length);
}

Tile::~Tile() {}

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

v8::Handle<v8::Value> Tile::Shape(const v8::Arguments& args) {
    v8::HandleScope scope;

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsString()) {
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

v8::Handle<v8::Value> Tile::Range(const v8::Arguments& args) {
    v8::HandleScope scope;

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsString()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("First argument must be a font stack")));
    }

    if (args.Length() < 2 || !args[1]->IsNumber()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Second argument must be a number")));
    }

    if (args.Length() < 3 || !args[2]->IsNumber()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Third argument must be a number")));
    }

    if (args.Length() < 4 || !args[3]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Fourth argument must be a callback function")));
    }

    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[1]);
    v8::String::Utf8Value fontstack(args[0]->ToString());
    uint32_t start = args[1]->NumberValue();
    uint32_t end = args[2]->NumberValue();

    Tile *tile = node::ObjectWrap::Unwrap<Tile>(args.This());

    RangeBaton* baton = new RangeBaton();
    baton->callback = v8::Persistent<v8::Function>::New(callback);
    baton->tile = tile;
    baton->fontstack = *fontstack;
    baton->start = start;
    baton->end = end;

    uv_work_t *req = new uv_work_t();
    req->data = baton;

    int status = uv_queue_work(uv_default_loop(), req, AsyncRange, (uv_after_work_cb)RangeAfter);
    assert(status == 0);

    return v8::Undefined();
}

void Tile::AsyncShape(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);
    // Maps char index (UTF-16) to width. If multiple glyphs map to the
    // same char the sum of all widths is used.
    // Note: this probably isn't the best solution. it would be better
    // to have an object for each cluster, but it needs to be
    // implemented with no overhead.
    std::map<unsigned, double> width_map_;
    fontserver::freetype_engine font_engine_;
    fontserver::face_manager_freetype font_manager(font_engine_);
    fontserver::text_itemizer itemizer;

    fontserver::font_set fset(baton->fontstack);
    fset.add_fontstack(baton->fontstack, ',');

    fontserver::face_set_ptr face_set = font_manager.get_face_set(fset);
    if (!face_set->size()) return;

    std::map<fontserver::face_ptr, fontserver::tile_face *> face_map;
    std::vector<fontserver::tile_face *> tile_faces;

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

        fontserver::text_format format(baton->fontstack, 24);
        fontserver::text_format_ptr format_ptr = 
            std::make_shared<fontserver::text_format>(format);

        // Process strings per layer.
        for (auto const& key : strings) {
            const llmr::vector::value& value = layer.values(key);
            std::string text;
            if (value.has_string_value()) {
                text = value.string_value();
            }

            if (!text.empty()) {
                // Clear cluster widths.
                width_map_.clear();

                UnicodeString const& str = text.data();

                fontserver::text_line line(0, str.length() - 1);

                itemizer.add_text(str, format_ptr);

                const double scale_factor = 1.0;

                // Shape the text.
                fontserver::harfbuzz_shaper shaper;
                shaper.shape_text(line,
                                  itemizer,
                                  width_map_,
                                  face_set,
                                  // font_manager,
                                  scale_factor);

                llmr::vector::label *label = mutable_layer->add_labels();
                label->set_text(key);

                // TODO: support multiple font stacks
                label->set_stack(0); 

                // Add all glyphs for this labels and add new font
                // faces as they appear.
                for (auto const& glyph : line) {
                    if (!glyph.face) {
                        std::cout << text << ' ' << 
                            line.size() << " glyphs\n" << 
                            " codepoint: " << glyph.glyph_index <<
                            " char_index: " << glyph.char_index <<
                            " face_ptr: " << glyph.face <<
                            '\n';
                        continue;
                    }

                    // Try to find whether this font has already been
                    // used in this tile.
                    std::map<fontserver::face_ptr, fontserver::tile_face *>::iterator face_map_itr = face_map.find(glyph.face);
                    if (face_map_itr == face_map.end()) {
                        fontserver::tile_face *face = 
                            new fontserver::tile_face(glyph.face);
                        std::pair<fontserver::face_ptr, fontserver::tile_face *> keyed(glyph.face, face);
                        face_map_itr = face_map.insert(keyed).first;

                        // Add to shared face cache if not found.
                        fontserver::font_face_set::iterator face_itr = std::find(face_set->begin(), face_set->end(), glyph.face);
                        if (face_itr == face_set->end()) {
                            face_set->add(glyph.face);
                        }
                    }

                    fontserver::tile_face *face = face_map_itr->second;

                    // Find out whether this font has been used in 
                    // this tile before and get its position.
                    std::vector<fontserver::tile_face *>::iterator tile_itr = std::find(tile_faces.begin(), tile_faces.end(), face);
                    if (tile_itr == tile_faces.end()) {
                        tile_faces.push_back(face);
                        tile_itr = tile_faces.end() - 1;
                    }

                    int tile_face_id = tile_itr - tile_faces.begin();

                    // Add glyph to tile_face.
                    face->add_glyph(glyph);

                    label->add_faces(tile_face_id);
                    label->add_glyphs(glyph.glyph_index);
                    label->add_x(width_map_[glyph.char_index]);
                    label->add_y(glyph.offset.y);
                }

                itemizer.clear();
            }
        }

        // Add a textual representation of the font so that we can figure
        // out later what font we need to use.
        for (auto const& face : tile_faces) {
            std::string name = face->family + " " + face->style;
            mutable_layer->add_faces(name);
            // We don't delete the TileFace objects here because
            // they are 'owned' by the global faces map and deleted
            // later on.
        }

        // Insert FAKE stacks
        mutable_layer->add_stacks(baton->fontstack);
    }

    InsertGlyphs(tile, tile_faces);
}

void Tile::AsyncRange(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    fontserver::freetype_engine font_engine_;
    fontserver::face_manager_freetype font_manager(font_engine_);

    fontserver::font_set fset(baton->fontstack);
    fset.add_fontstack(baton->fontstack, ',');

    fontserver::face_set_ptr face_set = font_manager.get_face_set(fset);
    if (!face_set->size()) return;

    std::map<fontserver::face_ptr, fontserver::tile_face *> face_map;
    std::vector<fontserver::tile_face *> tile_faces;

    llmr::vector::tile& tile = baton->tile->tile;

    fontserver::text_format format(baton->fontstack, 24);
    const double scale_factor = 1.0;

    // Set character sizes.
    double size = format.text_size * scale_factor;
    face_set->set_character_sizes(size);

    FT_UInt glyph_index = 0;
    FT_UInt glyph_end = 128;

    for (auto const& face : *face_set) {
        // Create tile_face, add to face_map and tile_faces.
        fontserver::tile_face *t_face = new fontserver::tile_face(face);
        face_map.emplace(face, t_face);
        tile_faces.push_back(t_face);

        // Get FreeType face from face_ptr.
        FT_Face ft_face = face->get_face();

        // Add all glyphs for this labels and add new font
        // faces as they appear.
        FT_ULong char_code = FT_Get_First_Char(ft_face, &glyph_index);
        while (glyph_index != 0 && char_code < glyph_end) {
            fontserver::glyph_info glyph;
            glyph.glyph_index = char_code;

            face->glyph_dimensions(glyph);

            // Add glyph to t_face.
            t_face->add_glyph(glyph);

            // Advance char_code to next glyph index.
            char_code = FT_Get_Next_Char(ft_face, char_code, &glyph_index);
        }
    }

    InsertGlyphs(tile, tile_faces);
}

// Insert SDF glyphs + bitmaps
void Tile::InsertGlyphs(llmr::vector::tile &tile, 
                        std::vector<fontserver::tile_face *> &tile_faces) {
    for (auto const& face : tile_faces) {
        llmr::vector::face *mutable_face = tile.add_faces();
        mutable_face->set_family(face->family);
        mutable_face->set_style(face->style);

        for (auto const& glyph : face->glyphs) {
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

void Tile::RangeAfter(uv_work_t* req) {
    v8::HandleScope scope;
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

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
