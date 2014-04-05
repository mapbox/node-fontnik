#include "globals.hpp"
#include <pthread.h>

pthread_once_t ft_init = PTHREAD_ONCE_INIT;
pthread_key_t ft_fontmap_key;

void ft_init_keys()
{
    // TODO: destructors
    pthread_key_create(&ft_fontmap_key, NULL);
}

font_face_set *ft_fontmap() {
    pthread_once(&ft_init, ft_init_keys);

    font_face_set *fontmap = (font_face_set *)pthread_getspecific(ft_fontmap_key);
    if (fontmap == NULL) {
        *fontmap = font_face_set();
        pthread_setspecific(ft_fontmap_key, fontmap);
    }

    return fontmap;
}
