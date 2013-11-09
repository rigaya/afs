#include <emmintrin.h>
#include <smmintrin.h>
#include <Windows.h>
#include "filter.h"

#define ENABLE_FUNC_BASE
#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_avx(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height) {
	afs_yuy2up_frame_simd(dst, src, width, pitch, height, AVX|SSE41|SSSE3|SSE2);
}
