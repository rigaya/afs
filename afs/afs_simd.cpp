#include <emmintrin.h> //SSE2
#include <Windows.h>
#include "filter.h"
#define ENABLE_FUNC_BASE
#include "afs_simd.h"



void __stdcall afs_blend_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
	afs_blend_simd(dst, src1, src2, src3, sip, mask, w, SSE2);
}

void __stdcall afs_blend_sse4_1(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
	afs_blend_simd(dst, src1, src2, src3, sip, mask, w, SSE41|SSSE3|SSE2);
}

void __stdcall afs_mie_spot_sse2( PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w) {
	afs_mie_spot_simd(dst, src1, src2, src3, src4, src_spot, w);
}

void __stdcall afs_mie_inter_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
	afs_mie_inter_simd(dst, src1, src2, src3, src4, w);
}

void __stdcall afs_deint4_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
	afs_deint4_simd(dst, src1, src3, src4, src5, src7, sip, mask, w, SSE2);
}
void __stdcall afs_deint4_sse4_1(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
	afs_deint4_simd(dst, src1, src3, src4, src5, src7, sip, mask, w, SSE41|SSSE3|SSE2);
}

void __stdcall afs_get_stripe_count_sse2(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
	afs_get_stripe_count_simd(count, sp0, sp1, sp, si_w, scan_w, scan_h, SSE2);
}

void __stdcall afs_get_motion_count_sse2(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
	afs_get_motion_count_simd(motion_count, sp, si_w, scan_w, scan_h, SSE2);
}

void __stdcall afs_get_stripe_count_sse2_popcnt(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
	afs_get_stripe_count_simd(count, sp0, sp1, sp, si_w, scan_w, scan_h, POPCNT|SSE2);
}

void __stdcall afs_get_motion_count_sse2_popcnt(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
	afs_get_motion_count_simd(motion_count, sp, si_w, scan_w, scan_h, POPCNT|SSE2);
}
