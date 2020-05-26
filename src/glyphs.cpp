// fontnik
#include "glyphs.hpp"
#include <protozero/pbf_reader.hpp>
#include <protozero/pbf_writer.hpp>
// node
#include <limits>
#include <memory>
#include <nan.h>
#include <node_buffer.h>

// sdf-glyph-foundry
#include <gzip/compress.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <mapbox/glyph_foundry.hpp>
#include <mapbox/glyph_foundry_impl.hpp>
#include <utility>


namespace node_fontnik {

struct FaceMetadata {
    // non copyable
    FaceMetadata(FaceMetadata const&) = delete;
    FaceMetadata& operator=(FaceMetadata const&) = delete;
    // movaable only for highest efficiency
    FaceMetadata& operator=(FaceMetadata&& c) = default;
    FaceMetadata(FaceMetadata&& c) = default;
    std::string family_name{};
    std::string style_name{};
    std::vector<int> points{};
    FaceMetadata(std::string  _family_name,
                 std::string  _style_name,
                 std::vector<int>&& _points) : family_name(std::move(_family_name)),
                                               style_name(std::move(_style_name)),
                                               points(std::move(_points)) {}
    FaceMetadata(std::string  _family_name,
                 std::vector<int>&& _points) : family_name(std::move(_family_name)),
                                               points(std::move(_points)) {}
};

struct LoadBaton {
    // We explicitly delete the copy constructor and assignment operator below
    // This allows us to have the `const char* font_data` without needing to define copy semantics
    // and avoids a g++ warning of ‘struct node_fontnik::LoadBaton’ has pointer data members [-Weffc++]
    // but does not override ‘node_fontnik::LoadBaton(const node_fontnik::LoadBaton&)’'
    LoadBaton(LoadBaton const&) = delete;
    LoadBaton& operator=(LoadBaton const&) = delete;

    Nan::Persistent<v8::Function> callback;
    Nan::Persistent<v8::Object> buffer;
    const char* font_data;
    std::size_t font_size;
    std::string error_name;
    std::vector<FaceMetadata> faces;
    uv_work_t request;
    LoadBaton(v8::Local<v8::Object> buf,
              v8::Local<v8::Value> cb) : font_data(node::Buffer::Data(buf)),
                                         font_size(node::Buffer::Length(buf)),
                                         
                                         request() {
        request.data = this;
        callback.Reset(cb.As<v8::Function>());
        buffer.Reset(buf.As<v8::Object>());
    }
    ~LoadBaton() {
        callback.Reset();
        buffer.Reset();
    }
};

struct RangeBaton {
    // We explicitly delete the copy constructor and assignment operator below
    // This allows us to have the `const char* font_data` without needing to define copy semantics
    // and avoids a g++ warning of ‘struct node_fontnik::LoadBaton’ has pointer data members [-Weffc++]
    // but does not override ‘node_fontnik::LoadBaton(const node_fontnik::LoadBaton&)’'
    RangeBaton(RangeBaton const&) = delete;
    RangeBaton& operator=(RangeBaton const&) = delete;
    Nan::Persistent<v8::Function> callback;
    Nan::Persistent<v8::Object> buffer;
    const char* font_data;
    std::size_t font_size;
    std::string error_name;
    std::uint32_t start;
    std::uint32_t end;
    std::vector<std::uint32_t> chars;
    std::string message;
    uv_work_t request;
    RangeBaton(v8::Local<v8::Object> buf,
               v8::Local<v8::Value> cb,
               std::uint32_t _start,
               std::uint32_t _end) : font_data(node::Buffer::Data(buf)),
                                     font_size(node::Buffer::Length(buf)),
                                     
                                     start(_start),
                                     end(_end),
                                     
                                     request() {
        request.data = this;
        callback.Reset(cb.As<v8::Function>());
        buffer.Reset(buf.As<v8::Object>());
    }
    ~RangeBaton() {
        callback.Reset();
        buffer.Reset();
    }
};

struct GlyphPBF {
    explicit GlyphPBF(v8::Local<v8::Object>& buffer)
        : data{node::Buffer::Data(buffer), node::Buffer::Length(buffer)} {
            buffer_ref.Reset(buffer.As<v8::Object>());
        }

    ~GlyphPBF() {
        buffer_ref.Reset();
    }

    // non-copyable
    GlyphPBF(GlyphPBF const&) = delete;
    GlyphPBF& operator=(GlyphPBF const&) = delete;

    // non-movable
    GlyphPBF(GlyphPBF&&) = delete;
    GlyphPBF& operator=(GlyphPBF&&) = delete;

