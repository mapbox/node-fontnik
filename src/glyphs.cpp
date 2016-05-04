// fontnik
#include "glyphs.hpp"

// node
#include <node_buffer.h>
#include <nan.h>

#include "agg_curves.h"

// boost
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#pragma GCC diagnostic pop

// std
#include <cmath> // std::sqrt
#include <fstream>
#include <codecvt>

#include "hb.h"
#include "hb-ft.h"

#include <protozero/pbf_reader.hpp>

namespace bg = boost::geometry;
namespace bgm = bg::model;
namespace bgi = bg::index;
typedef bgm::point<float, 2, bg::cs::cartesian> Point;
typedef bgm::box<Point> Box;
typedef std::vector<Point> Points;
typedef std::vector<Points> Rings;
typedef std::pair<Point, Point> SegmentPair;
typedef std::pair<Box, SegmentPair> SegmentValue;
typedef bgi::rtree<SegmentValue, bgi::rstar<16>> Tree;

namespace node_fontnik {

struct FaceMetadata {
    std::string family_name;
    std::string style_name;
    std::vector<int> points;
    FaceMetadata(std::string const& _family_name,
                 std::string const& _style_name,
                 std::vector<int> && _points) :
        family_name(_family_name),
        style_name(_style_name),
        points(std::move(_points)) {}
    FaceMetadata(std::string const& _family_name,
                 std::vector<int> && _points) :
        family_name(_family_name),
        points(std::move(_points)) {}
};

struct LoadBaton {
    Nan::Persistent<v8::Function> callback;
    Nan::Persistent<v8::Object> buffer;
    const char* font_data;
    size_t font_size;
    std::string error_name;
    std::vector<FaceMetadata> faces;
    uv_work_t request;
    LoadBaton(v8::Local<v8::Object> buf,
              v8::Local<v8::Value> cb) :
        font_data(node::Buffer::Data(buf)),
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
    Nan::Persistent<v8::Function> callback;
    Nan::Persistent<v8::Object> buffer;
    const char* font_data;
    size_t font_size;
    std::string error_name;
    std::uint32_t start;
    std::uint32_t end;
    std::vector<std::uint32_t> chars;
    std::string message;
    uv_work_t request;
    RangeBaton(v8::Local<v8::Object> buf,
               v8::Local<v8::Value> cb,
               std::uint32_t _start,
               std::uint32_t _end) :
        font_data(node::Buffer::Data(buf)),
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

struct ShapeBaton {
    uv_work_t request;
    ShapeBaton() :
        request() {
            request.data = this;
        }
    ~ShapeBaton() {}
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

    LoadBaton* baton = new LoadBaton(obj,info[1]);
    uv_queue_work(uv_default_loop(), &baton->request, LoadAsync, (uv_after_work_cb)AfterLoad);
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
                                       start->IntegerValue(),
                                       end->IntegerValue());
    uv_queue_work(uv_default_loop(), &baton->request, RangeAsync, (uv_after_work_cb)AfterRange);
}

NAN_METHOD(Shape) {
    ShapeBaton* baton = new ShapeBaton();
    uv_queue_work(uv_default_loop(), &baton->request, ShapeAsync, (uv_after_work_cb)AfterShape);
}

struct ft_library_guard {
    ft_library_guard(FT_Library * lib) :
        library_(lib) {}

    ~ft_library_guard() {
        if (library_) FT_Done_FreeType(*library_);
    }

    FT_Library * library_;
};

struct ft_glyph_guard {
    ft_glyph_guard(FT_Glyph * glyph) :
        glyph_(glyph) {}

    ~ft_glyph_guard() {
        if (glyph_) FT_Done_Glyph(*glyph_);
    }

    FT_Glyph * glyph_;
};

void LoadAsync(uv_work_t* req) {
    LoadBaton* baton = static_cast<LoadBaton*>(req->data);

    FT_Library library = nullptr;
    ft_library_guard library_guard(&library);
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        /* LCOV_EXCL_START */
        baton->error_name = std::string("Could not open FreeType library");
        return;
        /* LCOV_EXCL_END */
    }
    FT_Face ft_face = 0;
    int num_faces = 0;
    for (int i = 0; ft_face == 0 || i < num_faces; ++i) {
        FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
        if (face_error) {
            baton->error_name = std::string("Could not open font file");
            return;
        }
        std::set<int> points;
        if (num_faces == 0)
            num_faces = ft_face->num_faces;
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

        if (ft_face) {
            FT_Done_Face(ft_face);
        }
    }
};

