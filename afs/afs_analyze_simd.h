#pragma once

const int BLOCK_SIZE_YCP = 256;

void __stdcall afs_analyze_set_threshold_sse2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_avx(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);
void __stdcall afs_analyze_set_threshold_avx2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion);

void __stdcall afs_analyze_12_sse2_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_12_ssse3_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_12_sse4_1_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_12_avx_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_12_avx2_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_1_sse2_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_1_ssse3_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_1_sse4_1_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_1_avx_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_1_avx2_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_2_sse2_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_2_ssse3_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_2_sse4_1_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_2_avx_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);
void __stdcall afs_analyze_2_avx2_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h);

#ifdef ENABLE_FUNC_BASE
#include <emmintrin.h> //イントリンシック命令 SSE2
#if USE_SSSE3
#include <tmmintrin.h> //イントリンシック命令 SSSE3
#endif
#if USE_SSE41
#include <smmintrin.h> //イントリンシック命令 SSE4.1
#endif
#include "simd_util.h"

static const _declspec(align(16)) BYTE pb_thre_count[16]       = { 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03 };
static const _declspec(align(16)) BYTE pw_thre_count2[16]      = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(16)) BYTE pw_thre_count1[16]      = { 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_2stripe_0[16]   = { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_2stripe_1[16]   = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_1stripe_0[16]   = { 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_1stripe_1[16]   = { 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_12stripe_0[16]  = { 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_12stripe_1[16]  = { 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_2motion_0[16]   = { 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04 };
static const _declspec(align(16)) BYTE pw_mask_1motion_0[16]   = { 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40 };

static const _declspec(align(16)) BYTE pb_mask_1stripe_01[16]  = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };
static const _declspec(align(16)) BYTE pb_mask_12stripe_01[16] = { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 };
static const _declspec(align(16)) BYTE pb_mask_12motion_01[16] = { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc };
static const _declspec(align(16)) BYTE pw_mask_12stripe_01[16] = { 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_12motion_01[16] = { 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_lowbyte[16]     = { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00 };

static _declspec(align(16)) USHORT pw_thre_shift[8]       = { 0 };
static _declspec(align(16)) USHORT pw_thre_deint[8]       = { 0 };
static _declspec(align(16)) USHORT pw_thre_motion[3][8]   = { 0 };


static const _declspec(align(16)) BYTE Array_SUFFLE_YCP_Y[16] = {
	0, 1, 6, 7, 12, 13, 2, 3, 8, 9, 14, 15, 4, 5, 10, 11, 
	//0, 1, 6, 7, 12, 13, 2, 3, 8, 9, 14, 15, 4, 5, 10, 11
};

static void __forceinline __stdcall afs_analyze_set_threshold_simd(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    __m128i x0, x1;
    x0 = _mm_set1_epi16((short)thre_shift);
    _mm_stream_si128((__m128i *)pw_thre_shift, x0);
    
    x1 = _mm_set1_epi16((short)thre_deint);
    _mm_stream_si128((__m128i *)pw_thre_deint, x1);
    
    pw_thre_motion[0][0] = thre_Ymotion;
    pw_thre_motion[0][1] = thre_Cmotion;
    pw_thre_motion[0][2] = thre_Cmotion;
    pw_thre_motion[0][3] = thre_Ymotion;
    pw_thre_motion[0][4] = thre_Cmotion;
    pw_thre_motion[0][5] = thre_Cmotion;
    pw_thre_motion[0][6] = thre_Ymotion;
    pw_thre_motion[0][7] = thre_Cmotion;
    
    pw_thre_motion[1][0] = thre_Cmotion;
    pw_thre_motion[1][1] = thre_Ymotion;
    pw_thre_motion[1][2] = thre_Cmotion;
    pw_thre_motion[1][3] = thre_Cmotion;
    pw_thre_motion[1][4] = thre_Ymotion;
    pw_thre_motion[1][5] = thre_Cmotion;
    pw_thre_motion[1][6] = thre_Cmotion;
    pw_thre_motion[1][7] = thre_Ymotion;
    
    pw_thre_motion[2][0] = thre_Cmotion;
    pw_thre_motion[2][1] = thre_Cmotion;
    pw_thre_motion[2][2] = thre_Ymotion;
    pw_thre_motion[2][3] = thre_Cmotion;
    pw_thre_motion[2][4] = thre_Cmotion;
    pw_thre_motion[2][5] = thre_Ymotion;
    pw_thre_motion[2][6] = thre_Cmotion;
    pw_thre_motion[2][7] = thre_Cmotion;
}

template <BOOL aligned>
static void __forceinline afs_analyze_shrink_info_sub(BYTE *src, __m128i &x0, __m128i &x1) {
	__m128i x2, x3, x6, x7;
	x1 = (aligned) ? _mm_load_si128((__m128i *)(src +  0)) : _mm_loadu_si128((__m128i *)(src +  0));
	x2 = (aligned) ? _mm_load_si128((__m128i *)(src + 16)) : _mm_loadu_si128((__m128i *)(src + 16));
	x3 = (aligned) ? _mm_load_si128((__m128i *)(src + 32)) : _mm_loadu_si128((__m128i *)(src + 32));
#if USE_SSSE3
	__m128i xShuffleArray = _mm_load_si128((__m128i*)Array_SUFFLE_YCP_Y);
  #if USE_SSE41
	const int MASK_INT = 0x40 + 0x08 + 0x01;

	x0 = _mm_blend_epi16(x3, x1, MASK_INT);
	x6 = _mm_blend_epi16(x2, x3, MASK_INT);
	x7 = _mm_blend_epi16(x1, x2, MASK_INT);

	x0 = _mm_blend_epi16(x0, x2, MASK_INT << 1);
	x6 = _mm_blend_epi16(x6, x1, MASK_INT << 1);
	x7 = _mm_blend_epi16(x7, x3, MASK_INT << 1);
  #else
	static const _declspec(align(16)) USHORT maskY[8] = { 0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0x0000 };
	__m128i xMask = _mm_load_si128((__m128i*)maskY);

	x0 = select_by_mask(x3, x1, xMask);
	x6 = select_by_mask(x2, x3, xMask);
	x7 = select_by_mask(x1, x2, xMask);

	xMask = _mm_slli_si128(xMask, 2);

	x0 = select_by_mask(x0, x2, xMask);
	x6 = select_by_mask(x6, x1, xMask);
	x7 = select_by_mask(x7, x3, xMask);
  #endif //USE_SSE41

	x0 = _mm_shuffle_epi8(x0, xShuffleArray); //Y
	x6 = _mm_shuffle_epi8(x6, _mm_alignr_epi8(xShuffleArray, xShuffleArray, 6)); //Cb
	x7 = _mm_shuffle_epi8(x7, _mm_alignr_epi8(xShuffleArray, xShuffleArray, 12)); //Cr
#else
	//select y
	static const _declspec(align(16)) USHORT maskY_select[8] = { 0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0x0000 };
	__m128i xMask = _mm_load_si128((__m128i*)maskY_select);

	x0 = select_by_mask(x3, x1, xMask);
	xMask = _mm_slli_si128(xMask, 2);
	x0 = select_by_mask(x0, x2, xMask); //52741630

	x6 = _mm_unpacklo_epi16(x0, x0);    //11663300
	x7 = _mm_unpackhi_epi16(x0, x0);    //55227744
	
	static const _declspec(align(16)) USHORT maskY_shuffle[8] = { 0xffff, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000 };
	xMask = _mm_load_si128((__m128i*)maskY_shuffle);
	x0 = select_by_mask(x7, x6, xMask);                 //51627340
	x0 = _mm_shuffle_epi32(x0, _MM_SHUFFLE(1,2,3,0));   //73625140

	x0 = _mm_unpacklo_epi16(x0, _mm_srli_si128(x0, 8)); //75316420
	x0 = _mm_unpacklo_epi16(x0, _mm_srli_si128(x0, 8)); //76543210

	//select uv
	xMask = _mm_srli_si128(_mm_cmpeq_epi8(xMask, xMask), 8); //0x00000000, 0x00000000, 0xffffffff, 0xffffffff
	x6 = select_by_mask(_mm_srli_si128(x2, 2), _mm_srli_si128(x3, 2), xMask); //x  x v4 u4 v6 u6 x  x 
	x7 = select_by_mask(x1, x2, xMask);               //x  x  v1 u1 v3 u3 x  x
	xMask = _mm_slli_si128(xMask, 4);                 //0x00000000, 0xffffffff, 0xffffffff, 0x00000000
	x1 = palignr_sse2(x2, x1, 2);                     //v2 u2  x  x  x  x v0 u0
	x6 = select_by_mask(x1, x6, xMask);               //v2 u2 v4 u4 v6 u6 v0 u0
	x7 = select_by_mask(x3, x7, xMask);               //v7 u7 v1 u1 v3 u3 v5 u5
	x1 = _mm_shuffle_epi32(x6, _MM_SHUFFLE(1,2,3,0)); //v6 u6 v4 u4 v2 u2 v0 u0
	x2 = _mm_shuffle_epi32(x7, _MM_SHUFFLE(3,0,1,2)); //v7 u7 v5 u5 v3 u3 v1 u1

	x6 = _mm_unpacklo_epi16(x1, x2); //v3 v2 u3 u2 v1 v0 u1 u0
	x7 = _mm_unpackhi_epi16(x1, x2); //v7 v6 u7 u6 v5 v4 u5 u4

	x1 = _mm_unpacklo_epi32(x6, x7); //v5 v4 v1 v0 u5 u4 u1 u0
	x2 = _mm_unpackhi_epi32(x6, x7); //v7 v6 v3 v2 u7 u6 u3 u2

	x6 = _mm_unpacklo_epi32(x1, x2); //u7 u6 u5 u4 u3 u2 u1 u0
	x7 = _mm_unpackhi_epi32(x1, x2); //v7 v6 v5 v4 v3 v2 v1 v0
#endif //USE_SSSE3

	x1 = _mm_or_si128(x0, x6);
	x0 = _mm_and_si128(x0, x6);
	x1 = _mm_or_si128(x1, x7);
	x0 = _mm_and_si128(x0, x7);

	x1 = _mm_slli_epi16(x1, 8);
	x0 = _mm_srai_epi16(x0, 8);
	x1 = _mm_srai_epi16(x1, 8);

	x0 = _mm_packs_epi16(x0, x0);
	x1 = _mm_packs_epi16(x1, x1);

	x0 = _mm_and_si128(x0, _mm_load_si128((__m128i*)pb_mask_12motion_01));
	x1 = _mm_and_si128(x1, _mm_load_si128((__m128i*)pb_mask_12stripe_01));

	x0 = _mm_or_si128(x0, x1);
}

/*
以下 高速化版+6までの旧コード
static void __forceinline __stdcall afs_analyze_shrink_info_simd(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch, DWORD simd) {
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_src = (BYTE *)src;
	__m128i x0, x1, x2, x4, x5, x6, x7;
	__m128i buf[4];

	int ih = 0;
	for (; ih < 4; ih++, ptr_src += 48) {
		afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
		buf[ih] = x0;
	}
	x7 = buf[0];
	x6 = buf[1];
	x5 = buf[2];
	x4 = buf[3];
	for (; ih < h; ih++, ptr_src += 48, ptr_dst += si_pitch) {
		afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
		x2 = x6;
		x2 = _mm_or_si128(x2, x5);
		x2 = _mm_or_si128(x2, x4);
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
		x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
		x2 = _mm_or_si128(x2, x1);
		x2 = _mm_or_si128(x2, x7);
		_mm_storel_epi64((__m128i*)ptr_dst, x2);
		x7 = x6;
		x6 = x5;
		x5 = x4;
		x4 = x0;
	}

	x2 = x6;
	x2 = _mm_or_si128(x2, x5);
	x2 = _mm_or_si128(x2, x4);
	x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
	x2 = _mm_or_si128(x2, x7);
	_mm_storel_epi64((__m128i*)ptr_dst, x2);
	ptr_dst += si_pitch;

	x2 = x5;
	x2 = _mm_or_si128(x2, x4);
	x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
	x2 = _mm_or_si128(x2, x6);
	_mm_storel_epi64((__m128i*)ptr_dst, x2);
	ptr_dst += si_pitch;

	x2 = x4;
	x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
	x2 = _mm_or_si128(x2, x5);
	_mm_storel_epi64((__m128i*)ptr_dst, x2);
	ptr_dst += si_pitch;
	
	_mm_storel_epi64((__m128i*)ptr_dst, x4);
}

static void __forceinline __stdcall afs_analyze_shrink_info_simd_plus(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch, DWORD simd) {
	__m128i x0, x1, x2, x4, x5, x6, x7;
	__m128i buf[4];

	for (int jw = 0; jw < width; jw += 8, dst += 8, src += 8 * h) {
		BYTE *ptr_dst = (BYTE *)dst;
		BYTE *ptr_src = (BYTE *)src;
		int ih = 0;
		for (; ih < 4; ih++, ptr_src += 48) {
			afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
			buf[ih] = x0;
		}
		x7 = buf[0];
		x6 = buf[1];
		x5 = buf[2];
		x4 = buf[3];
		for (; ih < h; ih++, ptr_src += 48, ptr_dst += si_pitch) {
			afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
			x2 = x6;
			x2 = _mm_or_si128(x2, x5);
			x2 = _mm_or_si128(x2, x4);
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
			x2 = _mm_or_si128(x2, x1);
			x2 = _mm_or_si128(x2, x7);
			_mm_storel_epi64((__m128i*)ptr_dst, x2);
			x7 = x6;
			x6 = x5;
			x5 = x4;
			x4 = x0;
		}

		x2 = x6;
		x2 = _mm_or_si128(x2, x5);
		x2 = _mm_or_si128(x2, x4);
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
		x2 = _mm_or_si128(x2, x7);
		_mm_storel_epi64((__m128i*)ptr_dst, x2);
		ptr_dst += si_pitch;

		x2 = x5;
		x2 = _mm_or_si128(x2, x4);
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
		x2 = _mm_or_si128(x2, x6);
		_mm_storel_epi64((__m128i*)ptr_dst, x2);
		ptr_dst += si_pitch;

		x2 = x4;
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
		x2 = _mm_or_si128(x2, x5);
		_mm_storel_epi64((__m128i*)ptr_dst, x2);
		ptr_dst += si_pitch;
	
		_mm_storel_epi64((__m128i*)ptr_dst, x4);		
	}
}
*/
static void __forceinline __stdcall afs_analyze_12_simd_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr, *buf2_ptr;
	BYTE *ptr[2];
	int ih;
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) tmp8pix[48];
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
	buf_ptr = buffer;
	buf2_ptr = buffer + BUFFER_SIZE;

	for (int kw = 0; kw < width6; kw += 48, buf2_ptr += 8) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1));
#if USE_SSSE3
			x0 = _mm_abs_epi16(x0);
#else
			x1 = _mm_setzero_si128();
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
#endif
			x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
			x2 = _mm_load_si128((__m128i *)pw_thre_shift);
			x3 = _mm_cmpgt_epi16(x3, x0);
			x2 = _mm_cmpgt_epi16(x2, x0);
			x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
			x3 = _mm_or_si128(x3, x2);
			_mm_store_si128((__m128i *)(tmp8pix + jw*16), x3);
		}
		afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
		_mm_storel_epi64((__m128i*)(buf2_ptr), x0);
	}

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
	
	for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		buf2_ptr = buffer + BUFFER_SIZE;
		for (int kw = 0; kw < width6; kw += 48, buf2_ptr += 8) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step6));
				x1 = x0;
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6)));
#if USE_SSSE3
				x0 = _mm_abs_epi16(x0);