    protozero::data_view data;
    Nan::Persistent<v8::Object> buffer_ref;
};

struct CompositeBaton {
    CompositeBaton(CompositeBaton const&) = delete;
    CompositeBaton& operator=(CompositeBaton const&) = delete;

    Nan::Persistent<v8::Function> callback;
    std::vector<std::unique_ptr<GlyphPBF>> glyphs{};
    std::string error_name;
    std::unique_ptr<std::string> message;
    uv_work_t request;
    CompositeBaton(unsigned size, v8::Local<v8::Value> cb) : 
                                                             message(std::make_unique<std::string>()),
                                                             request() {
        glyphs.reserve(size);
        request.data = this;
        callback.Reset(cb.As<v8::Function>());
    }
    ~CompositeBaton() {
        callback.Reset();
    }
};

NAN_METHOD(Load) {
    // Validate arguments.
    if (!info[0]->IsObject()) {
        return Nan::ThrowTypeError("First argument must be a font buffer");
    }
    v8::Local<v8::Object> obj = info[0]->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return Nan::ThrowTypeError("First argument must be a font buffer");
    }

    if (info.Length() < 2 || !info[1]->IsFunction()) {
        return Nan::ThrowTypeError("Callback must be a function");
    }

    auto* baton = new LoadBaton(obj, info[1]);
    uv_queue_work(uv_default_loop(), &baton->request, LoadAsync, reinterpret_cast<uv_after_work_cb>(AfterLoad));
}

NAN_METHOD(Range) {
    // Validate arguments.
    if (info.Length() < 1 || !info[0]->IsObject()) {
        return Nan::ThrowTypeError("First argument must be an object of options");
    }

    v8::Local<v8::Object> options = info[0].As<v8::Object>();
    v8::Local<v8::Value> font_buffer = Nan::Get(options, Nan::New<v8::String>("font").ToLocalChecked()).ToLocalChecked();
    if (!font_buffer->IsObject()) {
        return Nan::ThrowTypeError("Font buffer is not an object");
    }
    v8::Local<v8::Object> obj = font_buffer->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
    v8::Local<v8::Value> start = Nan::Get(options, Nan::New<v8::String>("start").ToLocalChecked()).ToLocalChecked();
    v8::Local<v8::Value> end = Nan::Get(options, Nan::New<v8::String>("end").ToLocalChecked()).ToLocalChecked();

    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return Nan::ThrowTypeError("First argument must be a font buffer");
    }

    if (!start->IsNumber() || Nan::To<std::int32_t>(start).FromJust() < 0) {
        return Nan::ThrowTypeError("option `start` must be a number from 0-65535");
    }

    if (!end->IsNumber() || Nan::To<std::int32_t>(end).FromJust() > 65535) {
        return Nan::ThrowTypeError("option `end` must be a number from 0-65535");
    }

    if (Nan::To<std::int32_t>(end).FromJust() < Nan::To<std::int32_t>(start).FromJust()) {
        return Nan::ThrowTypeError("`start` must be less than or equal to `end`");
    }

    if (info.Length() < 2 || !info[1]->IsFunction()) {
        return Nan::ThrowTypeError("Callback must be a function");
    }

    auto* baton = new RangeBaton(obj,
                                       info[1],
                                       Nan::To<std::uint32_t>(start).FromJust(),
                                       Nan::To<std::uint32_t>(end).FromJust());
    uv_queue_work(uv_default_loop(), &baton->request, RangeAsync, reinterpret_cast<uv_after_work_cb>(AfterRange));
}

namespace utils {

inline void CallbackError(const std::string& message, v8::Local<v8::Function> func) {
    Nan::Callback cb(func);
    v8::Local<v8::Value> argv[1] = {Nan::Error(message.c_str())};
    Nan::Call(cb, 1, argv);
}

} // namespace utils