void AfterLoad(uv_work_t* req) {
    Nan::HandleScope scope;

    LoadBaton* baton = static_cast<LoadBaton*>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(baton->error_name.c_str()) };
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
                js_points->Set(p_idx++,Nan::New(pt));
            }
            js_face->Set(Nan::New("points").ToLocalChecked(), js_points);
            js_faces->Set(idx++,js_face);
        }
        v8::Local<v8::Value> argv[2] = { Nan::Null(), js_faces };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }
    delete baton;
};


void parseGlyphMetrics(Glyph *glyph, protozero::pbf_reader glyph_metrics_pbf) {
    while (glyph_metrics_pbf.next()) {
        switch (glyph_metrics_pbf.tag()) {
        case 1: // x_bearing
            glyph->metrics.x_bearing = glyph_metrics_pbf.get_sint32();
            break;
        case 2: // y_bearing
            glyph->metrics.y_bearing = glyph_metrics_pbf.get_sint32();
            break;
        case 3: // width
            glyph->metrics.width = glyph_metrics_pbf.get_uint32();
            break;
        case 4: // height
            glyph->metrics.height = glyph_metrics_pbf.get_uint32();
            break;
        case 5: // advance
            glyph->metrics.advance = glyph_metrics_pbf.get_uint32();
            break;
        default:
            glyph_metrics_pbf.skip();
            break;
        }
    }
}

void parseGlyph(protozero::pbf_reader glyph_pbf) {
    Glyph glyph;

    while (glyph_pbf.next()) {
        switch (glyph_pbf.tag()) {
        case 1: // glyph_index
            glyph.glyph_index = glyph_pbf.get_uint32();
            break;
        case 2: // codepoint
            glyph.codepoint = glyph_pbf.get_uint32();
            break;
        case 3: // metrics
            parseGlyphMetrics(&glyph, glyph_pbf.get_message());
            break;
        case 4: // bitmap
            glyph.bitmap = glyph_pbf.get_string();
            break;
        default:
            glyph_pbf.skip();
            break;
        }
    }

    // glyphSet.insert(glyph.id, glyph);

    std::cout << "glyph_index: " << glyph.glyph_index << 
                 " codepoint: " << glyph.codepoint << 
                 std::endl;
}

void parseFaceMetrics(protozero::pbf_reader face_metrics_pbf) {
    while (face_metrics_pbf.next()) {
        switch (face_metrics_pbf.tag()) {
        case 1: // ascender
            std::cout << "ascender: " << face_metrics_pbf.get_double() << std::endl;
            break;
        case 2: // descender
            std::cout << "descender: " << face_metrics_pbf.get_double() << std::endl;
            break;
        case 3: // line_height
            std::cout << "line_height: " << face_metrics_pbf.get_double() << std::endl;
            break;
        default:
            face_metrics_pbf.skip();
            break;
        }
    }
}

void parseFontMetadata(protozero::pbf_reader metadata_pbf) {
    while (metadata_pbf.next()) {
        switch (metadata_pbf.tag()) {
        case 1: // size
            std::cout << "size: " << metadata_pbf.get_uint32() << std::endl;
            break;
        case 2: // scale
            std::cout << "scale: " << metadata_pbf.get_float() << std::endl;
            break;
        case 3: // buffer
            std::cout << "buffer: " << metadata_pbf.get_uint32() << std::endl;
            break;
        case 4: // radius
            std::cout << "radius: " << metadata_pbf.get_uint32() << std::endl;
            break;
        case 5: // offset
            std::cout << "offset: " << metadata_pbf.get_float() << std::endl;
            break;
        case 6: // cutoff
            std::cout << "cutoff: " << metadata_pbf.get_float() << std::endl;
            break;
        case 7: // granularity
            std::cout << "granularity: " << metadata_pbf.get_uint32() << std::endl;
            break;
        default:
            metadata_pbf.skip();
            break;
        }
    }
}

