// fontnik
#include "glyphs.hpp"
#include <protozero/pbf_reader.hpp>
#include <protozero/pbf_writer.hpp>
// node
#include <limits>
#include <memory>
#include <napi.h>
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
    FaceMetadata(std::string _family_name,
                 std::string _style_name,
                 std::vector<int>&& _points)
        : family_name(std::move(_family_name)),
          style_name(std::move(_style_name)),
          points(std::move(_points)) {}
    FaceMetadata(std::string _family_name,
                 std::vector<int>&& _points)
        : family_name(std::move(_family_name)),
          points(std::move(_points)) {}
};

struct GlyphPBF {
    explicit GlyphPBF(Napi::Buffer<char> const& buffer)
        : data{buffer.As<Napi::Buffer<char>>().Data(),
               buffer.As<Napi::Buffer<char>>().Length()},
          buffer_ref_{Napi::Persistent(buffer)} {}

    // non-copyable
    GlyphPBF(GlyphPBF const&) = delete;
    GlyphPBF& operator=(GlyphPBF const&) = delete;

    // non-movable
    GlyphPBF(GlyphPBF&&) = delete;
    GlyphPBF& operator=(GlyphPBF&&) = delete;

    protozero::data_view data;
    Napi::Reference<Napi::Buffer<char>> buffer_ref_;
};

struct ft_library_guard {
    // non copyable
    ft_library_guard(ft_library_guard const&) = delete;
    ft_library_guard& operator=(ft_library_guard const&) = delete;

    explicit ft_library_guard(FT_Library* lib) : library_(lib) {}

