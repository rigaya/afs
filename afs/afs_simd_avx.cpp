#include <Windows.h>
#include "filter.h"
#define USE_SSSE3 1
#define USE_SSE41 1
#define USE_POPCNT 1

// arch:AVXでコンパイルすることで、128bit-AVX命令を生成
// ごくわずかだが高速

#include "afs_analyze_simd.h"

#if _MSC_VER >= 1800 && !defined(__AVX__) && !defined(_DEBUG)
static_assert(false, "do not forget to set /arch:AVX or /arch:AVX2 for this file.");
#endif

void __stdcall afs_analyze_set_threshold_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_12_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h, motion_count, mc_clip);
}
void __stdcall afs_analyze_1_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_1_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h, motion_count, mc_clip);
}
void __stdcall afs_analyze_2_avx_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_2_simd_plus2(dst, p0, p1, tb_order, width, step, si_pitch, h, motion_count, mc_clip);
}

#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_avx(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int y_start, int y_fin) {
    afs_yuy2up_frame_simd(dst, src, width, pitch, y_start, y_fin);
}

#include "afs_filter_simd.h"

void __stdcall afs_merge_scan_avx(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h, int x_start, int x_fin) {
    afs_merge_scan_simd(dst, src0, src1, si_w, h, x_start, x_fin);
}

void __stdcall afs_merge_scan_avx_plus(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h, int x_start, int x_fin) {
    afs_merge_scan_simd_plus(dst, src0, src1, si_w, h, x_start, x_fin);
}

void __stdcall afs_analyzemap_filter_avx(BYTE* sip, int si_w, int w, int h) {
    afs_analyzemap_filter_simd(sip, si_w, w, h);
}

void __stdcall afs_analyzemap_filter_avx_plus(BYTE* sip, int si_w, int w, int h) {
    afs_analyzemap_filter_simd_plus(sip, si_w, w, h);
}

#include "afs_simd.h"

void __stdcall afs_blend_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
    afs_blend_simd(dst, src1, src2, src3, sip, mask, w);
}

void __stdcall afs_mie_spot_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot, int w) {
    afs_mie_spot_simd(dst, src1, src2, src3, src4, src_spot, w);
}

void __stdcall afs_mie_inter_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
    afs_mie_inter_simd(dst, src1, src2, src3, src4, w);
}

void __stdcall afs_deint4_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
    afs_deint4_simd(dst, src1, src3, src4, src5, src7, sip, mask, w);
}

void __stdcall afs_get_stripe_count_avx(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
    afs_get_stripe_count_simd(count, sp0, sp1, sp, si_w, scan_w, scan_h);
}

void __stdcall afs_get_motion_count_avx(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
    afs_get_motion_count_simd(motion_count, sp, si_w, scan_w, scan_h);
}