#else
				x2 = _mm_setzero_si128();
				x2 = _mm_cmpgt_epi16(x2, x0);
				x0 = _mm_xor_si128(x0, x2);
				x0 = _mm_subs_epi16(x0, x2);
#endif
				x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
				x2 = _mm_load_si128((__m128i *)pw_thre_shift);
				x3 = _mm_cmpgt_epi16(x3, x0);
				x2 = _mm_cmpgt_epi16(x2, x0);
				x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
				x3 = _mm_or_si128(x3, x2);
			
				x2 = _mm_loadu_si128((__m128i *)ptr_p0);
				x2 = _mm_subs_epi16(x2, x1);
#if USE_SSSE3
				x0 = _mm_abs_epi16(x2);
				x2 = _mm_cmpeq_epi16(x2, x0);
#else
				x0 = x2;
				x2 = _mm_setzero_si128();
				x2 = _mm_cmpgt_epi16(x2, x0);
				x0 = _mm_xor_si128(x0, x2);
				x0 = _mm_subs_epi16(x0, x2);
#endif
				x1 = x0;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
				x1 = _mm_packs_epi16(x1, x1);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
				x0 = _mm_packs_epi16(x0, x0);
				x1 = _mm_unpacklo_epi8(x1, x0);
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x2 = _mm_xor_si128(x2, x7);
				x7 = _mm_xor_si128(x7, x2);
				x6 = _mm_and_si128(x6, x2);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_subs_epi8(x6, x1);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);

				x0 = x6;
				x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
				x0 = _mm_srli_epi16(x0, 4);
				x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_0));
				x3 = _mm_or_si128(x3, x0);
			
				x2 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x2 = _mm_subs_epi16(x2, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
#if USE_SSSE3
				x0 = _mm_abs_epi16(x2);
				x2 = _mm_cmpeq_epi16(x2, x0);
#else
				x0 = x2;
				x2 = _mm_setzero_si128();
				x2 = _mm_cmpgt_epi16(x2, x0);
				x0 = _mm_xor_si128(x0, x2);
				x0 = _mm_subs_epi16(x0, x2);
#endif
				x1 = x0;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
				x1 = _mm_packs_epi16(x1, x1);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
				x0 = _mm_packs_epi16(x0, x0);
				x1 = _mm_unpacklo_epi8(x1, x0);
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x2 = _mm_xor_si128(x2, x5);
				x5 = _mm_xor_si128(x5, x2);
				x4 = _mm_and_si128(x4, x2);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_subs_epi8(x4, x1);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				x0 = x4;
				x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
				x0 = _mm_srli_epi16(x0, 4);
				x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_1));
				x3 = _mm_or_si128(x3, x0);
			
				_mm_store_si128((__m128i *)(tmp8pix + jw*16), x3);
			}
			afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
			_mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
			_mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
		}

		if (ih >= 4) {
			buf2_ptr = buffer + BUFFER_SIZE;
			ptr_dst = (BYTE *)dst;
			for (int kw = 0; kw < width6; kw += 96, ptr_dst += 16, buf2_ptr += 16) {
				x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
				x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
				x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
				x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
				x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
				x2 = x6;
				x2 = _mm_or_si128(x2, x5);
				x2 = _mm_or_si128(x2, x4);
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
				x2 = _mm_or_si128(x2, x1);
				x2 = _mm_or_si128(x2, x7);
				_mm_storeu_si128((__m128i*)ptr_dst, x2);
			}
			dst += si_pitch;
		}
	}
	//残りの4ライン
	for ( ; ih < h + 4; ih++) {
		ptr_dst = (BYTE *)dst;
		buf2_ptr = buffer + BUFFER_SIZE;
		for (int kw = 0; kw < width6; kw += 96, ptr_dst += 16, buf2_ptr += 16) {
			x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
			x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
			x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
			x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
			x2 = x6;
			x2 = _mm_or_si128(x2, x5);
			x2 = _mm_or_si128(x2, x4);
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
			x2 = _mm_or_si128(x2, x7);
			_mm_storeu_si128((__m128i*)ptr_dst, x2);
			_mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
		}
		dst += si_pitch;
	}
}


