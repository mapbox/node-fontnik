// fontnik
#include <node_fontnik/glyphs.hpp>

// node
#include <node_buffer.h>
#include <nan.h>

#include "agg_curves.h"

// boost
// undef B0 to workaround https://svn.boost.org/trac/boost/ticket/10467
#undef B0
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>

// stl
#include <unordered_map>

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

namespace node_fontnik
{

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
};

struct LoadBaton {
    v8::Persistent<v8::Function> callback;
    v8::Persistent<v8::Object> buffer;
    const char * font_data;
    std::size_t font_size;
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
            NanAssignPersistent(callback, cb.As<v8::Function>());
            NanAssignPersistent(buffer, buf.As<v8::Object>());
        }
    ~LoadBaton() {
        NanDisposePersistent(callback);
        NanDisposePersistent(buffer);
    }
};

struct RangeBaton {
    v8::Persistent<v8::Function> callback;
    v8::Persistent<v8::Object> buffer;
    const char * font_data;
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
            NanAssignPersistent(callback, cb.As<v8::Function>());
            NanAssignPersistent(buffer, buf.As<v8::Object>());
        }
    ~RangeBaton() {
        NanDisposePersistent(callback);
        NanDisposePersistent(buffer);
    }
};

NAN_METHOD(Load) {
    NanScope();

    // Validate arguments.
    if (!args[0]->IsObject()) {
        return NanThrowTypeError("First argument must be a font buffer");
    }
    v8::Local<v8::Object> obj = args[0]->ToObject();
    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return NanThrowTypeError("First argument must be a font buffer");
    }

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return NanThrowTypeError("Callback must be a function");
    }

    LoadBaton* baton = new LoadBaton(obj,args[1]);
    uv_queue_work(uv_default_loop(), &baton->request, LoadAsync, (uv_after_work_cb)AfterLoad);
    NanReturnUndefined();
}

NAN_METHOD(Range) {
    NanScope();

    // Validate arguments.
    if (args.Length() < 1 || !args[0]->IsObject()) {
        return NanThrowTypeError("First argument must be an object of options");
    }

    v8::Local<v8::Object> options = args[0].As<v8::Object>();
    v8::Local<v8::Value> font_buffer = options->Get(NanNew<v8::String>("font"));
    if (!font_buffer->IsObject()) {
        return NanThrowTypeError("Font buffer is not an object");
    }
    v8::Local<v8::Object> obj = font_buffer->ToObject();
    v8::Local<v8::Value> start = options->Get(NanNew<v8::String>("start"));
    v8::Local<v8::Value> end = options->Get(NanNew<v8::String>("end"));

    if (obj->IsNull() || obj->IsUndefined() || !node::Buffer::HasInstance(obj)) {
        return NanThrowTypeError("First argument must be a font buffer");
    }

    if (!start->IsNumber() || start->IntegerValue() < 0) {
        return NanThrowTypeError("option `start` must be a number from 0-65535");
    }

    if (!end->IsNumber() || end->IntegerValue() > 65535) {
        return NanThrowTypeError("option `end` must be a number from 0-65535");
    }

    if (end->IntegerValue() < start->IntegerValue()) {
        return NanThrowTypeError("`start` must be less than or equal to `end`");
    }

    if (args.Length() < 2 || !args[1]->IsFunction()) {
        return NanThrowTypeError("Callback must be a function");
    }

    RangeBaton* baton = new RangeBaton(obj,
                                       args[1],
                                       start->IntegerValue(),
                                       end->IntegerValue());
    uv_queue_work(uv_default_loop(), &baton->request, RangeAsync, (uv_after_work_cb)AfterRange);
    NanReturnUndefined();
}

struct ft_library_guard {
    ft_library_guard(FT_Library * lib) :
        library_(lib) {}

    ~ft_library_guard()
    {
        if (library_) FT_Done_FreeType(*library_);
    }

    FT_Library * library_;
};

struct ft_glyph_guard {
    ft_glyph_guard(FT_Glyph * glyph) :
        glyph_(glyph) {}