NAN_METHOD(Composite) {
    // validate callback function
    v8::Local<v8::Value> callback_val = info[info.Length() - 1];
    if (!callback_val->IsFunction()) {
        Nan::ThrowError("last argument must be a callback function");
        return;
    }

    v8::Local<v8::Function> callback = callback_val.As<v8::Function>();

    // validate glyphPBF array
    if (!info[0]->IsArray()) {
        return utils::CallbackError("first arg 'glyphs' must be an array of glyphs objects", callback);
    }

    v8::Local<v8::Array> glyphs = info[0].As<v8::Array>();
    unsigned num_glyphs = glyphs->Length();

    if (num_glyphs <= 0) {
        return utils::CallbackError("'glyphs' array must be of length greater than 0", callback);
    }

    auto* baton = new CompositeBaton(num_glyphs, callback);

    for (unsigned t = 0; t < num_glyphs; ++t) {
        v8::Local<v8::Value> buf_val = Nan::Get(glyphs, t).ToLocalChecked();
        if (buf_val->IsNull() || buf_val->IsUndefined()) {
            return utils::CallbackError("buffer value in 'glyphs' array item is null or undefined", callback);
        }
        v8::MaybeLocal<v8::Object> maybe_buffer = buf_val->ToObject(Nan::GetCurrentContext()).ToLocalChecked();
        if (maybe_buffer.IsEmpty()) {
            return utils::CallbackError("buffer value in 'glyphs' array is empty", callback);
        }
        v8::Local<v8::Object> buffer = maybe_buffer.ToLocalChecked();

        if (!node::Buffer::HasInstance(buffer)) {
            return utils::CallbackError("buffer value in 'glyphs' array item is not a true buffer", callback);
        }
        baton->glyphs.push_back(std::make_unique<GlyphPBF>(buffer));
    }
    uv_queue_work(uv_default_loop(), &baton->request, CompositeAsync, reinterpret_cast<uv_after_work_cb>(AfterComposite));
}

using id_pair = std::pair<std::uint32_t, protozero::data_view>;
struct CompareID {
    bool operator()(id_pair const& r1, id_pair const& r2) {
        return (r1.first - r2.first) != 0U;
    }
};

void CompositeAsync(uv_work_t* req) {
    auto* baton = static_cast<CompositeBaton*>(req->data);
    try {
        std::vector<std::unique_ptr<std::vector<char>>> buffer_cache;
        std::map<std::uint32_t, protozero::data_view> id_mapping;
        bool first_buffer = true;
        std::string fontstack_name;
        std::string range;
        std::string& fontstack_buffer = *baton->message;
        protozero::pbf_writer pbf_writer(fontstack_buffer);
        protozero::pbf_writer fontstack_writer{pbf_writer, 1};
        // TODO(danespringmeyer): avoid duplicate fontstacks to be sent it
        for (auto const& glyph_obj : baton->glyphs) {
            protozero::data_view data_view{};
            if (gzip::is_compressed(glyph_obj->data.data(), glyph_obj->data.size())) {
                buffer_cache.push_back(std::make_unique<std::vector<char>>());
                gzip::Decompressor decompressor;
                decompressor.decompress(*buffer_cache.back(), glyph_obj->data.data(), glyph_obj->data.size());
                data_view = protozero::data_view{buffer_cache.back()->data(), buffer_cache.back()->size()};
            } else {
                data_view = glyph_obj->data;
            }
            protozero::pbf_reader fontstack_reader(data_view);
            while (fontstack_reader.next(1)) {
                auto stack_reader = fontstack_reader.get_message();
                while (stack_reader.next()) {
                    switch (stack_reader.tag()) {
                    case 1: // name
                    {
                        if (first_buffer) {
                            fontstack_name = stack_reader.get_string();
                        } else {
                            fontstack_name = fontstack_name + ", " + stack_reader.get_string();
                        }
                        break;
                    }
                    case 2: // range
                    {
                        if (first_buffer) {
                            range = stack_reader.get_string();
                        } else {
                            stack_reader.skip();
                        }
                        break;
                    }
                    case 3: // glyphs
                    {
                        auto glyphs_data = stack_reader.get_view();
                        // collect all ids from first
                        if (first_buffer) {
                            protozero::pbf_reader glyphs_reader(glyphs_data);
                            std::uint32_t glyph_id;
                            while (glyphs_reader.next(1)) {
                                glyph_id = glyphs_reader.get_uint32();
                            }
                            id_mapping.emplace(glyph_id, glyphs_data);
                        } else {
                            protozero::pbf_reader glyphs_reader(glyphs_data);
                            std::uint32_t glyph_id;
                            while (glyphs_reader.next(1)) {
                                glyph_id = glyphs_reader.get_uint32();
                            }
                            auto search = id_mapping.find(glyph_id);
                            if (search == id_mapping.end()) {
                                id_mapping.emplace(glyph_id, glyphs_data);
                            }
                        }
                        break;
                    }
                    default:
                        // ignore data for unknown tags to allow for future extensions
                        stack_reader.skip();
                    }
                }
            }
            first_buffer = false;
        }
        fontstack_writer.add_string(1, fontstack_name);
        fontstack_writer.add_string(2, range);
        for (auto const& glyph_pair : id_mapping) {
            fontstack_writer.add_message(3, glyph_pair.second);
        }
    } catch (std::exception const& ex) {
        baton->error_name = ex.what();
    }
}

