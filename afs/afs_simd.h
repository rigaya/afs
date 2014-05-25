#pragma once

#include "afs.h"
#include "filter.h"

void __stdcall afs_blend_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, unsigned char *sip, unsigned int mask, int w);
void __stdcall afs_blend_sse4_1(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, unsigned char *sip, unsigned int mask, int w);
void __stdcall afs_blend_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, unsigned char *sip, unsigned int mask, int w);
void __stdcall afs_blend_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, unsigned char *sip, unsigned int mask, int w);

void __stdcall afs_mie_spot_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w);
void __stdcall afs_mie_spot_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w);
void __stdcall afs_mie_spot_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w);


void __stdcall afs_mie_inter_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w);
void __stdcall afs_mie_inter_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w);
void __stdcall afs_mie_inter_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w);

void __stdcall afs_deint4_sse2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, unsigned char *sip, unsigned int mask, int w);
void __stdcall afs_deint4_sse4_1(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, unsigned char *sip, unsigned int mask, int w);
void __stdcall afs_deint4_avx(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, unsigned char *sip, unsigned int mask, int w);
void __stdcall afs_deint4_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, unsigned char *sip, unsigned int mask, int w);

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

#ifdef ENABLE_FUNC_BASE
#include "simd_util.h"
#include "afs.h"

static const _declspec(align(16)) USHORT pw_round_fix1[8] = {
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
};
static const _declspec(align(16)) USHORT pw_round_fix2[8] = {
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
};
static const _declspec(align(16)) USHORT dq_mask_select_sip[24] = {
    0x0000, 0x0000, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
    0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
    0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0x0000, 0x0000,
};
//r0 := (mask0 == 0) ? a0 : b0

#if USE_SSE41
static const int dq_mask_select_sip_int_0 = 0x80 + 0x40 + 0x20 + 0x10 + 0x00 + 0x04 + 0x00 + 0x00;
static const int dq_mask_select_sip_int_1 = 0x00 + 0x40 + 0x00 + 0x00 + 0x00 + 0x00 + 0x02 + 0x00;
static const int dq_mask_select_sip_int_2 = 0x00 + 0x00 + 0x20 + 0x00 + 0x08 + 0x04 + 0x02 + 0x01;
#define dq_mask_select_sip_int(i) (((i)==0) ? dq_mask_select_sip_int_0 : (((i)==1) ? dq_mask_select_sip_int_1 : dq_mask_select_sip_int_2))
#define dq_mask_select_sip_simd(x2,x3,i) _mm_blend_epi16((x2),(x3),dq_mask_select_sip_int(i))
#define _mm_blendv_epi8_simd _mm_blendv_epi8
#else
#define dq_mask_select_sip_simd(x2,x3,i) select_by_mask((x2),(x3),_mm_load_si128((__m128i*)&dq_mask_select_sip[(i)*8]))
#define _mm_blendv_epi8_simd select_by_mask
#endif

static const _declspec(align(16)) USHORT pw_mask_0c[8] = {
    0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c,
};