void RangeAsync(uv_work_t* req) {
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    unsigned array_size = baton->end - baton->start;
    baton->chars.reserve(array_size);
    for (unsigned i=baton->start; i <= baton->end; i++) {
        baton->chars.emplace_back(i);
    }

    FT_Library library = nullptr;
    ft_library_guard library_guard(&library);
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        /* LCOV_EXCL_START */
        baton->error_name = std::string("Could not open FreeType library");
        return;
        /* LCOV_EXCL_END */
    }

    FT_Face ft_face = 0;

    mapbox::fontnik::Font mutable_font;

    // Add metadata to font.
    mapbox::fontnik::Metadata *mutable_metadata = mutable_font.mutable_metadata();
    mutable_metadata->set_size(char_size);
    mutable_metadata->set_buffer(buffer_size);
    mutable_metadata->set_scale(scale_factor);
    mutable_metadata->set_granularity(granularity);
    mutable_metadata->set_offset(offset_size);
    mutable_metadata->set_radius(radius_size);
    mutable_metadata->set_cutoff(cutoff_size);

    int num_faces = 0;
    for (int i = 0; ft_face == 0 || i < num_faces; ++i) {
        FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
        if (face_error) {
            baton->error_name = std::string("Could not open font");
            return;
        }

        mapbox::fontnik::Face *mutable_face = mutable_font.add_faces();
        // mutable_face->set_range(std::to_string(baton->start) + "-" + std::to_string(baton->end));

        if (ft_face->family_name) {
            mutable_face->set_family_name(ft_face->family_name);
        }

        if (ft_face->style_name) {
            mutable_face->set_style_name(ft_face->style_name);
        }

        /*
        // TODO: verify if these values match following values?
        mutable_face->set_ascender(ft_face->ascender);
        mutable_face->set_descender(ft_face->descender);
        mutable_face->set_line_height(ft_face->height);
        */

        // Add metrics to face.
        mapbox::fontnik::FaceMetrics *mutable_face_metrics = mutable_face->mutable_metrics();
        // Ascender and descender from baseline returned by FreeType
        mutable_face_metrics->set_ascender(ft_face->size->metrics.ascender / 64);
        mutable_face_metrics->set_descender(ft_face->size->metrics.descender / 64);

        // Line height returned by FreeType, includes normal font
        // line spacing, but not additional user defined spacing
        mutable_face_metrics->set_line_height(ft_face->size->metrics.height);

        // Set character sizes.
        double size = char_size * scale_factor;
        FT_Set_Char_Size(ft_face,0,(FT_F26Dot6)(size * (1<<6)),0,0);

        for (std::vector<uint32_t>::size_type x = 0; x != baton->chars.size(); x++) {
            FT_ULong codepoint = baton->chars[x];

            // Return the glyph index of a given character code.
            FT_UInt glyph_index = FT_Get_Char_Index(ft_face, codepoint);

            if (!glyph_index) continue;

            Glyph glyph;
            glyph.glyph_index = glyph_index;
            glyph.codepoint = codepoint;

            RenderSDF(ft_face, glyph);

            // Add glyph to face.
            mapbox::fontnik::Glyph *mutable_glyph = mutable_face->add_glyphs();
            mutable_glyph->set_glyph_index(glyph_index);
            mutable_glyph->set_codepoint(codepoint);

            mapbox::fontnik::GlyphMetrics *mutable_glyph_metrics = mutable_glyph->mutable_metrics();
            mutable_glyph_metrics->set_x_bearing(glyph.metrics.x_bearing);
            mutable_glyph_metrics->set_y_bearing(glyph.metrics.y_bearing);
            mutable_glyph_metrics->set_width(glyph.metrics.width);
            mutable_glyph_metrics->set_height(glyph.metrics.height);
            mutable_glyph_metrics->set_advance(glyph.metrics.advance);

            if (glyph.metrics.width > 0) {
                mutable_glyph->set_bitmap(glyph.bitmap);
            }

        }

        if (ft_face) {
            FT_Done_Face(ft_face);
        }
    }

    baton->message = mutable_font.SerializeAsString();

    protozero::pbf_reader font_pbf(baton->message);

    while (font_pbf.next()) {
        protozero::pbf_reader face_pbf;
        protozero::pbf_reader metadata_pbf;

        switch (font_pbf.tag()) {
        case 1: // faces
            face_pbf = font_pbf.get_message();

            while (face_pbf.next()) {
                switch (face_pbf.tag()) {
                case 1: // family_name
                case 2: // style_name
                    std::cout << face_pbf.get_string() << std::endl;
                    break;
                case 3: // glyphs
                    parseGlyph(face_pbf.get_message());
                    break;
                case 4: // metrics
                    parseFaceMetrics(face_pbf.get_message());
                    break;
                default:
                    face_pbf.skip();
                    break;
                }
            }
            break;
        case 2: // metadata
            parseFontMetadata(font_pbf.get_message());
            break;
        default:
            font_pbf.skip();
            break;
        }
    }
}

