#include <Windows.h>
#include "filter.h"
#define USE_SSSE3 1
#define USE_SSE41 1
#define USE_POPCNT 1

#include "afs_analyze_simd.h"

void __stdcall afs_analyze_set_threshold_sse4_1_popcnt(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_set_threshold_nv16_sse4_1_popcnt(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    afs_analyze_set_threshold_nv16_simd(thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
}

void __stdcall afs_analyze_12_sse4_1_popcnt_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, motion_count, mc_clip);
}
void __stdcall afs_analyze_1_sse4_1_popcnt_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_1_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, motion_count, mc_clip);
}
void __stdcall afs_analyze_2_sse4_1_popcnt_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_2_simd_plus2(dst, (PIXEL_YC *)p0, (PIXEL_YC *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, motion_count, mc_clip);
}

void __stdcall afs_analyze_12_nv16_sse4_1_popcnt_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    afs_analyze_12_nv16_simd_plus2(dst, (BYTE *)p0, (BYTE *)p1, tb_order, width, step, si_pitch, h_start, h_fin, height, h_max, motion_count, mc_clip);
}
