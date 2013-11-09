#include <emmintrin.h>
#include <smmintrin.h>
#include <Windows.h>
#include "filter.h"
#define ENABLE_FUNC_BASE
#define USE_SSSE3 1
#define USE_SSE41 1

// arch:AVXでコンパイルすることで、128bit-AVX命令を生成
// ごくわずかだが高速

#include "afs_analyze_simd.h"

void __stdcall afs_analyze_set_threshold_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
	afs_analyze_set_threshold_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_12_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_12_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h);
}
void __stdcall afs_analyze_1_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_1_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h);
}
void __stdcall afs_analyze_2_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_2_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h);
}

#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_avx(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height) {
	afs_yuy2up_frame_simd(dst, src, width, pitch, height);
}

#include "afs_filter_simd.h"

void __stdcall afs_merge_scan_avx(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	afs_merge_scan_simd(dst, src0, src1, si_w, h);
}

void __stdcall afs_merge_scan_avx_plus(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	afs_merge_scan_simd_plus(dst, src0, src1, si_w, h);
}

void __stdcall afs_analyzemap_filter_avx(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd(sip, si_w, w, h, AVX | SSE41 | SSSE3 | SSE2);
}

void __stdcall afs_analyzemap_filter_avx_plus(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd_plus(sip, si_w, w, h, AVX | SSE41 | SSSE3 | SSE2);
}