template <bool aligned_store>
static void __forceinline __stdcall afs_blend_simd_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
	BYTE *ptr_dst  = (BYTE *)dst;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src2 = (BYTE *)src2;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_esi  = (BYTE *)sip;
	__m128i x0, x1, x2, x3, x4;
	const __m128i xMask = _mm_unpacklo_epi8(_mm_set1_epi32(mask), _mm_setzero_si128());
	const __m128i xPwRoundFix2 = _mm_load_si128((__m128i*)pw_round_fix2);

	for (int iw = w - 8; iw >= 0; ptr_src2 += 48, ptr_dst += 48, ptr_src1 += 48, ptr_src3 += 48, ptr_esi += 8, iw -= 8) {
		x0 = _mm_loadu_si128((__m128i*)(ptr_esi));
		x4 = _mm_setzero_si128();
		x0 = _mm_unpacklo_epi8(x0, x4);
		x0 = _mm_and_si128(x0, xMask);
		x0 = _mm_cmpeq_epi16(x0, x4); //sip(7766554433221100)

		x2 = x0;
		x2 = _mm_unpacklo_epi16(x2, x0); //x2 = sip(3333222211110000)
		x3 = x2;
		x3 = _mm_slli_si128(x3, 4);      //x3 = sip(222211110000xxxx)
		x1 = dq_mask_select_sip_simd(x2, x3, 0); //x1 = sip(2222111111000000)

		x4 = _mm_loadu_si128((__m128i*)(ptr_src2));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src1));
		x2 = x4;
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src3)));
		x4 = _mm_slli_epi16(x4, 1);
		x3 = _mm_adds_epi16(x3, x4);
		x3 = _mm_adds_epi16(x3, xPwRoundFix2);
		x3 = _mm_srai_epi16(x3, 2);
		x1 = _mm_blendv_epi8_simd(x2, x3, x1); //x1 = sip ? x3 : x2;
		_mm_stream_switch_si128((__m128i*)ptr_dst, x1);

		x1 = x0;
		x1 = _mm_srli_si128(x1, 4);                       //x1 = sip(xxxx776655443322)
		x1 = _mm_unpacklo_epi16(x1, x1);                  //x1 = sip(5555444433332222)
		x2 = _mm_shuffle_epi32(x1, _MM_SHUFFLE(2,2,1,1)); //x2 = sip(4444444433333333)
		x1 = dq_mask_select_sip_simd(x1, x2, 1); //x1 = sip(5544444433333322)

		x4 = _mm_loadu_si128((__m128i*)(ptr_src2+16));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src1+16));
		x2 = x4;
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src3+16)));
		x4 = _mm_slli_epi16(x4, 1);
		x3 = _mm_adds_epi16(x3, x4);
		x3 = _mm_adds_epi16(x3, xPwRoundFix2);
		x3 = _mm_srai_epi16(x3, 2);
		x1 = _mm_blendv_epi8_simd(x2, x3, x1); //x1 = sip ? x3 : x2;
		_mm_stream_switch_si128((__m128i*)(ptr_dst+16), x1);

		x2 = x0;
		x2 = _mm_unpackhi_epi16(x2, x2); //x2 = sip(7777666655554444)
		x3 = _mm_srli_si128(x2, 4);      //x3 = sip(xxxx777766665555)
		x1 = dq_mask_select_sip_simd(x2, x3, 2); //x1 = sip(7777776666665555)

		x4 = _mm_loadu_si128((__m128i*)(ptr_src2+32));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src1+32));
		x2 = x4;
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src3+32)));
		x4 = _mm_slli_epi16(x4, 1);
		x3 = _mm_adds_epi16(x3, x4);
		x3 = _mm_adds_epi16(x3, xPwRoundFix2);
		x3 = _mm_srai_epi16(x3, 2);
		x1 = _mm_blendv_epi8_simd(x2, x3, x1); //x1 = sip ? x3 : x2;
		_mm_stream_switch_si128((__m128i*)(ptr_dst+32), x1);
	}
}

static void __forceinline __stdcall afs_blend_simd(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
	const int dst_mod16 = (size_t)dst & 0x0f;
	if (dst_mod16) {
		int dw = (dst_mod16) ? (16 * (3-((dst_mod16 % 6)>>1))-dst_mod16) / 6 : 0;
		afs_blend_simd_base<false>(dst, src1, src2, src3, sip, mask, 8);
		dst += dw; src1 += dw; src2 += dw; src3 += dw; sip += dw; w -= dw;
	}
	afs_blend_simd_base<true>(dst, src1, src2, src3, sip, mask, w & (~0x07));
	if (w & 0x07) {
		dst += w-8; src1 += w-8; src2 += w-8; src3 += w-8; sip += w-8;
		afs_blend_simd_base<false>(dst, src1, src2, src3, sip, mask, 8);
	}
}

