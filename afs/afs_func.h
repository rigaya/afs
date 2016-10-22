#pragma once

#include "filter.h"
#include "afs.h"

typedef void (__stdcall *func_analyze_set_threshold)(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
typedef void (__stdcall *func_analyze)(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
typedef void (__stdcall *func_analyze_shrink_info)(unsigned char *dst, PIXEL_YC *src, int h, int width, int si_pitch);
typedef void (__stdcall *func_analyzemap_filter)(unsigned char* sip, int si_w, int w, int h);
typedef void (__stdcall *func_merge_scan)(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);
typedef void (__stdcall *func_yuy2up)(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
typedef void (__stdcall *func_blend)(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
typedef void (__stdcall *func_mie_spot)(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
typedef void (__stdcall *func_mie_inter)(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
typedef void (__stdcall *func_deint4)(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
typedef void (__stdcall *func_copy_line)(void *dst, void *src1, int w, int src_frame_pixels);
typedef void (__stdcall *func_get_stripe_count)(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);
typedef void (__stdcall *func_get_motion_count)(int *count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);

typedef struct {
    int                        align_minus_one;
    int                        min_cycle;
    int                        max_block_size;
    BOOL                       mc_count;
    func_analyze_set_threshold set_threshold;
    func_analyze_shrink_info   shrink_info;
    func_analyze               analyze_main[3];
} AFS_FUNC_ANALYZE;

typedef struct {
    func_get_stripe_count      stripe;
    func_get_motion_count      motion;
} AFS_FUNC_GET_COUNT;

typedef struct {
    //afs
    func_blend                 blend[4];
    func_mie_spot              mie_spot[4];
    func_mie_inter             mie_inter[4];
    func_deint4                deint4[4];
    func_copy_line             copy_line[4];
    //analyze
    AFS_FUNC_ANALYZE           analyze[2];
    //filter
    func_analyzemap_filter     analyzemap_filter;
    func_merge_scan            merge_scan;
    //yuy2up
    func_yuy2up                yuy2up[4];
    //get count
    AFS_FUNC_GET_COUNT         get_count;
    //simd avail
    DWORD                      simd_avail;
} AFS_FUNC;

void get_afs_func_list(AFS_FUNC *func_list);

void __stdcall afs_blend_sse2(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_sse4_1(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_avx(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_avx2(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);

void __stdcall afs_blend_nv16up_sse2(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16_sse2(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16up_sse4_1(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16_sse4_1(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16up_ssse3(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16_ssse3(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16up_avx(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_blend_nv16_avx(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);

void __stdcall afs_mie_spot_sse2(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_avx(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_avx2(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);

void __stdcall afs_mie_spot_nv16up_avx(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16_avx(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16up_sse4_1(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16_sse4_1(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16up_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16up_sse2(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);
void __stdcall afs_mie_spot_nv16_sse2(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels);

void __stdcall afs_mie_inter_sse2(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_avx(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_avx2(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);

void __stdcall afs_mie_inter_nv16up_avx(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16_avx(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16up_sse4_1(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16_sse4_1(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16up_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16_ssse3(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16up_sse2(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);
void __stdcall afs_mie_inter_nv16_sse2(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels);

void __stdcall afs_deint4_sse2(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_sse4_1(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_avx(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_avx2(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);

void __stdcall afs_deint4_nv16up_sse2(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16_sse2(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16up_sse4_1(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16_sse4_1(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16up_ssse3(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16_ssse3(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16up_avx(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);
void __stdcall afs_deint4_nv16_avx(void *dst, void *src1, void *src3, void *src4, void *src5, void *src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels);

void __stdcall afs_convert_nv16_yc48up_sse2(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48_sse2(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48up_ssse3(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48_ssse3(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48up_sse4_1(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48_sse4_1(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48up_avx(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_convert_nv16_yc48_avx(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_copy_yc48_line_sse(void *dst, void *src1, int w, int src_frame_pixels);
void __stdcall afs_copy_yc48_line_avx2(void *dst, void *src1, int w, int src_frame_pixels);

void __stdcall afs_get_stripe_count(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_stripe_count_sse2(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_stripe_count_sse2_popcnt(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_stripe_count_avx(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_stripe_count_avx2(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);

void __stdcall afs_get_motion_count(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_motion_count_sse2(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_motion_count_sse2_popcnt(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_motion_count_avx(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);
void __stdcall afs_get_motion_count_avx2(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);

void __stdcall afs_analyzemap_filter_sse2(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_sse2_plus(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_ssse3(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_ssse3_plus(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx_plus(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx2(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx2_plus(unsigned char* sip, int si_w, int w, int h);

void __stdcall afs_merge_scan_sse2(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);
void __stdcall afs_merge_scan_sse2_plus(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);
void __stdcall afs_merge_scan_avx(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);
void __stdcall afs_merge_scan_avx_plus(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);
void __stdcall afs_merge_scan_avx2(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);
void __stdcall afs_merge_scan_avx2_plus(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h, int x_start, int x_fin);

void __stdcall afs_analyze_set_threshold_sse2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_ssse3(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_sse4_1(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_sse4_1_popcnt(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_avx2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);

void __stdcall afs_analyze_set_threshold_nv16_sse2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_nv16_ssse3(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_nv16_sse4_1(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_nv16_sse4_1_popcnt(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_nv16_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_nv16_avx2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);

void __stdcall afs_analyze_12_sse2_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_ssse3_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_sse4_1_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_sse4_1_popcnt_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_avx_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_avx2_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_1_sse2_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_1_ssse3_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_1_sse4_1_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_1_sse4_1_popcnt_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_1_avx_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_1_avx2_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_2_sse2_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_2_ssse3_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_2_sse4_1_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_2_sse4_1_popcnt_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_2_avx_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_2_avx2_plus2(unsigned char *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);

void __stdcall afs_analyze_12_nv16_sse2_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_nv16_ssse3_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_nv16_sse4_1_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_nv16_sse4_1_popcnt_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_nv16_avx_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);
void __stdcall afs_analyze_12_nv16_avx2_plus2(BYTE *dst, void *p0, void *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip);

void __stdcall afs_yuy2up_frame_sse2(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_yuy2up_frame_sse4_1(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_yuy2up_frame_avx(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_yuy2up_frame_avx2(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);

void __stdcall afs_convert_yc48_to_nv16_sse2(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_convert_yc48_to_nv16_ssse3(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_convert_yc48_to_nv16_sse4_1(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_convert_yc48_to_nv16_avx(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
void __stdcall afs_convert_yc48_to_nv16_avx2(void *pixel, int dst_pitch, int dst_frame_pixels, const void *src, int width, int src_pitch, int y_start, int y_fin);
