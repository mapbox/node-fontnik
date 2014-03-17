#include "freetype_font.h"

v8::Persistent<v8::Function> FT_Font::constructor;

FT_Font::FT_Font() {
}

FT_Font::~FT_Font() {
    FT_Done_Face(face);
}

void FT_Font::Init() {
    // Prepare constructor template
    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(New);
    Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
    tpl->SetClassName(String::NewSymbol("FT_Font"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Prototype
    tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("hasInstance"),
        FunctionTemplate::New(HasInstance)->GetFunction());
    tpl->PrototypeTemplate()->Set(v8::String::NewSymbol("getFamilyName"),
        FunctionTemplate::New(HasInstance)->GetFunction());
    tpl->PrototypeTemplate()->SetAccessor(v8::String::NewSymbol("metrics"), FunctionTemplate::New(Metrics)->GetFunction());
    tpl->PrototypeTemplate()->SetIndexedPropertyHandler(GetGlyph);


    constructor = v8::Persistent<v8::Function>::New(tpl->GetFunction());
}

v8::Handle<v8::Value> FT_Font::New(const v8::Arguments& args) {
    if (!args.IsConstructCall()) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }
    if (args.Length() != 1) {
        return ThrowException(v8::Exception::TypeError(v8::String::New("Constructor must be called with new keyword")));
    }

    if (args[0]->IsExternal()) {
        face = (FT_Face)v8::External::Cast(*args[0])->Value();
    } else {
        v8::String::Utf8Value font_name(args[0]->ToString());
        /*
        PangoFontDescription *desc = pango_font_description_from_string(*font_name);
        pango_font = pango_font_map_load_font(pango_fontmap(), pango_context(), desc);
        */
    }

    if (face) {
        FT_Font* font = new FT_Font(face);
        font->Wrap(args.This());
        args.This()->Set(v8::String::NewSymbol("family"), v8::String::New(face->family_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("style"), v8::String::New(face->style_name), v8::ReadOnly);
        args.This()->Set(v8::String::NewSymbol("length"), v8::Number::New(face->num_glyphs), v8::ReadOnly);

        return args.This();
    } else {
        return ThrowException(v8::Exception::Error(v8::String::New("No matching font found")));
    }
}

v8::Handle<v8::Value> FT_Font::NewInstance(const v8::Arguments& args) {
    v8::HandleScope scope;

    const unsigned argc = 1;
    v8::Handle<v8::Value> argv[argc] = { args[0] };
    v8::Local<v8::Object> instance = constructor->NewInstance(argc, argv);

    return scope.Close(instance);
}

bool FT_Font::HasInstance(v8::Handle<v8::Value> value) {
    if (!value->IsObject()) return false;
    return constructor->HasInstance(value->ToObject());
}

v8::Handle<v8::Value> FT_Font::GetFamilyName(const v8::Arguments& args) {
    v8::HandleScope scope;

    FT_Font font = node::ObjectWrap::Unwrap<FT_Font>(args.This());

    return scope.Close(v8::String::New(font->face->family_name)); 
}

v8::Handle<v8::Value> FT_Font::GetGlyph(uint32_t glyph_index, const v8::AccessorInfo& info)
{
    v8::HandleScope scope;
    FT_Font* font = node::ObjectWrap::Unwrap<FT_Font>(info.This());
    FT_Face* face = font->face;
    // fprintf(stderr, "x/y/h: %ld/%ld/%ld\n", face->size->metrics.x_scale, face->size->metrics.y_scale / 2048, face->size->metrics.height);
    FT_Set_Char_Size(face, 0, size * 64, 72, 72);
    FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
    if (error) {
        FT_Done_Face(face);
        return Undefined();
    } else {
        v8::Local<v8::Object> result = v8::Object::New();
        result->Set(v8::String::NewSymbol("id"), v8::Uint32::New(glyph_index), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("width"), v8::Number::New(face->glyph->bitmap.width), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("height"), v8::Number::New(face->glyph->bitmap.rows), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("left"), v8::Number::New(face->glyph->bitmap_left), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("top"), v8::Number::New(face->glyph->bitmap_top), v8::ReadOnly);
        result->Set(v8::String::NewSymbol("advance"), v8::Number::New(face->glyph->metrics.horiAdvance), v8::ReadOnly);

        FT_GlyphSlot slot = face->glyph;
        int width = slot->bitmap.width;
        int height = slot->bitmap.rows;

        // Create a signed distance field for the glyph bitmap.
        if (width > 0) {
            unsigned int buffered_width = width + 2 * buffer;
            unsigned int buffered_height = height + 2 * buffer;

            unsigned char *distance = make_distance_map((unsigned char *)slot->bitmap.buffer, width, height, buffer);

            unsigned char *map = (unsigned char *)malloc(buffered_width * buffered_height);
            for (unsigned int y = 0; y < buffered_height; y++) {
                memcpy(map + buffered_width * y, distance + y * distmap_size, buffered_width);
            }
            free(distance);

            result->Set(v8::String::NewSymbol("bitmap"), node::Buffer::New((const char *)map, buffered_width * buffered_height)->handle_);
            free(map);
        }

        FT_Done_Face(face);
        return scope.Close(result);
    }
}

v8::Handle<v8::Value> FT_Font::Metrics(v8::Local<v8::String> property, const v8::AccessorInfo &info) {
    HandleScope scope;

    FT_Font* font = v8::ObjectWrap::Unwrap<FT_Font>(info.This());
    FT_Face* face = font->face;

    FT_Error error = FT_Set_Char_Size(face, 0, size * 64, 72, 72);
    if (error) {
        return ThrowException(v8::Exception::Error(v8::String::New("Could not set char size")));
    }

    // Generate a list of all glyphs
    llmr::face metrics;
    metrics.set_family(face->family_name);
    metrics.set_style(face->style_name);

    for (int glyph_index = 0; glyph_index < face->num_glyphs; glyph_index++) {
        FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_HINTING | FT_LOAD_RENDER);
        if (error) {
            return ThrowException(v8::Exception::Error(v8::String::New("Could not load glyph")));
        }

        llmr::glyph *glyph = metrics.add_glyphs();
        glyph->set_id(glyph_index);
        glyph->set_width((unsigned int)face->glyph->bitmap.width);
        glyph->set_height((unsigned int)face->glyph->bitmap.rows);
        glyph->set_left((unsigned int)face->glyph->bitmap_left);
        glyph->set_top((unsigned int)face->glyph->bitmap_top);
        glyph->set_advance(face->glyph->metrics.horiAdvance / 64);
    }

    FT_Done_Face(face);

    std::string serialized = metrics.SerializeAsString();
    return scope.Close(node::Buffer::New(serialized.data(), serialized.length())->handle_);
}
