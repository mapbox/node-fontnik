#include <node.h>

// freetype2
extern "C"
{
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
}

class FT_Font : public node::ObjectWrap {
public:
    static void Init();
    static v8::Handle<v8::Value> NewInstance(const v8::Arguments& args);
    static bool HasInstance(v8::Handle<v8::Value> val);

private:
    explicit FT_Font(FT_Face ft_face);
    ~FT_Font();

    static v8::Handle<v8::Value> New(const v8::Arguments& args);
    static v8::Persistent<v8::FunctionTemplate> constructor;
};