template <bool aligned_store>
static void __forceinline __stdcall afs_mie_spot_simd_base( PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w) {
	BYTE *ptr_dst  = (BYTE *)dst;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src2 = (BYTE *)src2;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_src4 = (BYTE *)src4;
	BYTE *ptr_src_spot = (BYTE *)src_spot;
	__m128i x0, x1, x2, x3, x4, x5;
	const __m128i xPwRoundFix1 = _mm_load_si128((__m128i*)pw_round_fix1);
	const __m128i xPwRoundFix2 = _mm_load_si128((__m128i*)pw_round_fix2);

	for (int iw = w - 8; iw >= 0; ptr_src1 += 48, ptr_src2 += 48, ptr_src3 += 48, ptr_src4 += 48, ptr_src_spot += 48, ptr_dst += 48, iw -= 8) {
		x0 = _mm_loadu_si128((__m128i*)(ptr_src1 +  0));
		x1 = _mm_loadu_si128((__m128i*)(ptr_src1 + 16));
		x2 = _mm_loadu_si128((__m128i*)(ptr_src1 + 32));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src3 +  0));
		x4 = _mm_loadu_si128((__m128i*)(ptr_src3 + 16));
		x5 = _mm_loadu_si128((__m128i*)(ptr_src3 + 32));
		x0 = _mm_adds_epi16(x0, _mm_loadu_si128((__m128i*)(ptr_src2 +  0)));
		x1 = _mm_adds_epi16(x1, _mm_loadu_si128((__m128i*)(ptr_src2 + 16)));
		x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src2 + 32)));
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src4 +  0)));
		x4 = _mm_adds_epi16(x4, _mm_loadu_si128((__m128i*)(ptr_src4 + 16)));
		x5 = _mm_adds_epi16(x5, _mm_loadu_si128((__m128i*)(ptr_src4 + 32)));
		x0 = _mm_adds_epi16(x0, x3);
		x1 = _mm_adds_epi16(x1, x4);
		x2 = _mm_adds_epi16(x2, x5);
		x3 = _mm_loadu_si128((__m128i*)(ptr_src_spot +  0));
		x4 = _mm_loadu_si128((__m128i*)(ptr_src_spot + 16));
		x5 = _mm_loadu_si128((__m128i*)(ptr_src_spot + 32));
		x0 = _mm_adds_epi16(x0, xPwRoundFix2);
		x1 = _mm_adds_epi16(x1, xPwRoundFix2);
		x2 = _mm_adds_epi16(x2, xPwRoundFix2);
		x3 = _mm_adds_epi16(x3, xPwRoundFix1);
		x4 = _mm_adds_epi16(x4, xPwRoundFix1);
		x5 = _mm_adds_epi16(x5, xPwRoundFix1);
		x0 = _mm_srai_epi16(x0, 2);
		x1 = _mm_srai_epi16(x1, 2);
		x2 = _mm_srai_epi16(x2, 2);
		x0 = _mm_adds_epi16(x0, x3);
		x1 = _mm_adds_epi16(x1, x4);
		x2 = _mm_adds_epi16(x2, x5);
		x0 = _mm_srai_epi16(x0, 1);
		x1 = _mm_srai_epi16(x1, 1);
		x2 = _mm_srai_epi16(x2, 1);
		_mm_stream_switch_si128((__m128i*)(ptr_dst +  0), x0);
		_mm_stream_switch_si128((__m128i*)(ptr_dst + 16), x1);
		_mm_stream_switch_si128((__m128i*)(ptr_dst + 32), x2);
	}
}

static void __forceinline __stdcall afs_mie_spot_simd(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot, int w) {
	const int dst_mod16 = (size_t)dst & 0x0f;
	if (dst_mod16) {
		int dw = (dst_mod16) ? (16 * (3-((dst_mod16 % 6)>>1))-dst_mod16) / 6 : 0;
		afs_mie_spot_simd_base<false>(dst, src1, src2, src3, src4, src_spot, 8);
		dst += dw; src1 += dw; src2 += dw; src3 += dw; src4 += dw; src_spot += dw; w -= dw;
	}
	afs_mie_spot_simd_base<true>(dst, src1, src2, src3, src4, src_spot, w & (~0x07));
	if (w & 0x07) {
		dst += w-8; src1 += w-8; src2 += w-8; src3 += w-8; src4 += w-8; src_spot += w-8;
		afs_mie_spot_simd_base<false>(dst, src1, src2, src3, src4, src_spot, 8);
	}
}