static void __forceinline __stdcall afs_analyze_1_simd_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr, *buf2_ptr;
	BYTE *ptr[2];
	int ih;
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) tmp8pix[48];
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
	buf_ptr = buffer;
	buf2_ptr = buffer + BUFFER_SIZE;
	
	x3 = _mm_load_si128((__m128i *)pw_thre_shift);
	for (int kw = 0; kw < width6; kw += 48, buf2_ptr += 8) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			//afs_analyze_1_mmx_sub
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
#if USE_SSSE3
			x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
			x1 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
			x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
			
			_mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
		}
		afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
		_mm_storel_epi64((__m128i*)(buf2_ptr), x0);
	}

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
	for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		buf2_ptr = buffer + BUFFER_SIZE;
		for (int kw = 0; kw < width6; kw += 48, buf2_ptr += 8) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				//afs_analyze_1_mmx_loop
				//former field line
				//analyze motion
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
#if USE_SSSE3
				x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
				x1 = _mm_setzero_si128();
				x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
				x2 = x3;
				x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
				
				//analyze non-shift
				x1 = _mm_loadu_si128((__m128i *)ptr_p0);
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0 + step6)));
#if USE_SSSE3
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
#else
				x0 = x1;
				x1 = _mm_setzero_si128();
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
#endif
				x0 = _mm_cmpgt_epi16(x0, x3);
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x7);
				x7 = _mm_xor_si128(x7, x1);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_and_si128(x6, x0);
				x6 = _mm_subs_epi16(x6, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);


				x1 = x6;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_0));
				x2 = _mm_or_si128(x2, x1);

				
				//analyze shift
				x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1] + step6)));