void AfterComposite(uv_work_t* req) {
    Nan::HandleScope scope;

    auto* baton = static_cast<CompositeBaton*>(req->data);
    Nan::AsyncResource async_resource(__func__);
    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = {Nan::Error(baton->error_name.c_str())};
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    } else {
        std::string& fontstack_message = *baton->message;
        const auto argc = 2U;
        v8::Local<v8::Value> argv[argc] = {
            Nan::Null(),
            Nan::NewBuffer(
                &fontstack_message[0],
                static_cast<unsigned int>(fontstack_message.size()),
                [](char* /*unused*/, void* hint) {
                    delete reinterpret_cast<std::string*>(hint);
                },
                baton->message.release())
                .ToLocalChecked()};
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }

    delete baton;
}

struct ft_library_guard {
    // non copyable
    ft_library_guard(ft_library_guard const&) = delete;
    ft_library_guard& operator=(ft_library_guard const&) = delete;

    explicit ft_library_guard(FT_Library* lib) : library_(lib) {}

    ~ft_library_guard() {
        if (library_ != nullptr) { FT_Done_FreeType(*library_);

}
    }

    FT_Library* library_;
};

struct ft_face_guard {
    // non copyable
    ft_face_guard(ft_face_guard const&) = delete;
    ft_face_guard& operator=(ft_face_guard const&) = delete;
    explicit ft_face_guard(FT_Face* f) : face_(f) {}

    ~ft_face_guard() {
        if (face_ != nullptr) {
            FT_Done_Face(*face_);
        }
    }

    FT_Face* face_;
};

void LoadAsync(uv_work_t* req) {
    auto* baton = static_cast<LoadBaton*>(req->data);
    try {
        FT_Library library = nullptr;
        ft_library_guard library_guard(&library);
        FT_Error error = FT_Init_FreeType(&library);
        if (error != 0) {
            /* LCOV_EXCL_START */
            baton->error_name = std::string("could not open FreeType library");
            return;
            /* LCOV_EXCL_END */
        }
        FT_Face ft_face = nullptr;
        FT_Long num_faces = 0;
        for (int i = 0; ft_face == nullptr || i < num_faces; ++i) {
            ft_face_guard face_guard(&ft_face);
            FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
            if (face_error != 0) {
                baton->error_name = std::string("could not open font file");
                return;
            }
            if (num_faces == 0) {
                num_faces = ft_face->num_faces;
            }

            if (ft_face->family_name != nullptr) {
                std::set<int> points;
                FT_ULong charcode;
                FT_UInt gindex;
                charcode = FT_Get_First_Char(ft_face, &gindex);
                while (gindex != 0) {
                    charcode = FT_Get_Next_Char(ft_face, charcode, &gindex);
                    if (charcode != 0) { points.emplace(charcode);

}
                }

                std::vector<int> points_vec(points.begin(), points.end());

                if (ft_face->style_name != nullptr) {
                    baton->faces.emplace_back(ft_face->family_name, ft_face->style_name, std::move(points_vec));
                } else {
                    baton->faces.emplace_back(ft_face->family_name, std::move(points_vec));
                }
            } else {
                baton->error_name = std::string("font does not have family_name or style_name");
                return;
            }
        }
    } catch (std::exception const& ex) {
        baton->error_name = ex.what();
    }
}

void AfterLoad(uv_work_t* req) {
    Nan::HandleScope scope;

    auto* baton = static_cast<LoadBaton*>(req->data);
    Nan::AsyncResource async_resource(__func__);
    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = {Nan::Error(baton->error_name.c_str())};
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Array> js_faces = Nan::New<v8::Array>(baton->faces.size());
        unsigned idx = 0;
        for (auto const& face : baton->faces) {
            v8::Local<v8::Object> js_face = Nan::New<v8::Object>();
            Nan::Set(js_face, Nan::New("family_name").ToLocalChecked(), Nan::New(face.family_name).ToLocalChecked());
            if (!face.style_name.empty()) {
                Nan::Set(js_face, Nan::New("style_name").ToLocalChecked(), Nan::New(face.style_name).ToLocalChecked());
            }
            v8::Local<v8::Array> js_points = Nan::New<v8::Array>(face.points.size());
            unsigned p_idx = 0;
            for (auto const& pt : face.points) {
                Nan::Set(js_points, p_idx++, Nan::New(pt));
            }
            Nan::Set(js_face, Nan::New("points").ToLocalChecked(), js_points);
            Nan::Set(js_faces, idx++, js_face);
        }
        v8::Local<v8::Value> argv[2] = {Nan::Null(), js_faces};
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }
    delete baton;
}

