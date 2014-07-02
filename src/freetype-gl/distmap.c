/* =========================================================================
 * Freetype GL - A C OpenGL Freetype engine
 * Platform:    Any
 * WWW:         http://code.google.com/p/freetype-gl/
 * -------------------------------------------------------------------------
 * Copyright 2011 Nicolas P. Rougier. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY NICOLAS P. ROUGIER ''AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL NICOLAS P. ROUGIER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Nicolas P. Rougier.
 * ========================================================================= */
#include <edtaa/edtaa4func.h>
#include <freetype-gl/distmap.h>

#include <stdlib.h>
#include <string.h>

unsigned char *
make_distance_map(unsigned char *img,
                  unsigned int src_width, unsigned int src_height, int buffer)
{
    int width = distmap_size;
    int height = distmap_size;

    short *xdist = (short *)  malloc(width * height * sizeof(short));
    short *ydist = (short *)  malloc(width * height * sizeof(short));
    double *gx      = (double *) malloc(width * height * sizeof(double));
    double *gy      = (double *) malloc(width * height * sizeof(double));
    double *data    = (double *) calloc(width * height, sizeof(double));
    double *outside = (double *) calloc(width * height, sizeof(double));
    double *inside  = (double *) calloc(width * height, sizeof(double));
    int i;

    // Convert img into double (data)
    double img_min = 255, img_max = -255;
    for (unsigned y = 0; y < src_height; y++) {
        for (unsigned x = 0; x < src_width; x++) {
            double v = img[y * src_width + x];
            data[width * (y + buffer) + buffer + x] = v;
            if (v > img_max) {
                img_max = v;
            }
            if (v < img_min) {
                img_min = v;
            }
        }
    }
    // Rescale image levels between 0 and 1
    for (unsigned y = 0; y < src_height; y++) {
        for (unsigned x = 0; x < src_width; x++) {
            data[width * (y + buffer) + buffer + x] = (img[y * src_width + x] - img_min) / img_max;
        }
    }


    // Compute outside = edtaa4(bitmap); % Transform background (0's)
    memset(gx, 0, sizeof(double)*width * height);
    memset(gy, 0, sizeof(double)*width * height);
    computegradient(data, width, height, gx, gy);
    edtaa4(data, gx, gy, height, width, xdist, ydist, outside);
    for (i = 0; i < width * height; ++i) {
        if (outside[i] < 0) {
            outside[i] = 0.0;
        }
    }

    // Compute inside = edtaa4(1-bitmap); % Transform foreground (1's)
    memset(gx, 0, sizeof(double)*width * height);
    memset(gy, 0, sizeof(double)*width * height);
    for (i = 0; i < width * height; ++i) {
        data[i] = 1 - data[i];
    }
    computegradient(data, width, height, gx, gy);
    edtaa4(data, gx, gy, height, width, xdist, ydist, inside);
    for (i = 0; i < width * height; ++i) {
        if (inside[i] < 0) {
            inside[i] = 0.0;
        }
    }

    // distmap = outside - inside; % Bipolar distance field
    unsigned char *out = (unsigned char *) malloc(width * height * sizeof(unsigned char));
    for (i = 0; i < width * height; ++i) {
        outside[i] -= inside[i];
        outside[i] = 64 + outside[i] * 32;
        if (outside[i] < 0) {
            outside[i] = 0;
        }
        if (outside[i] > 255) {
            outside[i] = 255;
        }
        out[i] = ((255 - (unsigned char) outside[i])/8) * 8;
        //out[i] = (unsigned char) outside[i];
    }

    free(xdist);
    free(ydist);
    free(gx);
    free(gy);
    free(data);
    free(outside);
    free(inside);
    return out;
}