void AfterRange(uv_work_t* req) {
    Nan::HandleScope scope;

    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = { Nan::Error(baton->error_name.c_str()) };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Array> js_faces = Nan::New<v8::Array>();
        unsigned idx = 0;
        v8::Local<v8::Value> argv[2] = { Nan::Null(), Nan::CopyBuffer(baton->message.data(), baton->message.size()).ToLocalChecked() };
        Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New(baton->callback), 2, argv);
    }

    delete baton;
};

void ShapeAsync(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);

    std::string filename("/Users/mikemorris/Desktop/Open_Sans/OpenSans-Regular.ttf");

    char* font_data;
    size_t font_size;

    std::ifstream file(filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
        font_size = file.tellg();
        font_data = new char[font_size];
        file.seekg(0, std::ios::beg);
        file.read(font_data, font_size);
        file.close();
    } else {
        throw std::runtime_error("Failed to open font file");
    }

    hb_font_t* hb_font(hb_ft_font_create(font_data, [](void* data) {
        delete[] static_cast<char*>(data);
    }));

    FT_Library library = nullptr;
    ft_library_guard library_guard(&library);
    FT_Error error = FT_Init_FreeType(&library);
    if (error) {
        throw std::runtime_error("Failed to open FreeType library");
    }

    FT_Face ft_face = 0;

    int num_faces = 0;
    for (size_t i = 0; ft_face == 0 || i < num_faces; ++i) {
        FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(font_data), static_cast<FT_Long>(font_size), i, &ft_face);
        if (face_error) {
            throw std::runtime_error("Failed to create FreeType face");
        }

        if (num_faces == 0) {
            num_faces = ft_face->num_faces;
        }

        if (ft_face) {
            // Set character sizes.
            // This is mandatory before calling FT_Load_Glyph.
            double size = char_size * scale_factor;
            FT_Set_Char_Size(ft_face, 0, (FT_F26Dot6)(size * (1<<6)), 0, 0);

            hb_blob_t* hb_blob = hb_blob_create(font_data, font_size, HB_MEMORY_MODE_WRITABLE, font_data, [](void* data) {
                delete[] static_cast<char*>(data);
            });

            hb_face_t* hb_face = hb_face_create(hb_blob, i);
            hb_blob_destroy(hb_blob);

            hb_font_t* hb_font(hb_font_create(hb_face));

            /*
            hb_font_t* hb_font(hb_ft_font_create(font_data, [](void* data) {
                delete[] static_cast<char*>(data);
            }));
            */

            const unsigned int upem = hb_face_get_upem(hb_face);
            hb_face_destroy(hb_face);

            // hb_font_set_scale(hb_font, upem, upem);
            hb_font_set_scale(hb_font, 1, 1);
            hb_ft_font_set_funcs(hb_font);

            hb_buffer_t* hb_buffer(hb_buffer_create());

            // TODO: itemize string here
            const std::u32string text(U"Hello world!");

            hb_buffer_reset(hb_buffer);
            hb_buffer_add_utf32(hb_buffer, reinterpret_cast<const uint32_t*>(text.c_str()), text.length(), 0, text.length());

            hb_buffer_set_direction(hb_buffer, HB_DIRECTION_LTR);
            // hb_buffer_set_direction(buffer, hb_direction_from_string (direction, -1));

            hb_buffer_set_script(hb_buffer, HB_SCRIPT_LATIN);
            // hb_buffer_set_script(buffer, hb_script_from_string (script, -1));

            hb_buffer_set_language(hb_buffer, hb_language_from_string("en", -1));
            // hb_buffer_set_language(buffer, hb_language_from_string (language, -1));

            hb_shape(hb_font, hb_buffer, 0 /*features*/, 0 /*num_features*/);

            const size_t num_glyphs = hb_buffer_get_length(hb_buffer);
            hb_glyph_info_t* hb_glyph_infos = hb_buffer_get_glyph_infos(hb_buffer, NULL);
            hb_glyph_position_t* hb_glyph_positions = hb_buffer_get_glyph_positions(hb_buffer, NULL);

            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cv;
            std::cout << "text: " << cv.to_bytes(text) << 
                         " num_glyphs: " << num_glyphs << std::endl;

            uint32_t glyph_index;
            for (size_t j = 0; j < num_glyphs; j++) {
                /**
                 * hb_glyph_info_t:
                 * @codepoint: either a Unicode code point (before 
                 * shaping) or a glyph index (after shaping).
                 */
                glyph_index = hb_glyph_infos[j].codepoint;

                std::cout << "glyph_index: " << glyph_index <<
                             " hb_cluster: " << hb_glyph_infos[j].cluster <<
                             " hb_mask: " << hb_glyph_infos[j].mask <<
                             " hb_x_advance: " << hb_glyph_positions[j].x_advance <<
                             // " hb_y_advance: " << hb_glyph_positions[j].y_advance <<
                             // " hb_x_offset: " << hb_glyph_positions[j].x_offset <<
                             // " hb_y_offset: " << hb_glyph_positions[j].y_offset << 
                             std::endl;

                if (FT_Load_Glyph(ft_face, static_cast<FT_UInt>(glyph_index), FT_LOAD_NO_HINTING)) continue;

                FT_Glyph ft_glyph = nullptr;
                ft_glyph_guard glyph_guard(&ft_glyph);
                if (FT_Get_Glyph(ft_face->glyph, &ft_glyph)) continue;

                std::cout << "ft_glyph loaded" << std::endl;

                // TODO: RenderSDF(glyph_index);
            }

            hb_buffer_destroy(hb_buffer);
            hb_font_destroy(hb_font);

            FT_Done_Face(ft_face);
        }
    }
};