template <bool aligned_store>
static void __forceinline __stdcall afs_mie_inter_simd_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src2 = (BYTE *)src2;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_src4 = (BYTE *)src4;
	__m128i x0, x1, x2, x3, x4, x5;
	const __m128i xPwRoundFix2 = _mm_load_si128((__m128i*)pw_round_fix2);

	for (int iw = w - 8; iw >= 0; ptr_src1 += 48, ptr_src2 += 48, ptr_src3 += 48, ptr_src4 += 48, ptr_dst += 48, iw -= 8) {
		x0 = _mm_loadu_si128((__m128i*)(ptr_src1 +  0));
		x1 = _mm_loadu_si128((__m128i*)(ptr_src1 + 16));
		x2 = _mm_loadu_si128((__m128i*)(ptr_src1 + 32));

		// new code ----------------------------------------------------
		x3 = _mm_loadu_si128((__m128i*)(ptr_src3 +  0));
		x4 = _mm_loadu_si128((__m128i*)(ptr_src3 + 16));
		x5 = _mm_loadu_si128((__m128i*)(ptr_src3 + 32));
		x0 = _mm_adds_epi16(x0, _mm_loadu_si128((__m128i*)(ptr_src2 +  0)));
		x1 = _mm_adds_epi16(x1, _mm_loadu_si128((__m128i*)(ptr_src2 + 16)));
		x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src2 + 32)));
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src4 +  0)));
		x4 = _mm_adds_epi16(x4, _mm_loadu_si128((__m128i*)(ptr_src4 + 16)));
		x5 = _mm_adds_epi16(x5, _mm_loadu_si128((__m128i*)(ptr_src4 + 32)));
		x0 = _mm_adds_epi16(x0, x3);
		x1 = _mm_adds_epi16(x1, x4);
		x2 = _mm_adds_epi16(x2, x5);
		// new code ----------------------------------------------------

		// original code -----------------------------------------------
		//x0 = _mm_adds_epi16(x0, _mm_loadu_si128((__m128i*)(ptr_src2 +  0)));
		//x1 = _mm_adds_epi16(x1, _mm_loadu_si128((__m128i*)(ptr_src2 + 16)));
		//x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src2 + 32)));
		//x0 = _mm_adds_epi16(x0, _mm_loadu_si128((__m128i*)(ptr_src3 +  0)));
		//x1 = _mm_adds_epi16(x1, _mm_loadu_si128((__m128i*)(ptr_src3 + 16)));
		//x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src3 + 32)));
		//x0 = _mm_adds_epi16(x0, _mm_loadu_si128((__m128i*)(ptr_src4 +  0)));
		//x1 = _mm_adds_epi16(x1, _mm_loadu_si128((__m128i*)(ptr_src4 + 16)));
		//x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src4 + 32)));
		// original code -----------------------------------------------

		x0 = _mm_adds_epi16(x0, xPwRoundFix2);
		x1 = _mm_adds_epi16(x1, xPwRoundFix2);
		x2 = _mm_adds_epi16(x2, xPwRoundFix2);
		x0 = _mm_srai_epi16(x0, 2);
		x1 = _mm_srai_epi16(x1, 2);
		x2 = _mm_srai_epi16(x2, 2);
		_mm_stream_switch_si128((__m128i*)(ptr_dst +  0), x0);
		_mm_stream_switch_si128((__m128i*)(ptr_dst + 16), x1);
		_mm_stream_switch_si128((__m128i*)(ptr_dst + 32), x2);
	}
}

static void __forceinline __stdcall afs_mie_inter_simd(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
	const int dst_mod16 = (size_t)dst & 0x0f;
	if (dst_mod16) {
		int dw = (dst_mod16) ? (16 * (3-((dst_mod16 % 6)>>1))-dst_mod16) / 6 : 0;
		afs_mie_inter_simd_base<false>(dst, src1, src2, src3, src4, 8);
		dst += dw; src1 += dw; src2 += dw; src3 += dw; src4 += dw; w -= dw;
	}
	afs_mie_inter_simd_base<true>(dst, src1, src2, src3, src4, w & (~0x07));
	if (w & 0x07) {
		dst += w-8; src1 += w-8; src2 += w-8; src3 += w-8; src4 += w-8;
		afs_mie_inter_simd_base<false>(dst, src1, src2, src3, src4, 8);
	}
}

