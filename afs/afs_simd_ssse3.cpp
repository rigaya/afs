#include <Windows.h>
#include "filter.h"
#define USE_SSSE3 1
#define USE_SSE41 0

#include "afs_analyze_simd.h"

void __stdcall afs_analyze_set_threshold_ssse3(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_set_threshold_nv16_ssse3(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_nv16_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_12_ssse3_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h, motion_count, mc_clip);
}
void __stdcall afs_analyze_1_ssse3_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_1_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h, motion_count, mc_clip);
}
void __stdcall afs_analyze_2_ssse3_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_2_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h, motion_count, mc_clip);
}

void __stdcall afs_analyze_12_nv16_ssse3_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_nv16_simd_plus2(dst, (BYTE *)p0, (BYTE *)p1, tb_order, width, step, si_pitch, h, max_h, motion_count, mc_clip);
}

#include "afs_simd.h"

void __stdcall afs_blend_nv16up_ssse3(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, sip, mask, w, src_frame_pixels);
}

void __stdcall afs_blend_nv16_ssse3(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, sip, mask, w, src_frame_pixels);
}

void __stdcall afs_mie_spot_nv16up_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src_spot, w, src_frame_pixels);
}

void __stdcall afs_mie_spot_nv16_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src_spot, w, src_frame_pixels);
}

void __stdcall afs_mie_inter_nv16up_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, w, src_frame_pixels);
}

void __stdcall afs_mie_inter_nv16_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, w, src_frame_pixels);
}

void __stdcall afs_deint4_nv16up_ssse3(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_nv16up_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src5, (uint8_t *)src7, (uint8_t *)sip, mask, w, src_frame_pixels);
}

void __stdcall afs_deint4_nv16_ssse3(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_nv16_simd((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src5, (uint8_t *)src7, (uint8_t *)sip, mask, w, src_frame_pixels);
}

#include "afs_yuy2up_simd.h"

void __stdcall afs_yuy2up_frame_ssse3(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin) {
    afs_yuy2up_frame_simd((PIXEL_YC *)pixel, dst_pitch, (PIXEL_YC *)src, width, src_pitch, y_start, y_fin);
}

#include "afs_filter_simd.h"

void __stdcall afs_analyzemap_filter_ssse3(BYTE* sip, int si_w, int w, int h) {
    afs_analyzemap_filter_simd(sip, si_w, w, h);
}

void __stdcall afs_analyzemap_filter_ssse3_plus(BYTE* sip, int si_w, int w, int h) {
    afs_analyzemap_filter_simd_plus(sip, si_w, w, h);
}

#include "afs_convert_csp.h"

void __stdcall afs_convert_yc48_to_nv16_ssse3(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin) {
    afs_convert_yc48_to_nv16_simd(pixel, dst_pitch, dst_frame_pixels, (PIXEL_YC *)src, width, src_pitch, y_start, y_fin);
}

void __stdcall afs_convert_nv16_yc48up_ssse3(void *dst, void *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48up_simd((PIXEL_YC *)dst, (uint8_t *)src1, w, src_frame_pixels);
}

void __stdcall afs_convert_nv16_yc48_ssse3(void *dst, void *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48_simd((PIXEL_YC *)dst, (uint8_t *)src1, w, src_frame_pixels);
}