void AfterShape(uv_work_t* req) {
    ShapeBaton* baton = static_cast<ShapeBaton*>(req->data);
    delete baton;
};

struct User {
    Rings rings;
    Points ring;
};

void CloseRing(Points &ring) {
    const Point &first = ring.front();
    const Point &last = ring.back();

    if (first.get<0>() != last.get<0>() ||
        first.get<1>() != last.get<1>()) {
        ring.push_back(first);
    }
}

int MoveTo(const FT_Vector *to, void *ptr) {
    User *user = (User*)ptr;
    if (!user->ring.empty()) {
        CloseRing(user->ring);
        user->rings.push_back(user->ring);
        user->ring.clear();
    }
    user->ring.push_back(Point { float(to->x) / 64, float(to->y) / 64 });
    return 0;
}

int LineTo(const FT_Vector *to, void *ptr) {
    User *user = (User*)ptr;
    user->ring.push_back(Point { float(to->x) / 64, float(to->y) / 64 });
    return 0;
}

int ConicTo(const FT_Vector *control,
            const FT_Vector *to,
            void *ptr) {
    User *user = (User*)ptr;

    Point prev = user->ring.back();

    // pop off last point, duplicate of first point in bezier curve
    user->ring.pop_back();

    agg_fontnik::curve3_div curve(prev.get<0>(), prev.get<1>(),
                            float(control->x) / 64, float(control->y) / 64,
                            float(to->x) / 64, float(to->y) / 64);

    curve.rewind(0);
    double x, y;
    unsigned cmd;

    while (agg_fontnik::path_cmd_stop != (cmd = curve.vertex(&x, &y))) {
        user->ring.push_back(Point {x, y});
    }

    return 0;
}

