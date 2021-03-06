﻿#include "filter.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall afs_yuy2up_mmx(PIXEL_YC *dst, PIXEL_YC *src, int n);
#ifdef __cplusplus
}
#endif

static void __stdcall afs_yuy2up_frame_mmx(void *pixel, int dst_pitch, int dst_frame_pixels, const void *_src, int width, int src_pitch, int y_start, int y_fin) {
    PIXEL_YC *dst = (PIXEL_YC *)pixel;
    PIXEL_YC *src = (PIXEL_YC *)_src;
    for (int pos_y = y_start; pos_y < y_fin; pos_y++) {
        afs_yuy2up_mmx(dst, src, width);
        dst += dst_pitch;
        src += src_pitch;
    }
}