template <bool aligned_store>
static void __forceinline __stdcall afs_deint4_simd_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_sip = (BYTE *)sip;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_src4 = (BYTE *)src4;
	BYTE *ptr_src5 = (BYTE *)src5;
	BYTE *ptr_src7 = (BYTE *)src7;
	__m128i x0, x1, x2, x3, x4;
	const __m128i xMask = _mm_unpacklo_epi8(_mm_set1_epi32(mask), _mm_setzero_si128());
	const __m128i xPwRoundFix1 = _mm_load_si128((__m128i*)pw_round_fix1);

	for (int iw = w - 8; iw >= 0; ptr_src4 += 48, ptr_dst += 48, ptr_src3 += 48, ptr_src5 += 48, ptr_src1 += 48, ptr_src7 += 48, ptr_sip += 8, iw -= 8) {
		x0 = _mm_loadu_si128((__m128i*)(ptr_sip));
		x4 = _mm_setzero_si128();
		x0 = _mm_unpacklo_epi8(x0, x4);
		x0 = _mm_and_si128(x0, xMask);
		x0 = _mm_cmpeq_epi16(x0, x4); //sip(7766554433221100)

		x2 = x0;
		x2 = _mm_unpacklo_epi16(x2, x0); //x2 = sip(3333222211110000)
		x3 = x2;
		x3 = _mm_slli_si128(x3, 4);      //x3 = sip(222211110000xxxx)
		x1 = dq_mask_select_sip_simd(x2, x3, 0); //x1 = sip(2222111111000000)

		x2 = _mm_loadu_si128((__m128i*)(ptr_src1));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src3));
		x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src7)));
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src5)));
		x2 = _mm_subs_epi16(x2, x3);
		x2 = _mm_srai_epi16(x2, 3);
		x3 = _mm_subs_epi16(x3, x2);
		x3 = _mm_adds_epi16(x3, xPwRoundFix1);
		x3 = _mm_srai_epi16(x3, 1);
		x2 = _mm_loadu_si128((__m128i*)(ptr_src4));
		x1 = _mm_blendv_epi8_simd(x2, x3, x1); //x1 = sip ? x3 : x2;
		_mm_stream_switch_si128((__m128i*)(ptr_dst), x1);

		x1 = x0;
		x1 = _mm_srli_si128(x1, 4);                       //x1 = sip(xxxx776655443322)
		x1 = _mm_unpacklo_epi16(x1, x1);                  //x1 = sip(5555444433332222)
		x2 = _mm_shuffle_epi32(x1, _MM_SHUFFLE(2,2,1,1)); //x2 = sip(4444444433333333)
		x1 = dq_mask_select_sip_simd(x1, x2, 1); //x1 = sip(5544444433333322)

		x2 = _mm_loadu_si128((__m128i*)(ptr_src1+16));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src3+16));
		x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src7+16)));
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src5+16)));
		x2 = _mm_subs_epi16(x2, x3);
		x2 = _mm_srai_epi16(x2, 3);
		x3 = _mm_subs_epi16(x3, x2);
		x3 = _mm_adds_epi16(x3, xPwRoundFix1);
		x3 = _mm_srai_epi16(x3, 1);
		x2 = _mm_loadu_si128((__m128i*)(ptr_src4+16));
		x1 = _mm_blendv_epi8_simd(x2, x3, x1); //x1 = sip ? x3 : x2;
		_mm_stream_switch_si128((__m128i*)(ptr_dst+16), x1);

		x2 = x0;
		x2 = _mm_unpackhi_epi16(x2, x2); //x2 = sip(7777666655554444)
		x3 = _mm_srli_si128(x2, 4);      //x3 = sip(xxxx777766665555)
		x1 = dq_mask_select_sip_simd(x2, x3, 2); //x1 = sip(7777776666665555)

		x2 = _mm_loadu_si128((__m128i*)(ptr_src1+32));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src3+32));
		x2 = _mm_adds_epi16(x2, _mm_loadu_si128((__m128i*)(ptr_src7+32)));
		x3 = _mm_adds_epi16(x3, _mm_loadu_si128((__m128i*)(ptr_src5+32)));
		x2 = _mm_subs_epi16(x2, x3);
		x2 = _mm_srai_epi16(x2, 3);
		x3 = _mm_subs_epi16(x3, x2);
		x3 = _mm_adds_epi16(x3, xPwRoundFix1);
		x3 = _mm_srai_epi16(x3, 1);
		x2 = _mm_loadu_si128((__m128i*)(ptr_src4+32));
		x1 = _mm_blendv_epi8_simd(x2, x3, x1); //x1 = sip ? x3 : x2;
		_mm_stream_switch_si128((__m128i*)(ptr_dst+32), x1);
	}
}

static void __forceinline __stdcall afs_deint4_simd(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
	const int dst_mod16 = (size_t)dst & 0x0f;
	if (dst_mod16) {
		int dw = (dst_mod16) ? (16 * (3-((dst_mod16 % 6)>>1))-dst_mod16) / 6 : 0;
		afs_deint4_simd_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, 8);
		dst += dw; src1 += dw; src3 += dw; src4 += dw; src5 += dw; src7 += dw; sip += dw; w -= dw;
	}
	afs_deint4_simd_base<true>(dst, src1, src3, src4, src5, src7, sip, mask, w & (~0x07));
	if (w & 0x07) {
		dst += w-8; src1 += w-8; src3 += w-8; src4 += w-8; src5 += w-8; src7 += w-8; sip += w-8;
		afs_deint4_simd_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, 8);
	}
}

