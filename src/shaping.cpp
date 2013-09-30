#include "shaping.hpp"
#include "font.hpp"

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

using namespace v8;

// Returns the Nth number in the fibonacci sequence where N is the first
// argument passed.
Handle<Value> Shaping(const Arguments& args) {
    HandleScope scope;

    if (args.Length() < 1) {
        return ThrowException(Exception::TypeError(String::New("First argument must be the string to shape")));
    }

    if (args.Length() < 2 || !Font::HasInstance(args[1])) {
        return ThrowException(Exception::TypeError(String::New("Second argument must be a font object")));
    }

    String::Value text(args[0]->ToString());
    Font* font = node::ObjectWrap::Unwrap<Font>(args[1]->ToObject());
    unsigned int length = text.length();


    Local<Array> unshaped = Array::New();
    for (unsigned i = 0; i < length; i++) {
        FT_UInt glyph_index = FT_Get_Char_Index(font->face, (*text)[i]);
        unshaped->Set(i, Uint32::New(glyph_index));
    }


    hb_font_t *hb_font = hb_ft_font_create(font->face, NULL);

    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_reset(buffer);
    hb_buffer_set_direction(buffer, HB_DIRECTION_RTL);
    hb_buffer_set_script(buffer, HB_SCRIPT_ARABIC);
    hb_buffer_set_language(buffer, hb_language_from_string("ar", 2));
    hb_buffer_add_utf16(buffer, *text, length, 0, length);


    hb_shape(hb_font, buffer, NULL, 0);

    unsigned num_glyphs = hb_buffer_get_length(buffer);
    hb_glyph_info_t *glyphs = hb_buffer_get_glyph_infos(buffer, NULL);
    hb_glyph_position_t *positions = hb_buffer_get_glyph_positions(buffer, NULL);

    Local<Array> shaped = Array::New();
    for (unsigned i = 0; i < num_glyphs; i++) {
        hb_glyph_info_t *glyph = glyphs + i;
        hb_glyph_position_t *pos = positions + i;
        shaped->Set(i, Uint32::New(glyph->codepoint));
    }


    Local<Object> result = Object::New();
    result->Set(String::NewSymbol("unshaped"), unshaped);
    result->Set(String::NewSymbol("shaped"), shaped);
    return scope.Close(result);
}