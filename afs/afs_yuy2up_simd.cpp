#include <emmintrin.h>
#include <Windows.h>
#include "filter.h"

#define ENABLE_FUNC_BASE
#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_sse2(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height) {
	afs_yuy2up_frame_simd(dst, src, width, pitch, height, SSE2);
}

void __stdcall afs_yuy2up_frame_sse4_1(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height) {
	afs_yuy2up_frame_simd(dst, src, width, pitch, height, SSE41|SSSE3|SSE2);
}