static const _declspec(align(16)) BYTE STRIPE_COUNT_CHECK_MASK[][16] = {
	{ 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50 }, 
	{ 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60 }, 
};

static void __forceinline __stdcall afs_get_stripe_count_simd(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
	const int y_fin = scan_h - sp0->bottom - ((scan_h - sp0->top - sp0->bottom) & 1);
	const DWORD check_mask[2] = { 0x50, 0x60 };
	__m128i xZero = _mm_setzero_si128();
	__m128i xMask, x0, x1;
	for(int pos_y = sp0->top; pos_y < y_fin; pos_y++) {
		BYTE *sip = sp->map + pos_y * si_w + sp0->left;
		const int first_field_flag = !is_latter_field(pos_y, sp0->tb_order);
		xMask = _mm_load_si128((__m128i*)STRIPE_COUNT_CHECK_MASK[first_field_flag]);
		const int x_count = scan_w - sp0->right - sp0->left;
		unsigned char *sip_fin = sip + (x_count & ~31);
		for ( ; sip < sip_fin; sip += 32) {
			x0 = _mm_loadu_si128((__m128i*)(sip +  0));
			x1 = _mm_loadu_si128((__m128i*)(sip + 16));
			x0 = _mm_and_si128(x0, xMask);
			x1 = _mm_and_si128(x1, xMask);
			x0 = _mm_cmpeq_epi8(x0, xZero);
			x1 = _mm_cmpeq_epi8(x1, xZero);
			DWORD count0 = _mm_movemask_epi8(x0);
			DWORD count1 = _mm_movemask_epi8(x1);
			count[first_field_flag] += popcnt32(((count1 << 16) | count0));
		}
		if (x_count & 16) {
			x0 = _mm_loadu_si128((__m128i*)sip);
			x0 = _mm_and_si128(x0, xMask);
			x0 = _mm_cmpeq_epi8(x0, xZero);
			DWORD count0 = _mm_movemask_epi8(x0);
			count[first_field_flag] += popcnt32(count0);
			sip += 16;
		}
		sip_fin = sip + (x_count & 15);
		for ( ; sip < sip_fin; sip++)
			count[first_field_flag] += (!(*sip & check_mask[first_field_flag]));
	}
}

static const _declspec(align(16)) BYTE MOTION_COUNT_CHECK[16] = {
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
};

static void __forceinline __stdcall afs_get_motion_count_simd(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
	const int y_fin = scan_h - sp->bottom - ((scan_h - sp->top - sp->bottom) & 1);
	__m128i xMotion = _mm_load_si128((__m128i *)MOTION_COUNT_CHECK);
	__m128i x0, x1;
	for(int pos_y = sp->top; pos_y < y_fin; pos_y++) {
		BYTE *sip = sp->map + pos_y * si_w + sp->left;
		const int is_latter_feild = is_latter_field(pos_y, sp->tb_order);
		const int x_count = scan_w - sp->right - sp->left;
		BYTE *sip_fin = sip + (x_count & ~31);
		for ( ; sip < sip_fin; sip += 32) {
			x0 = _mm_loadu_si128((__m128i*)(sip +  0));
			x1 = _mm_loadu_si128((__m128i*)(sip + 16));
			x0 = _mm_andnot_si128(x0, xMotion);
			x1 = _mm_andnot_si128(x1, xMotion);
			x0 = _mm_cmpeq_epi8(x0, xMotion);
			x1 = _mm_cmpeq_epi8(x1, xMotion);
			DWORD count0 = _mm_movemask_epi8(x0);
			DWORD count1 = _mm_movemask_epi8(x1);
			motion_count[is_latter_feild] += popcnt32(((count1 << 16) | count0));
		}
		if (x_count & 16) {
			x0 = _mm_loadu_si128((__m128i*)sip);
			x0 = _mm_andnot_si128(x0, xMotion);
			x0 = _mm_cmpeq_epi8(x0, xMotion);
			DWORD count0 = _mm_movemask_epi8(x0);
			motion_count[is_latter_feild] += popcnt32(count0);
			sip += 16;
		}
		sip_fin = sip + (x_count & 15);
		for ( ; sip < sip_fin; sip++)
			motion_count[is_latter_feild] += ((~*sip & 0x40) >> 6);
	}
}


#endif //ENABLE_FUNC_BASE