int CubicTo(const FT_Vector *c1,
            const FT_Vector *c2,
            const FT_Vector *to,
            void *ptr) {
    User *user = (User*)ptr;

    Point prev = user->ring.back();

    // pop off last point, duplicate of first point in bezier curve
    user->ring.pop_back();

    agg_fontnik::curve4_div curve(prev.get<0>(), prev.get<1>(),
                            float(c1->x) / 64, float(c1->y) / 64,
                            float(c2->x) / 64, float(c2->y) / 64,
                            float(to->x) / 64, float(to->y) / 64);

    curve.rewind(0);
    double x, y;
    unsigned cmd;

    while (agg_fontnik::path_cmd_stop != (cmd = curve.vertex(&x, &y))) {
        user->ring.push_back(Point {x, y});
    }

    return 0;
}

// point in polygon ray casting algorithm
bool PolyContainsPoint(const Rings &rings, const Point &p) {
    bool c = false;

    for (const Points &ring : rings) {
        auto p1 = ring.begin();
        auto p2 = p1 + 1;

        for (; p2 != ring.end(); p1++, p2++) {
            if (((p1->get<1>() > p.get<1>()) != (p2->get<1>() > p.get<1>())) && (p.get<0>() < (p2->get<0>() - p1->get<0>()) * (p.get<1>() - p1->get<1>()) / (p2->get<1>() - p1->get<1>()) + p1->get<0>())) {
                c = !c;
            }
        }
    }

    return c;
}

double SquaredDistance(const Point &v, const Point &w) {
    const double a = v.get<0>() - w.get<0>();
    const double b = v.get<1>() - w.get<1>();
    return a * a + b * b;
}

Point ProjectPointOnLineSegment(const Point &p,
                                const Point &v,
                                const Point &w) {
  const double l2 = SquaredDistance(v, w);
  if (l2 == 0) return v;

  const double t = ((p.get<0>() - v.get<0>()) * (w.get<0>() - v.get<0>()) + (p.get<1>() - v.get<1>()) * (w.get<1>() - v.get<1>())) / l2;
  if (t < 0) return v;
  if (t > 1) return w;

  return Point {
      v.get<0>() + t * (w.get<0>() - v.get<0>()),
      v.get<1>() + t * (w.get<1>() - v.get<1>())
  };
}

double SquaredDistanceToLineSegment(const Point &p,
                                    const Point &v,
                                    const Point &w) {
    const Point s = ProjectPointOnLineSegment(p, v, w);
    return SquaredDistance(p, s);
}

double MinDistanceToLineSegment(const Tree &tree,
                                const Point &p,
                                int radius) {
    const int squared_radius = radius * radius;

    std::vector<SegmentValue> results;
    tree.query(bgi::intersects(
        Box{
            Point{p.get<0>() - radius, p.get<1>() - radius},
            Point{p.get<0>() + radius, p.get<1>() + radius}
        }),
        std::back_inserter(results));

    double sqaured_distance = std::numeric_limits<double>::infinity();

    for (const auto &value : results) {
        const SegmentPair &segment = value.second;
        const double dist = SquaredDistanceToLineSegment(p,
                                                         segment.first,
                                                         segment.second);
        if (dist < sqaured_distance && dist < squared_radius) {
            sqaured_distance = dist;
        }
    }

    return std::sqrt(sqaured_distance);
}

