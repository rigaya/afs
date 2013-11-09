﻿#include "filter.h"

#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall afs_yuy2up_mmx(PIXEL_YC *dst, PIXEL_YC *src, int n);
#ifdef __cplusplus
}
#endif

static void __stdcall afs_yuy2up_frame_mmx(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height) {
	for (int pos_y = 0; pos_y < height; pos_y++) {
		afs_yuy2up_mmx(dst, src, width);
		dst += pitch;
		src += pitch;
	}
}
