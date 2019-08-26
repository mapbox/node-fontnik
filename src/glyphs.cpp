// fontnik
#include "glyphs.hpp"
#include <protozero/pbf_writer.hpp>
// node
#include <limits>
#include <nan.h>
#include <node_buffer.h>

// sdf-glyph-foundry
#include <mapbox/glyph_foundry.hpp>
#include <mapbox/glyph_foundry_impl.hpp>

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
    FaceMetadata(std::string const& _family_name,
                 std::string const& _style_name,
                 std::vector<int>&& _points) : family_name(_family_name),
                                               style_name(_style_name),
                                               points(std::move(_points)) {}
    FaceMetadata(std::string const& _family_name,
                 std::vector<int>&& _points) : family_name(_family_name),
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
                                         error_name(),
                                         faces(),
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
                                     error_name(),
                                     start(_start),
                                     end(_end),
                                     chars(),
                                     message(),
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

NAN_METHOD(Load) {
    // Validate arguments.
    if (!info[0]->IsObject()) {
        return Nan::ThrowTypeError("First argument must be a font buffer");
    }
    v8::Local<v8::Object> obj = info[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return Nan::ThrowTypeError("First argument must be a font buffer");
    }

    if (info.Length() < 2 || !info[1]->IsFunction()) {
        return Nan::ThrowTypeError("Callback must be a function");
    }

    LoadBaton* baton = new LoadBaton(obj, info[1]);
    uv_queue_work(uv_default_loop(), &baton->request, LoadAsync, reinterpret_cast<uv_after_work_cb>(AfterLoad));
}

NAN_METHOD(Range) {
    // Validate arguments.
    if (info.Length() < 1 || !info[0]->IsObject()) {
        return Nan::ThrowTypeError("First argument must be an object of options");
    }

    v8::Local<v8::Object> options = info[0].As<v8::Object>();
    v8::Local<v8::Value> font_buffer = options->Get(Nan::New<v8::String>("font").ToLocalChecked());
    if (!font_buffer->IsObject()) {
        return Nan::ThrowTypeError("Font buffer is not an object");
    }
    v8::Local<v8::Object> obj = font_buffer->ToObject();
    v8::Local<v8::Value> start = options->Get(Nan::New<v8::String>("start").ToLocalChecked());
    v8::Local<v8::Value> end = options->Get(Nan::New<v8::String>("end").ToLocalChecked());

    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return Nan::ThrowTypeError("First argument must be a font buffer");
    }

    if (!start->IsNumber() || start->IntegerValue() < 0) {
        return Nan::ThrowTypeError("option `start` must be a number from 0-65535");
    }

    if (!end->IsNumber() || end->IntegerValue() > 65535) {
        return Nan::ThrowTypeError("option `end` must be a number from 0-65535");
    }

    if (end->IntegerValue() < start->IntegerValue()) {
        return Nan::ThrowTypeError("`start` must be less than or equal to `end`");
    }

    if (info.Length() < 2 || !info[1]->IsFunction()) {
        return Nan::ThrowTypeError("Callback must be a function");
    }

    RangeBaton* baton = new RangeBaton(obj,
                                       info[1],
                                       start->Uint32Value(),
                                       end->Uint32Value());
    uv_queue_work(uv_default_loop(), &baton->request, RangeAsync, reinterpret_cast<uv_after_work_cb>(AfterRange));
}

struct ft_library_guard {
    // non copyable
    ft_library_guard(ft_library_guard const&) = delete;
    ft_library_guard& operator=(ft_library_guard const&) = delete;

    ft_library_guard(FT_Library* lib) : library_(lib) {}

    ~ft_library_guard() {
        if (library_) FT_Done_FreeType(*library_);
    }

    FT_Library* library_;
};

struct ft_face_guard {
    // non copyable
    ft_face_guard(ft_face_guard const&) = delete;
    ft_face_guard& operator=(ft_face_guard const&) = delete;
    ft_face_guard(FT_Face* f) : face_(f) {}

    ~ft_face_guard() {
        if (face_) {
            FT_Done_Face(*face_);
        }
    }

    FT_Face* face_;
};