#if USE_SSSE3
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
#else
				x0 = x1;
				x1 = _mm_setzero_si128();
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
#endif
				x0 = _mm_cmpgt_epi16(x0, x3);
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x5);
				x5 = _mm_xor_si128(x5, x1);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_and_si128(x4, x0);
				x4 = _mm_subs_epi16(x4, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				
				x1 = x4;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_1));
				x2 = _mm_or_si128(x2, x1);
				
				_mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
			}
			afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
			_mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
			_mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
		}

		if (ih >= 4) {
			buf2_ptr = buffer + BUFFER_SIZE;
			ptr_dst = (BYTE *)dst;
			for (int kw = 0; kw < width6; kw += 96, ptr_dst += 16, buf2_ptr += 16) {
				x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
				x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
				x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
				x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
				x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
				x2 = x6;
				x2 = _mm_or_si128(x2, x5);
				x2 = _mm_or_si128(x2, x4);
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
				x2 = _mm_or_si128(x2, x1);
				x2 = _mm_or_si128(x2, x7);
				_mm_storeu_si128((__m128i*)ptr_dst, x2);
			}
			dst += si_pitch;
		}
	}
	//残りの4ライン
	for ( ; ih < h + 4; ih++) {
		ptr_dst = (BYTE *)dst;
		buf2_ptr = buffer + BUFFER_SIZE;
		for (int kw = 0; kw < width6; kw += 96, ptr_dst += 16, buf2_ptr += 16) {
			x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
			x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
			x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
			x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
			x2 = x6;
			x2 = _mm_or_si128(x2, x5);
			x2 = _mm_or_si128(x2, x4);
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
			x2 = _mm_or_si128(x2, x7);
			_mm_storeu_si128((__m128i*)ptr_dst, x2);
			_mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
		}
		dst += si_pitch;
	}
}

