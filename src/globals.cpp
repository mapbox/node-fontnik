#include "globals.hpp"
#include <pthread.h>

pthread_once_t pango_init = PTHREAD_ONCE_INIT;
pthread_key_t pango_fontmap_key;
pthread_key_t pango_context_key;
pthread_key_t pango_renderer_key;

void pango_init_keys()
{
    // TODO: destructors
    pthread_key_create(&pango_fontmap_key, NULL);
    pthread_key_create(&pango_context_key, NULL);
    pthread_key_create(&pango_renderer_key, NULL);
}

PangoFontMap *pango_fontmap()
{
    pthread_once(&pango_init, pango_init_keys);

    PangoFontMap *fontmap = (PangoFontMap *)pthread_getspecific(pango_fontmap_key);
    if (fontmap == NULL) {
        fontmap = pango_ft2_font_map_new();
        pthread_setspecific(pango_fontmap_key, fontmap);
    }

    return fontmap;
}

PangoContext *pango_context()
{
    pthread_once(&pango_init, pango_init_keys);

    PangoContext *context = (PangoContext *)pthread_getspecific(pango_context_key);
    if (context == NULL) {
        context = pango_font_map_create_context(pango_fontmap());
        pthread_setspecific(pango_context_key, context);
    }

    return context;
}


PangoRenderer *pango_renderer()
{
    pthread_once(&pango_init, pango_init_keys);

    PangoRenderer *renderer = (PangoRenderer *)pthread_getspecific(pango_renderer_key);
    if (renderer == NULL) {
        renderer = pango_sdf_get_renderer();
        pthread_setspecific(pango_renderer_key, renderer);
    }

    return renderer;
}

