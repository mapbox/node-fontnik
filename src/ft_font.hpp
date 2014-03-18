#pragma once

#include <node.h>

// stl
#include <string>
#include <map>
#include <vector>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
}

struct FT_LibraryRec_;

class FT_Font : public node::ObjectWrap {
public:
    static void Init(v8::Handle<v8::Object> target);
    static bool HasInstance(v8::Handle<v8::Value> value);

    static v8::Handle<v8::Value> New(FT_Face ft_face);

    static std::vector<std::string> face_names();
    static std::map<std::string,std::pair<int,std::string> > const& get_mapping();
private:
    explicit FT_Font(FT_Face ft_face);
    ~FT_Font();

    static v8::Persistent<v8::FunctionTemplate> constructor;
    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Handle<v8::Value> GetGlyph(uint32_t, const v8::AccessorInfo& info);
    static v8::Handle<v8::Value> Metrics(v8::Local<v8::String> property, const v8::AccessorInfo &info);

    static const int size;
    static const int buffer;
    static FT_LibraryRec_ *library;

    static std::map<std::string, std::pair<int,std::string> > name2file_;
    static std::map<std::string, std::string> memory_fonts_;

    FT_LibraryRec_ *library_;
    FT_Face ft_face;
};