static void __forceinline __stdcall afs_analyze_2_simd_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr, *buf2_ptr;
	BYTE *ptr[2];
	int ih;
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) tmp8pix[48];
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
	buf_ptr = buffer;
	buf2_ptr = buffer + BUFFER_SIZE;

	for (int kw = 0; kw < width6; kw += 48, buf2_ptr += 8) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
			//afs_analyze_2_mmx_sub
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
#if USE_SSSE3
			x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
			x1 = _mm_setzero_si128();
			x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
			
			_mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
		}
		afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
		_mm_storel_epi64((__m128i*)(buf2_ptr), x0);
	}

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
	for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		buf2_ptr = buffer + BUFFER_SIZE;
		for (int kw = 0; kw < width6; kw += 48, buf2_ptr += 8) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				//afs_analyze_1_mmx_loop
				//former field line
				//analyze motion
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
#if USE_SSSE3
				x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
				x1 = _mm_setzero_si128();
				x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
				x2 = x3;
				x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
				
				//analyze non-shift
				x1 = _mm_loadu_si128((__m128i *)ptr_p0);
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0 + step6)));
#if USE_SSSE3
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
#else
				x0 = x1;
				x1 = _mm_setzero_si128();
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
#endif
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x7);
				x7 = _mm_xor_si128(x7, x1);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_and_si128(x6, x0);
				x6 = _mm_subs_epi16(x6, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);
				x1 = x6;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_0));
				x2 = _mm_or_si128(x2, x1);
				
				//analyze shift
				x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1] + step6)));
