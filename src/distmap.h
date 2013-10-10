#ifndef DISTMAP_H
#define DISTMAP_H

#ifdef __cplusplus
extern "C" {
#endif

const int distmap_size = 72;

unsigned char *make_distance_map(unsigned char *img, unsigned int src_width, unsigned int src_height, int buffer);

#ifdef __cplusplus
}
#endif

#endif
