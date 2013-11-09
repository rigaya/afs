#pragma once

#include "filter.h"
#include "afs.h"

typedef void (__stdcall *func_analyze_set_threshold)(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
typedef void (__stdcall *func_analyze)(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
typedef void (__stdcall *func_analyze_shrink_info)(unsigned char *dst, PIXEL_YC *src, int h, int width, int si_pitch);
typedef void (__stdcall *func_analyzemap_filter)(unsigned char* sip, int si_w, int w, int h);
typedef void (__stdcall *func_merge_scan)(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);
typedef void (__stdcall *func_yuy2up)(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int height);
typedef void (__stdcall *func_blend)(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, unsigned char *sip, unsigned int mask, int w);
typedef void (__stdcall *func_mie_spot)(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2,PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w);
typedef void (__stdcall *func_mie_inter)(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w);
typedef void (__stdcall *func_deint4)(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, unsigned char *sip, unsigned int mask, int w);
typedef void (__stdcall *func_get_stripe_count)(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h);
typedef void (__stdcall *func_get_motion_count)(int *count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h);

typedef struct {
	int                        align_minus_one;
	int                        min_cycle;
	int                        max_block_size;
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
	func_blend                 blend;
	func_mie_spot              mie_spot;
	func_mie_inter             mie_inter;
	func_deint4                deint4;
	//analyze
	AFS_FUNC_ANALYZE           analyze;
	//filter
	func_analyzemap_filter     analyzemap_filter;
	func_merge_scan            merge_scan;
	//yuy2up
	func_yuy2up                yuy2up;
	//get count
	AFS_FUNC_GET_COUNT         get_count;
} AFS_FUNC;

void get_afs_func_list(AFS_FUNC *func_list);