    ~ft_library_guard() {
        if (library_ != nullptr) {
            FT_Done_FreeType(*library_);
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

struct AsyncLoad : Napi::AsyncWorker {
    using Base = Napi::AsyncWorker;
    AsyncLoad(Napi::Buffer<char> const& buffer, Napi::Function const& callback)
        : Base(callback),
          font_data_{buffer.Data()},
          font_size_{buffer.Length()},
          buffer_ref_{Napi::Persistent(buffer)} {}

    void Execute() override {
        try {
            FT_Library library = nullptr;
            ft_library_guard library_guard(&library);
            FT_Error error = FT_Init_FreeType(&library);
            if (error != 0) {
                //LCOV_EXCL_START
                SetError("could not open FreeType library");
                return;
                // LCOV_EXCL_END
            }
            FT_Face ft_face = nullptr;
            FT_Long num_faces = 0;
            for (int i = 0; ft_face == nullptr || i < num_faces; ++i) {
                ft_face_guard face_guard(&ft_face);
                FT_Error face_error = FT_New_Memory_Face(library,
                                                         reinterpret_cast<FT_Byte const*>(font_data_),
                                                         static_cast<FT_Long>(font_size_), i, &ft_face);
                if (face_error != 0) {
                    SetError("could not open font file");
                    return;
                }
                if (num_faces == 0) {
                    num_faces = ft_face->num_faces;
                    faces_.reserve(static_cast<std::size_t>(num_faces));
                }
                if (ft_face->family_name != nullptr) {
                    std::set<int> points;
                    FT_ULong charcode;
                    FT_UInt gindex;
                    charcode = FT_Get_First_Char(ft_face, &gindex);
                    while (gindex != 0) {
                        charcode = FT_Get_Next_Char(ft_face, charcode, &gindex);
                        if (charcode != 0) points.emplace(charcode);
                    }
                    std::vector<int> points_vec(points.begin(), points.end());
                    if (ft_face->style_name != nullptr) {
                        faces_.emplace_back(ft_face->family_name, ft_face->style_name, std::move(points_vec));
                    } else {
                        faces_.emplace_back(ft_face->family_name, std::move(points_vec));
                    }
                } else {
                    SetError("font does not have family_name or style_name");
                    return;
                }
            }
        } catch (std::exception const& ex) {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override {
        Napi::Array js_faces = Napi::Array::New(env, faces_.size());
        std::uint32_t index = 0;
        for (auto const& face : faces_) {
            Napi::Object js_face = Napi::Object::New(env);
            js_face.Set("family_name", face.family_name);
            if (!face.style_name.empty()) {
                js_face.Set("style_name", face.style_name);
            }
            Napi::Array js_points = Napi::Array::New(env, face.points.size());
            std::uint32_t p_idx = 0;
            for (auto const& pt : face.points) {
                js_points.Set(p_idx++, pt);
            }
            js_face.Set("points", js_points);
            js_faces.Set(index++, js_face);
        }
        return {env.Null(), js_faces};
    }

  private:
    char const* font_data_;
    std::size_t font_size_;
    Napi::Reference<Napi::Buffer<char>> buffer_ref_;
    std::vector<FaceMetadata> faces_;
};

struct AsyncRange : Napi::AsyncWorker {
    using Base = Napi::AsyncWorker;
    AsyncRange(Napi::Buffer<char> const& buffer, std::uint32_t start, std::uint32_t end, Napi::Function const& callback)
        : Base(callback),
          font_data_{buffer.Data()},
          font_size_{buffer.Length()},
          start_(start),
          end_(end),
          buffer_ref_{Napi::Persistent(buffer)} {}

    void Execute() override {
        try {
            unsigned array_size = end_ - start_;
            chars_.reserve(array_size);
            for (unsigned i = start_; i <= end_; ++i) {
                chars_.emplace_back(i);
            }

            FT_Library library = nullptr;
            ft_library_guard library_guard(&library);
            FT_Error error = FT_Init_FreeType(&library);
            if (error != 0) {
                // LCOV_EXCL_START
                SetError("could not open FreeType library");
                return;
                // LCOV_EXCL_END
            }

            protozero::pbf_writer pbf_writer{message_};
            FT_Face ft_face = nullptr;
            FT_Long num_faces = 0;
            for (int i = 0; ft_face == nullptr || i < num_faces; ++i) {
                ft_face_guard face_guard(&ft_face);
                FT_Error face_error = FT_New_Memory_Face(library,
                                                         reinterpret_cast<FT_Byte const*>(font_data_),
                                                         static_cast<FT_Long>(font_size_), i, &ft_face);
                if (face_error != 0) {
                    SetError("could not open font");
                    return;
                }

                if (num_faces == 0) num_faces = ft_face->num_faces;

                if (ft_face->family_name != nullptr) {
                    protozero::pbf_writer fontstack_writer{pbf_writer, 1};
                    if (ft_face->style_name != nullptr) {
                        fontstack_writer.add_string(1, std::string(ft_face->family_name) + " " + std::string(ft_face->style_name));
                    } else {
                        fontstack_writer.add_string(1, std::string(ft_face->family_name));
                    }
                    fontstack_writer.add_string(2, std::to_string(start_) + "-" + std::to_string(end_));

                    const double scale_factor = 1.0;

                    // Set character sizes.
                    double size = 24 * scale_factor;
                    FT_Set_Char_Size(ft_face, 0, static_cast<FT_F26Dot6>(size * (1 << 6)), 0, 0);

                    for (std::vector<uint32_t>::size_type x = 0; x != chars_.size(); ++x) {
                        FT_ULong char_code = chars_[x];
                        sdf_glyph_foundry::glyph_info glyph;
                        // Get FreeType face from face_ptr.
                        FT_UInt char_index = FT_Get_Char_Index(ft_face, char_code);
                        if (char_index == 0U) continue;

                        glyph.glyph_index = char_index;
                        sdf_glyph_foundry::RenderSDF(glyph, 24, 3, 0.25, ft_face);

                        // Add glyph to fontstack.
                        protozero::pbf_writer glyph_writer{fontstack_writer, 3};

                        // shortening conversion
                        if (char_code > std::numeric_limits<FT_ULong>::max()) {
                            SetError("Invalid value for char_code: too large");
                            return;
                        }
                        glyph_writer.add_uint32(1, static_cast<std::uint32_t>(char_code));

                        if (glyph.width > 0) glyph_writer.add_bytes(2, glyph.bitmap);

                        // direct type conversions, no need for checking or casting
                        glyph_writer.add_uint32(3, glyph.width);
                        glyph_writer.add_uint32(4, glyph.height);
                        glyph_writer.add_sint32(5, glyph.left);

                        // conversions requiring checks, for safety and correctness

                        // double to int
                        double top = static_cast<double>(glyph.top) - glyph.ascender;
                        if (top < std::numeric_limits<std::int32_t>::min() || top > std::numeric_limits<std::int32_t>::max()) {
                            SetError("Invalid value for glyph.top-glyph.ascender");
                            return;
                        }
                        glyph_writer.add_sint32(6, static_cast<std::int32_t>(top));

                        // double to uint
                        if (glyph.advance < std::numeric_limits<std::uint32_t>::min() || glyph.advance > std::numeric_limits<std::uint32_t>::max()) {
                            SetError("Invalid value for glyph.top-glyph.ascender");
                            return;
                        }
                        glyph_writer.add_uint32(7, static_cast<std::uint32_t>(glyph.advance));
                    }
                } else {
                    SetError("font does not have family_name");
                    return;
                }
            }
        } catch (std::exception const& ex) {
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override {
        return {env.Null(), Napi::Buffer<char>::Copy(env, message_.data(), message_.size())};
    }

  private:
    char const* font_data_;
    std::size_t font_size_;
    std::uint32_t start_;
    std::uint32_t end_;
    Napi::Reference<Napi::Buffer<char>> buffer_ref_;
    std::vector<std::uint32_t> chars_;
    std::string message_;
};

Napi::Value Load(Napi::CallbackInfo const& info) {
    Napi::Env env = info.Env();
    // Validate arguments.
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "First argument must be a font buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object obj = info[0].As<Napi::Object>();

    if (!obj.IsBuffer()) {
        Napi::TypeError::New(env, "First argument must be a font buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[1].IsFunction()) {
        Napi::TypeError::New(env, "Callback must be a function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto* worker = new AsyncLoad(obj.As<Napi::Buffer<char>>(), info[1].As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

Napi::Value Range(Napi::CallbackInfo const& info) {
    Napi::Env env = info.Env();

    // Validate arguments.
    if (info.Length() < 1 || !info[0].IsObject()) {
        Napi::TypeError::New(env, "First argument must be an object of options").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object options = info[0].As<Napi::Object>();
    Napi::Value font_buffer = options.Get("font");
    if (!font_buffer.IsObject()) {
        Napi::TypeError::New(env, "Font buffer is not an object").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    Napi::Object obj = font_buffer.As<Napi::Object>();
    Napi::Value start_val = options.Get("start");
    Napi::Value end_val = options.Get("end");

    if (!obj.IsBuffer()) {
        Napi::TypeError::New(env, "First argument must be a font buffer").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!start_val.IsNumber() || start_val.As<Napi::Number>().Int32Value() < 0) {
        Napi::TypeError::New(env, "option `start` must be a number from 0-65535").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (!end_val.IsNumber() || end_val.As<Napi::Number>().Int32Value() > 65535) {
        Napi::TypeError::New(env, "option `end` must be a number from 0-65535").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    std::uint32_t start = start_val.As<Napi::Number>().Uint32Value();
    std::uint32_t end = end_val.As<Napi::Number>().Uint32Value();

    if (end < start) {
        Napi::TypeError::New(env, "`start` must be less than or equal to `end`").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    if (info.Length() < 2 || !info[1].IsFunction()) {
        Napi::TypeError::New(env, "Callback must be a function").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto* worker = new AsyncRange(obj.As<Napi::Buffer<char>>(), start, end, info[1].As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

struct AsyncComposite : Napi::AsyncWorker {
    using Base = Napi::AsyncWorker;

    AsyncComposite(std::vector<std::unique_ptr<GlyphPBF>>&& glyphs, Napi::Function const& callback)
        : Base(callback),
          glyphs_(std::move(glyphs)),
          message_(std::make_unique<std::string>()) {}

    void Execute() override {
        try {
            std::vector<std::unique_ptr<std::vector<char>>> buffer_cache;
            std::map<std::uint32_t, protozero::data_view> id_mapping;
            bool first_buffer = true;
            std::string fontstack_name;
            std::string range;
            protozero::pbf_writer pbf_writer(*message_);
            protozero::pbf_writer fontstack_writer{pbf_writer, 1};
            // TODO(danespringmeyer): avoid duplicate fontstacks to be sent it
            for (auto const& glyph_obj : glyphs_) {
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
            SetError(ex.what());
        }
    }

    std::vector<napi_value> GetResult(Napi::Env env) override {
        std::string& str = *message_;
        auto buffer = Napi::Buffer<char>::New(
            env, &str[0], str.size(),
            [](Napi::Env env_, char* /*unused*/, std::string* str_ptr) {
                if (str_ptr != nullptr) {
                    Napi::MemoryManagement::AdjustExternalMemory(env_, -static_cast<std::int64_t>(str_ptr->size()));
                }
                delete str_ptr;
            },
            message_.release());
        Napi::MemoryManagement::AdjustExternalMemory(env, static_cast<std::int64_t>(str.size()));
        return {env.Null(), buffer};
    }

  private:
    std::vector<std::unique_ptr<GlyphPBF>> glyphs_;
    std::unique_ptr<std::string> message_;
};

Napi::Value Composite(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    // validate callback function
    Napi::Value callback_val = info[info.Length() - 1];
    if (!callback_val.IsFunction()) {
        Napi::Error::New(env, "last argument must be a callback function").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    // validate glyphPBF array
    if (!info[0].IsArray()) {
        Napi::TypeError::New(env, "first arg 'glyphs' must be an array of glyphs objects").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Array glyphs_array = info[0].As<Napi::Array>();
    std::size_t num_glyphs = glyphs_array.Length();

    if (num_glyphs <= 0) {
        Napi::TypeError::New(env, "'glyphs' array must be of length greater than 0").ThrowAsJavaScriptException();
        return env.Undefined();
    }

    std::vector<std::unique_ptr<GlyphPBF>> glyphs{};
    glyphs.reserve(num_glyphs);
    for (std::uint32_t index = 0; index < num_glyphs; ++index) {
        Napi::Value buf_val = glyphs_array.Get(index);
        if (!buf_val.IsBuffer()) {
            Napi::TypeError::New(env, "buffer value in 'glyphs' array item is not a true buffer");
            return env.Undefined();
        }
        Napi::Buffer<char> buffer = buf_val.As<Napi::Buffer<char>>();
        if (buffer.IsEmpty()) {
            Napi::TypeError::New(env, "buffer value in 'glyphs' array is empty");
            return env.Undefined();
        }
        glyphs.push_back(std::make_unique<GlyphPBF>(buffer));
    }

    auto* worker = new AsyncComposite(std::move(glyphs), callback_val.As<Napi::Function>());
    worker->Queue();
    return env.Undefined();
}

} // namespace node_fontnik