void RangeAsync(uv_work_t* req) {
    auto* baton = static_cast<RangeBaton*>(req->data);
    try {

        unsigned array_size = baton->end - baton->start;
        baton->chars.reserve(array_size);
        for (unsigned i = baton->start; i <= baton->end; i++) {
            baton->chars.emplace_back(i);
        }

        FT_Library library = nullptr;
        ft_library_guard library_guard(&library);
        FT_Error error = FT_Init_FreeType(&library);
        if (error != 0) {
            /* LCOV_EXCL_START */
            baton->error_name = std::string("could not open FreeType library");
            return;
            /* LCOV_EXCL_END */
        }

        protozero::pbf_writer pbf_writer{baton->message};
        FT_Face ft_face = nullptr;
        FT_Long num_faces = 0;
        for (int i = 0; ft_face == nullptr || i < num_faces; ++i) {
            ft_face_guard face_guard(&ft_face);
            FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
            if (face_error != 0) {
                baton->error_name = std::string("could not open font");
                return;
            }

            if (num_faces == 0) {
                num_faces = ft_face->num_faces;
            }

            if (ft_face->family_name != nullptr) {
                protozero::pbf_writer fontstack_writer{pbf_writer, 1};
                if (ft_face->style_name != nullptr) {
                    fontstack_writer.add_string(1, std::string(ft_face->family_name) + " " + std::string(ft_face->style_name));
                } else {
                    fontstack_writer.add_string(1, std::string(ft_face->family_name));
                }
                fontstack_writer.add_string(2, std::to_string(baton->start) + "-" + std::to_string(baton->end));

                const double scale_factor = 1.0;

                // Set character sizes.
                double size = 24 * scale_factor;
                FT_Set_Char_Size(ft_face, 0, static_cast<FT_F26Dot6>(size * (1 << 6)), 0, 0);

                for (std::vector<uint32_t>::size_type x = 0; x != baton->chars.size(); x++) {
                    FT_ULong char_code = baton->chars[x];
                    sdf_glyph_foundry::glyph_info glyph;

                    // Get FreeType face from face_ptr.
                    FT_UInt char_index = FT_Get_Char_Index(ft_face, char_code);

                    if (char_index == 0U) { continue;

}

                    glyph.glyph_index = char_index;
                    sdf_glyph_foundry::RenderSDF(glyph, 24, 3, 0.25, ft_face);

                    // Add glyph to fontstack.
                    protozero::pbf_writer glyph_writer{fontstack_writer, 3};

                    // shortening conversion
                    if (char_code > std::numeric_limits<FT_ULong>::max()) {
                        throw std::runtime_error("Invalid value for char_code: too large");
                    } 
                        glyph_writer.add_uint32(1, static_cast<std::uint32_t>(char_code));
                    

                    if (glyph.width > 0) {
                        glyph_writer.add_bytes(2, glyph.bitmap);
                    }

                    // direct type conversions, no need for checking or casting
                    glyph_writer.add_uint32(3, glyph.width);
                    glyph_writer.add_uint32(4, glyph.height);
                    glyph_writer.add_sint32(5, glyph.left);

                    // conversions requiring checks, for safety and correctness

                    // double to int
                    double top = static_cast<double>(glyph.top) - glyph.ascender;
                    if (top < std::numeric_limits<std::int32_t>::min() || top > std::numeric_limits<std::int32_t>::max()) {
                        throw std::runtime_error("Invalid value for glyph.top-glyph.ascender");
                    } 
                        glyph_writer.add_sint32(6, static_cast<std::int32_t>(top));
                    

                    // double to uint
                    if (glyph.advance < std::numeric_limits<std::uint32_t>::min() || glyph.advance > std::numeric_limits<std::uint32_t>::max()) {
                        throw std::runtime_error("Invalid value for glyph.top-glyph.ascender");
                    } 
                        glyph_writer.add_uint32(7, static_cast<std::uint32_t>(glyph.advance));
                    
                }
            } else {
                baton->error_name = std::string("font does not have family_name");
                return;
            }
        }
    } catch (std::exception const& ex) {
        baton->error_name = ex.what();
    }
}

void AfterRange(uv_work_t* req) {
    Nan::HandleScope scope;

    auto* baton = static_cast<RangeBaton*>(req->data);
    Nan::AsyncResource async_resource(__func__);
    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = {Nan::Error(baton->error_name.c_str())};
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = {Nan::Null(), Nan::CopyBuffer(baton->message.data(), static_cast<std::uint32_t>(baton->message.size())).ToLocalChecked()};
        async_resource.runInAsyncScope(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }

    delete baton;
}

} // namespace node_fontnik