#if USE_SSSE3
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
#else
				x0 = x1;
				x1 = _mm_setzero_si128();
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
#endif
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x5);
				x5 = _mm_xor_si128(x5, x1);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_and_si128(x4, x0);
				x4 = _mm_subs_epi16(x4, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				x1 = x4;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_1));
				x2 = _mm_or_si128(x2, x1);
				
				_mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
			}
			afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
			_mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
			_mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
		}

		if (ih >= 4) {
			buf2_ptr = buffer + BUFFER_SIZE;
			ptr_dst = (BYTE *)dst;
			for (int kw = 0; kw < width6; kw += 96, ptr_dst += 16, buf2_ptr += 16) {
				x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
				x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
				x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
				x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
				x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
				x2 = x6;
				x2 = _mm_or_si128(x2, x5);
				x2 = _mm_or_si128(x2, x4);
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
				x2 = _mm_or_si128(x2, x1);
				x2 = _mm_or_si128(x2, x7);
				_mm_storeu_si128((__m128i*)ptr_dst, x2);
			}
			dst += si_pitch;
		}
	}
	//残りの4ライン
	for ( ; ih < h + 4; ih++) {
		ptr_dst = (BYTE *)dst;
		buf2_ptr = buffer + BUFFER_SIZE;
		for (int kw = 0; kw < width6; kw += 96, ptr_dst += 16, buf2_ptr += 16) {
			x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
			x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
			x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
			x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
			x2 = x6;
			x2 = _mm_or_si128(x2, x5);
			x2 = _mm_or_si128(x2, x4);
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
			x2 = _mm_or_si128(x2, x7);
			_mm_storeu_si128((__m128i*)ptr_dst, x2);
			_mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
		}
		dst += si_pitch;
	}
}

#endif //ENABLE_FUNC_BASE