    ~ft_glyph_guard()
    {
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
        baton->error_name = std::string("could not open FreeType library");
        return;
        /* LCOV_EXCL_END */
    }
    FT_Face ft_face = 0;
    int num_faces = 0;
    for ( int i = 0; ft_face == 0 || i < num_faces; ++i )
    {
        FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
        if (face_error) {
            baton->error_name = std::string("could not open font file");
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
        baton->faces.emplace_back(ft_face->family_name, ft_face->style_name, std::move(points_vec));
        if (ft_face) {
            FT_Done_Face(ft_face);
        }
    }
};

void AfterLoad(uv_work_t* req) {
    NanScope();
    LoadBaton* baton = static_cast<LoadBaton*>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = { NanError(baton->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Array> js_faces = NanNew<v8::Array>();
        unsigned idx = 0;
        for (auto const& face : baton->faces) {
            v8::Local<v8::Object> js_face = NanNew<v8::Object>();
            js_face->Set(NanNew("family_name"),NanNew(face.family_name));
            js_face->Set(NanNew("style_name"),NanNew(face.style_name));
            v8::Local<v8::Array> js_points = NanNew<v8::Array>(face.points.size());
            unsigned p_idx = 0;
            for (auto const& pt : face.points) {
                js_points->Set(p_idx++,NanNew(pt));
            }
            js_face->Set(NanNew("points"),js_points);
            js_faces->Set(idx++,js_face);
        }
        v8::Local<v8::Value> argv[2] = { NanNull(), js_faces };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(baton->callback), 2, argv);
    }
    delete baton;
};

void RangeAsync(uv_work_t* req) {
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    unsigned array_size = baton->end - baton->start;
    baton->chars.reserve(array_size);
    for (unsigned i=baton->start; i <= array_size; i++) {
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

    FT_Face ft_face = 0;

    llmr::glyphs::glyphs glyphs;

    int num_faces = 0;
    for ( int i = 0; ft_face == 0 || i < num_faces; ++i )
    {
        FT_Error face_error = FT_New_Memory_Face(library, reinterpret_cast<FT_Byte const*>(baton->font_data), static_cast<FT_Long>(baton->font_size), i, &ft_face);
        if (face_error) {
            baton->error_name = std::string("could not open font");
            return;
        }

        llmr::glyphs::fontstack *mutable_fontstack = glyphs.add_stacks();
        mutable_fontstack->set_name(std::string(ft_face->family_name) + " " + ft_face->style_name);
        mutable_fontstack->set_range(std::to_string(baton->start) + "-" + std::to_string(baton->end));

        const double scale_factor = 1.0;

        // Set character sizes.
        double size = 24 * scale_factor;
        FT_Set_Char_Size(ft_face,0,(FT_F26Dot6)(size * (1<<6)),0,0);

        for (std::vector<uint32_t>::size_type x = 0; x != baton->chars.size(); x++) {
            FT_ULong char_code = baton->chars[x];
            glyph_info glyph;

            // Get FreeType face from face_ptr.
            FT_UInt char_index = FT_Get_Char_Index(ft_face, char_code);

            if (!char_index) continue;

            glyph.glyph_index = char_index;
            RenderSDF(glyph, 24, 3, 0.25, ft_face);

            // Add glyph to fontstack.
            llmr::glyphs::glyph *mutable_glyph = mutable_fontstack->add_glyphs();
            mutable_glyph->set_id(char_code);
            mutable_glyph->set_width(glyph.width);
            mutable_glyph->set_height(glyph.height);
            mutable_glyph->set_left(glyph.left);
            mutable_glyph->set_top(glyph.top - glyph.ascender);
            mutable_glyph->set_advance(glyph.advance);

            if (glyph.width > 0) {
                mutable_glyph->set_bitmap(glyph.bitmap);
            }

        }
        if (ft_face) {
            FT_Done_Face(ft_face);
        }
    }

    baton->message = glyphs.SerializeAsString();
}

void AfterRange(uv_work_t* req) {
    NanScope();
    RangeBaton* baton = static_cast<RangeBaton*>(req->data);

    if (!baton->error_name.empty()) {
        v8::Local<v8::Value> argv[1] = { NanError(baton->error_name.c_str()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(baton->callback), 1, argv);
    } else {
        v8::Local<v8::Array> js_faces = NanNew<v8::Array>();
        unsigned idx = 0;
        v8::Local<v8::Value> argv[2] = { NanNull(), NanNewBufferHandle(baton->message.data(), baton->message.size()) };
        NanMakeCallback(NanGetCurrentContext()->Global(), NanNew(baton->callback), 2, argv);
    }

    delete baton;
};

struct User {
    Rings rings;
    Points ring;
};

void CloseRing(Points &ring)
{
    const Point &first = ring.front();
    const Point &last = ring.back();

    if (first.get<0>() != last.get<0>() ||
        first.get<1>() != last.get<1>())
    {
        ring.push_back(first);
    }
}

int MoveTo(const FT_Vector *to, void *ptr)
{
    User *user = (User*)ptr;
    if (!user->ring.empty()) {
        CloseRing(user->ring);
        user->rings.push_back(user->ring);
        user->ring.clear();
    }
    user->ring.push_back(Point { float(to->x) / 64, float(to->y) / 64 });
    return 0;
}

int LineTo(const FT_Vector *to, void *ptr)
{
    User *user = (User*)ptr;
    user->ring.push_back(Point { float(to->x) / 64, float(to->y) / 64 });
    return 0;
}

int ConicTo(const FT_Vector *control,
            const FT_Vector *to,
            void *ptr)
{
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
            void *ptr)
{
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
bool PolyContainsPoint(const Rings &rings, const Point &p)
{
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

double SquaredDistance(const Point &v, const Point &w)
{
    const double a = v.get<0>() - w.get<0>();
    const double b = v.get<1>() - w.get<1>();
    return a * a + b * b;
}

Point ProjectPointOnLineSegment(const Point &p,
                                const Point &v,
                                const Point &w)
{
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
                                    const Point &w)
{
    const Point s = ProjectPointOnLineSegment(p, v, w);
    return SquaredDistance(p, s);
}

double MinDistanceToLineSegment(const Tree &tree,
                                const Point &p,
                                int radius)
{
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

void RenderSDF(glyph_info &glyph,
                     int size,
                     int buffer,
                     float cutoff,
                     FT_Face ft_face)
{

    if (FT_Load_Glyph (ft_face, glyph.glyph_index, FT_LOAD_NO_HINTING)) {
        return;
    }

    FT_Glyph ft_glyph = nullptr;
    ft_glyph_guard glyph_guard(&ft_glyph);
    if (FT_Get_Glyph(ft_face->glyph, &ft_glyph)) return;

    int advance = ft_face->glyph->metrics.horiAdvance / 64;
    int ascender = ft_face->size->metrics.ascender / 64;
    int descender = ft_face->size->metrics.descender / 64;

    glyph.line_height = ft_face->size->metrics.height;
    glyph.advance = advance;
    glyph.ascender = ascender;
    glyph.descender = descender;

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
            point.set<0>(point.get<0>() + -bbox_xmin + buffer);
            point.set<1>(point.get<1>() + -bbox_ymin + buffer);
        }
    }

    if (bbox_xmax - bbox_xmin == 0 || bbox_ymax - bbox_ymin == 0) return;

    glyph.left = bbox_xmin;
    glyph.top = bbox_ymax;
    glyph.width = bbox_xmax - bbox_xmin;
    glyph.height = bbox_ymax - bbox_ymin;

    Tree tree;
    float offset = 0.5;
    int radius = 8;

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
    unsigned int buffered_width = glyph.width + 2 * buffer;
    unsigned int buffered_height = glyph.height + 2 * buffer;
    unsigned int bitmap_size = buffered_width * buffered_height;
    glyph.bitmap.resize(bitmap_size);

    for (unsigned int y = 0; y < buffered_height; y++) {
        for (unsigned int x = 0; x < buffered_width; x++) {
            unsigned int ypos = buffered_height - y - 1;
            unsigned int i = ypos * buffered_width + x;

            double d = MinDistanceToLineSegment(tree, Point {x + offset, y + offset}, radius) * (256 / radius);

            // Invert if point is inside.
            const bool inside = PolyContainsPoint(user.rings, Point { x + offset, y + offset });
            if (inside) {
                d = -d;
            }

            // Shift the 0 so that we can fit a few negative values
            // into our 8 bits.
            d += cutoff * 256;

            // Clamp to 0-255 to prevent overflows or underflows.
            int n = d > 255 ? 255 : d;
            n = n < 0 ? 0 : n;

            glyph.bitmap[i] = static_cast<char>(255 - n);
        }
    }
}

} // ns node_fontnik