void RenderSDF(FT_Face ft_face,
               Glyph &glyph) {
    if (FT_Load_Glyph (ft_face, glyph.glyph_index, FT_LOAD_NO_HINTING)) {
        return;
    }

    FT_Glyph ft_glyph = nullptr;
    ft_glyph_guard glyph_guard(&ft_glyph);
    if (FT_Get_Glyph(ft_face->glyph, &ft_glyph)) return;

    glyph.metrics.advance = ft_face->glyph->metrics.horiAdvance / 64;

    FT_Outline_Funcs func_interface = {
        .move_to = &MoveTo,
        .line_to = &LineTo,
        .conic_to = &ConicTo,
        .cubic_to = &CubicTo,
        .shift = 0,
        .delta = 0
    };

    User user;

    // Decompose outline into bezier curves and line segments
    FT_Outline outline = ((FT_OutlineGlyph)ft_glyph)->outline;
    if (FT_Outline_Decompose(&outline, &func_interface, &user)) return;

    if (!user.ring.empty()) {
        CloseRing(user.ring);
        user.rings.push_back(user.ring);
    }

    if (user.rings.empty()) return;

    // Calculate the real glyph bbox.
    double bbox_xmin = std::numeric_limits<double>::infinity(),
           bbox_ymin = std::numeric_limits<double>::infinity();

    double bbox_xmax = -std::numeric_limits<double>::infinity(),
           bbox_ymax = -std::numeric_limits<double>::infinity();

    for (const Points &ring : user.rings) {
        for (const Point &point : ring) {
            if (point.get<0>() > bbox_xmax) bbox_xmax = point.get<0>();
            if (point.get<0>() < bbox_xmin) bbox_xmin = point.get<0>();
            if (point.get<1>() > bbox_ymax) bbox_ymax = point.get<1>();
            if (point.get<1>() < bbox_ymin) bbox_ymin = point.get<1>();
        }
    }

    bbox_xmin = std::round(bbox_xmin);
    bbox_ymin = std::round(bbox_ymin);
    bbox_xmax = std::round(bbox_xmax);
    bbox_ymax = std::round(bbox_ymax);

    // Offset so that glyph outlines are in the bounding box.
    for (Points &ring : user.rings) {
        for (Point &point : ring) {
            point.set<0>(point.get<0>() + -bbox_xmin + buffer_size);
            point.set<1>(point.get<1>() + -bbox_ymin + buffer_size);
        }
    }

    if (bbox_xmax - bbox_xmin == 0 || bbox_ymax - bbox_ymin == 0) return;

    glyph.metrics.x_bearing = bbox_xmin;
    glyph.metrics.y_bearing = bbox_ymax;
    glyph.metrics.width = bbox_xmax - bbox_xmin;
    glyph.metrics.height = bbox_ymax - bbox_ymin;

    Tree tree;

    for (const Points &ring : user.rings) {
        auto p1 = ring.begin();
        auto p2 = p1 + 1;

        for (; p2 != ring.end(); p1++, p2++) {
            const int segment_x1 = std::min(p1->get<0>(), p2->get<0>());
            const int segment_x2 = std::max(p1->get<0>(), p2->get<0>());
            const int segment_y1 = std::min(p1->get<1>(), p2->get<1>());
            const int segment_y2 = std::max(p1->get<1>(), p2->get<1>());

            tree.insert(SegmentValue {
                Box {
                    Point {segment_x1, segment_y1},
                    Point {segment_x2, segment_y2}
                },
                SegmentPair {
                    Point {p1->get<0>(), p1->get<1>()},
                    Point {p2->get<0>(), p2->get<1>()}
                }
            });
        }
    }

    // Loop over every pixel and determine the positive/negative distance to the outline.
    unsigned int buffered_width = glyph.metrics.width + 2 * buffer_size;
    unsigned int buffered_height = glyph.metrics.height + 2 * buffer_size;
    unsigned int bitmap_size = buffered_width * buffered_height;
    glyph.bitmap.resize(bitmap_size);

    for (unsigned int y = 0; y < buffered_height; y++) {
        for (unsigned int x = 0; x < buffered_width; x++) {
            unsigned int ypos = buffered_height - y - 1;
            unsigned int i = ypos * buffered_width + x;

            double d = MinDistanceToLineSegment(tree, Point {x + offset_size, y + offset_size}, radius_size) * (256 / radius_size);

            // Invert if point is inside.
            const bool inside = PolyContainsPoint(user.rings, Point {x + offset_size, y + offset_size});
            if (inside) {
                d = -d;
            }

            // Shift the 0 so that we can fit a few negative values
            // into our 8 bits.
            d += cutoff_size * 256;

            // Clamp to 0-255 to prevent overflows or underflows.
            int n = d > 255 ? 255 : d;
            n = n < 0 ? 0 : n;
            n = ((255 - n) / granularity) * granularity;

            glyph.bitmap[i] = static_cast<char>(n);
        }
    }
}

} // ns node_fontnik
