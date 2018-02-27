#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "filter.h"
#define USE_SSSE3 1
#define USE_SSE41 1
#define USE_POPCNT 1
#include "simd_util.h"
#include "afs_convert_const.h"

// arch:AVXでコンパイルすることで、128bit-AVX命令を生成
// ごくわずかだが高速

#include "afs_analyze_simd.h"

#if _MSC_VER >= 1800 && !defined(__AVX__) && !defined(_DEBUG)
static_assert(false, "do not forget to set /arch:AVX or /arch:AVX2 for this file.");
#endif

void __stdcall afs_analyze_set_threshold_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_set_threshold_nv16_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_nv16_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_12_avx_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, motion_count, mc_clip);
}
void __stdcall afs_analyze_1_avx_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_1_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, motion_count, mc_clip);
}
void __stdcall afs_analyze_2_avx_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_2_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, motion_count, mc_clip);
}

void __stdcall afs_analyze_12_nv16_avx_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_nv16_simd_plus2(dst, (BYTE *)p0, (BYTE *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, h_max, motion_count, mc_clip);
}

#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_avx(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin) {
    afs_yuy2up_frame_simd((PIXEL_YC *)pixel, dst_pitch, (PIXEL_YC *)src, width, src_pitch, y_start, y_fin);
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

void __stdcall afs_blend_avx(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_simd((PIXEL_YC *)dst, (PIXEL_YC *)src1, (PIXEL_YC *)src2, (PIXEL_YC *)src3, sip, mask, w);
}

void __stdcall afs_mie_spot_avx(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_simd((PIXEL_YC *)dst, (PIXEL_YC *)src1, (PIXEL_YC *)src2, (PIXEL_YC *)src3, (PIXEL_YC *)src4, (PIXEL_YC *)src_spot, w);
}

void __stdcall afs_mie_inter_avx(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_simd((PIXEL_YC *)dst, (PIXEL_YC *)src1, (PIXEL_YC *)src2, (PIXEL_YC *)src3, (PIXEL_YC *)src4, w);
}

void __stdcall afs_deint4_avx(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_simd((PIXEL_YC *)dst, (PIXEL_YC *)src1, (PIXEL_YC *)src3, (PIXEL_YC *)src4, (PIXEL_YC *)src5, (PIXEL_YC *)src7, sip, mask, w);
}

void __stdcall afs_blend_nv16up_avx(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, sip, mask, w, src_frame_pixels);
}

void __stdcall afs_blend_nv16_avx(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, sip, mask, w, src_frame_pixels);
}

void __stdcall afs_mie_spot_nv16up_avx(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src_spot, w, src_frame_pixels);
}

void __stdcall afs_mie_spot_nv16_avx(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src_spot, w, src_frame_pixels);
}

void __stdcall afs_mie_inter_nv16up_avx(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, w, src_frame_pixels);
}

void __stdcall afs_mie_inter_nv16_avx(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, w, src_frame_pixels);
}

void __stdcall afs_deint4_nv16up_avx(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src5, (uint8_t *)src7, (uint8_t *)sip, mask, w, src_frame_pixels);
}

void __stdcall afs_deint4_nv16_avx(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src5, (uint8_t *)src7, (uint8_t *)sip, mask, w, src_frame_pixels);
}

void __stdcall afs_get_stripe_count_avx(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
    afs_get_stripe_count_simd(count, sp0, sp1, sp, si_w, scan_w, scan_h);
}

void __stdcall afs_get_motion_count_avx(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
    afs_get_motion_count_simd(motion_count, sp, si_w, scan_w, scan_h);
}

#include "afs_convert_csp.h"

void __stdcall afs_convert_yc48_to_nv16_avx(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin) {
    afs_convert_yc48_to_nv16_simd(pixel, dst_pitch, dst_frame_pixels, (PIXEL_YC *)src, width, src_pitch, y_start, y_fin);
}

#include "afs_simd.h"

void __stdcall afs_convert_nv16_yc48up_avx(void *dst, void *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48up_simd((PIXEL_YC *)dst, (uint8_t *)src1, w, src_frame_pixels);
}

void __stdcall afs_convert_nv16_yc48_avx(void *dst, void *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48_simd((PIXEL_YC *)dst, (uint8_t *)src1, w, src_frame_pixels);
}
