#include <emmintrin.h>
#include <smmintrin.h>
#include <Windows.h>
#include "filter.h"
#define ENABLE_FUNC_BASE
#include "afs_analyze_simd.h"

// arch:AVXでコンパイルすることで、128bit-AVX命令を生成
// ごくわずかだが高速


void __stdcall afs_analyze_set_threshold_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
	afs_analyze_set_threshold_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_12_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_12_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h, AVX|SSE41|SSE3|SSE2);
}
void __stdcall afs_analyze_1_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_1_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h, AVX|SSE41|SSE3|SSE2);
}
void __stdcall afs_analyze_2_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_2_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h, AVX|SSE41|SSE3|SSE2);
}
