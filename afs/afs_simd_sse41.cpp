#include <Windows.h>
#include "filter.h"
#define ENABLE_FUNC_BASE
#define USE_SSSE3 1
#define USE_SSE41 1

#include "afs_analyze_simd.h"

void __stdcall afs_analyze_12_sse4_1_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_12_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h);
}
void __stdcall afs_analyze_1_sse4_1_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_1_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h);
}
void __stdcall afs_analyze_2_sse4_1_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_2_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h);
}

#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_sse4_1(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height) {
	afs_yuy2up_frame_simd(dst, src, width, pitch, height);
}

#include "afs_simd.h"

void __stdcall afs_blend_sse4_1(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
	afs_blend_simd(dst, src1, src2, src3, sip, mask, w);
}

void __stdcall afs_deint4_sse4_1(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
	afs_deint4_simd(dst, src1, src3, src4, src5, src7, sip, mask, w);
}
