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
    static v8::Persistent<v8::FunctionTemplate> constructor;
    static bool HasInstance(v8::Handle<v8::Value> val);

    static FT_Font* New(FT_Face ft_face);
protected:
    FT_Font(FT_Face ft_face);
    ~FT_Font();
};
