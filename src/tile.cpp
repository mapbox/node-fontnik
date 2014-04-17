#include "tile.hpp"
#include "font_face_set.hpp"
#include "harfbuzz_shaper.hpp"

// node
#include <node_buffer.h>

#include <set>
#include <algorithm>
#include <memory>

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

    if (args.Length() < 1) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("First argument must be a font stack")));
    }

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return ThrowException(v8::Exception::TypeError(
            v8::String::New("Second argument must be a callback function")));
    }
    v8::Local<v8::Function> callback = v8::Local<v8::Function>::Cast(args[1]);
    // TODO: validate this is a string
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

    fontserver::face_set_ptr face_set = font_manager.get_face_set(baton->fontstack);

    std::cout << baton->fontstack << ' ' << face_set->size() << '\n';

    if (!face_set->size()) return;

    typedef std::map<uint32_t, fontserver::glyph_info> Glyphs;
    Glyphs glyphs;

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
        fontserver::face_set_ptr layer_faces = font_manager.get_face_set();

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

                fontserver::text_format format(baton->fontstack, 24);
                fontserver::text_format_ptr format_ptr = 
                    std::make_shared<fontserver::text_format>(format);

                itemizer.add_text(str, format_ptr);

                const double scale_factor = 1.0;

                // Shape the text.
                fontserver::harfbuzz_shaper shaper;
                shaper.shape_text(line,
                                  itemizer,
                                  glyphs,
                                  width_map_,
                                  font_manager,
                                  scale_factor);

                llmr::vector::label *label = mutable_layer->add_labels();
                label->set_text(key);

                // TODO: support multiple font stacks
                label->set_stack(0); 

                // Add all glyphs for this labels and add new font
                // faces as they appear.
                for (auto const& glyph_pos : glyphs) {
                    fontserver::glyph_info const& glyph = glyph_pos.second;

                    fontserver::face_ptr const& face = std::make_shared<fontserver::font_face>(*glyph.face);

                    // Try to find whether this font has already been
                    // used in this tile.
                    fontserver::font_face_set::iterator face_itr = std::find(face_set->begin(), face_set->end(), glyph.face);
                    if (face_itr == face_set->end()) {
                        face_set->add(face);
                    }

                    // Find out whether this font has been used in 
                    // this tile before and get its position.
                    fontserver::font_face_set::iterator layer_itr = std::find(layer_faces->begin(), layer_faces->end(), glyph.face);
                    if (layer_itr == layer_faces->end()) {
                        layer_faces->add(face);
                        layer_itr = layer_faces->end() - 1;
                    }

                    int layer_face_id = layer_itr - layer_faces->begin();

                    /*
                    std::cout << " face: " << layer_face_id <<
                        " glyph: " << glyph.glyph_index <<
                        " x: " << width_map_[glyph.char_index] <<
                        " y: " << glyph.offset.y <<
                        '\n';
                    */

                    label->add_faces(layer_face_id);
                    label->add_glyphs(glyph.glyph_index);
                    label->add_x(width_map_[glyph.char_index]);
                    label->add_y(glyph.offset.y);
                }

                itemizer.clear();
                glyphs.clear();

                std::cout << "faces: " << label->faces_size() <<
                    " glyphs: " << label->glyphs_size() <<
                    " x: " << label->x_size() <<
                    " y: " << label->y_size() <<
                    " labels: " << mutable_layer->labels_size() <<
                    '\n';
            }
        }

        // Add a textual representation of the font so that we can figure out
        // later what font we need to use.
        for (auto const& face : *layer_faces) {
            std::string name = face->family_name() + " " + face->style_name();
            mutable_layer->add_faces(name);
            // We don't delete the TileFace objects here because
            // they are 'owned' by the global faces map and deleted
            // later on.
        }

        // Insert FAKE stacks
        mutable_layer->add_stacks(baton->fontstack);

        std::cout << " layer_faces: " << mutable_layer->faces_size() << '\n';
    }

    // Insert SDF glyphs + bitmaps
    for (auto const& face : *face_set) {
        llmr::vector::face *mutable_face = tile.add_faces();
        mutable_face->set_family(face->family_name());
        mutable_face->set_style(face->style_name());

        // Determine ASCII glyphs
        // std::set<uint32_t> omit;
        // FT_UInt glyph_index;
        // FT_ULong char_code = FT_Get_First_Char(ft_face, &glyph_index);
        // while (glyph_index != 0 && char_code < 256) {
        //     omit.insert(glyph_index);
        //     char_code = FT_Get_Next_Char(ft_face, char_code, &glyph_index);
        // }

        for (auto const& glyph : *face) {
            // Omit ASCII glyphs we determined earlier
            // if (omit.find(id) != omit.end()) {
            //     continue;
            // }

            /*
            std::cout << " Glyph: " << glyph.second.glyph_index <<
                " width: " << glyph.second.width <<
                " height: " << glyph.second.height() <<
                " left: " << glyph.second.left <<
                " top: " << glyph.second.top <<
                " advance: " << glyph.second.advance <<
                '\n';
            */
            
            llmr::vector::glyph *mutable_glyph = mutable_face->add_glyphs();
            mutable_glyph->set_id(glyph.second.glyph_index);
            mutable_glyph->set_width(glyph.second.width);
            mutable_glyph->set_height(glyph.second.height());
            mutable_glyph->set_left(glyph.second.left);
            mutable_glyph->set_top(glyph.second.top);
            mutable_glyph->set_advance(glyph.second.advance);
            if (glyph.second.width > 0) {
                mutable_glyph->set_bitmap(glyph.second.bitmap);
            }
        }

        std::cout << "face_glyphs: " << mutable_face->glyphs_size() << '\n';
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