void LoadAsync(uv_work_t* req) {
    LoadBaton* baton = static_cast<LoadBaton*>(req->data);
    try {
        FT_Library library = nullptr;
        ft_library_guard library_guard(&library);
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            /* LCOV_EXCL_START */
            baton->error_name = std::string("could not open FreeType library");
            return;
            /* LCOV_EXCL_END */
        }
        FT_Face ft_face = 0;
        FT_Long num_faces = 0;
        for (int i = 0; ft_face == 0 || i < num_faces; ++i) {
            ft_face_guard face_guard(&ft_face);
            FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
            if (face_error) {
                baton->error_name = std::string("could not open font file");
                return;
            }
            if (num_faces == 0) {
                num_faces = ft_face->num_faces;
            }

            if (ft_face->family_name) {
                std::set<int> points;
                FT_ULong charcode;
                FT_UInt gindex;
                charcode = FT_Get_First_Char(ft_face, &gindex);
                while (gindex != 0) {
                    charcode = FT_Get_Next_Char(ft_face, charcode, &gindex);
                    if (charcode != 0) points.emplace(charcode);
                }

                std::vector<int> points_vec(points.begin(), points.end());

                if (ft_face->style_name) {
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

    LoadBaton* baton = static_cast<LoadBaton*>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = {Nan::Error(baton->error_name.c_str())};
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Array> js_faces = Nan::New<v8::Array>(baton->faces.size());
        unsigned idx = 0;
        for (auto const& face : baton->faces) {
            v8::Local<v8::Object> js_face = Nan::New<v8::Object>();
            js_face->Set(Nan::New("family_name").ToLocalChecked(), Nan::New(face.family_name).ToLocalChecked());
            if (!face.style_name.empty()) js_face->Set(Nan::New("style_name").ToLocalChecked(), Nan::New(face.style_name).ToLocalChecked());
            v8::Local<v8::Array> js_points = Nan::New<v8::Array>(face.points.size());
            unsigned p_idx = 0;
            for (auto const& pt : face.points) {
                js_points->Set(p_idx++, Nan::New(pt));
            }
            js_face->Set(Nan::New("points").ToLocalChecked(), js_points);
            js_faces->Set(idx++, js_face);
        }
        v8::Local<v8::Value> argv[2] = {Nan::Null(), js_faces};
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }
    delete baton;
}

void RangeAsync(uv_work_t* req) {
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);
    try {

        unsigned array_size = baton->end - baton->start;
        baton->chars.reserve(array_size);
        for (unsigned i = baton->start; i <= baton->end; i++) {
            baton->chars.emplace_back(i);
        }

        FT_Library library = nullptr;
        ft_library_guard library_guard(&library);
        FT_Error error = FT_Init_FreeType(&library);
        if (error) {
            /* LCOV_EXCL_START */
            baton->error_name = std::string("could not open FreeType library");
            return;
            /* LCOV_EXCL_END */
        }

        protozero::pbf_writer pbf_writer{baton->message};
        FT_Face ft_face = 0;
        FT_Long num_faces = 0;
        for (int i = 0; ft_face == 0 || i < num_faces; ++i) {
            ft_face_guard face_guard(&ft_face);
            FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
            if (face_error) {
                baton->error_name = std::string("could not open font");
                return;
            }

            if (num_faces == 0) {
                num_faces = ft_face->num_faces;
            }

            if (ft_face->family_name) {
                protozero::pbf_writer fontstack_writer{pbf_writer, 1};
                if (ft_face->style_name) {
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

                    if (!char_index) continue;

                    glyph.glyph_index = char_index;
                    sdf_glyph_foundry::RenderSDF(glyph, 24, 3, 0.25, ft_face);

                    // Add glyph to fontstack.
                    protozero::pbf_writer glyph_writer{fontstack_writer, 3};

                    // shortening conversion
                    if (char_code > std::numeric_limits<FT_ULong>::max()) {
                        throw std::runtime_error("Invalid value for char_code: too large");
                    } else {
                        glyph_writer.add_uint32(1, static_cast<std::uint32_t>(char_code));
                    }

                    if (glyph.width > 0) {
                        glyph_writer.add_bytes(2, glyph.bitmap);
                    }

                    // direct type conversions, no need for checking or casting
                    glyph_writer.add_uint32(3, glyph.width);
                    glyph_writer.add_uint32(4, glyph.width);
                    glyph_writer.add_sint32(5, glyph.width);

                    // conversions requiring checks, for safety and correctness

                    // double to int
                    double top = static_cast<double>(glyph.top) - glyph.ascender;
                    if (top < std::numeric_limits<std::int32_t>::min() || top > std::numeric_limits<std::int32_t>::max()) {
                        throw std::runtime_error("Invalid value for glyph.top-glyph.ascender");
                    } else {
                        glyph_writer.add_sint32(6, static_cast<std::int32_t>(top));
                    }

                    // double to uint
                    if (glyph.advance < std::numeric_limits<std::uint32_t>::min() || glyph.advance > std::numeric_limits<std::uint32_t>::max()) {
                        throw std::runtime_error("Invalid value for glyph.top-glyph.ascender");
                    } else {
                        glyph_writer.add_uint32(7, static_cast<std::uint32_t>(glyph.advance));
                    }
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

    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = {Nan::Error(baton->error_name.c_str())};
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Value> argv[2] = {Nan::Null(), Nan::CopyBuffer(baton->message.data(), static_cast<std::uint32_t>(baton->message.size())).ToLocalChecked()};
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }

    delete baton;
}

} // namespace node_fontnik
