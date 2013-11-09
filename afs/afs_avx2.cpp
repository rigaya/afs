#include <nmmintrin.h> //SSE4.2
#include <immintrin.h> //AVX, AVX2
#include <Windows.h>
#include "filter.h"
#include "afs.h"
#include "simd_util.h"

static const _declspec(align(32)) USHORT pw_round_fix1[16] = {
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,
};
static const _declspec(align(32)) USHORT pw_round_fix2[16] = {
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
    0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002, 0x0002,
};
static const _declspec(align(16)) USHORT dq_mask_select_sip[24] = {
    0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0x0000, 0x0000,
    0x0000, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0x0000,
    0x0000, 0x0000, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,
};
//r0 := (mask0 == 0) ? a0 : b0
static const int dq_mask_select_sip_int_0 = 0x80 + 0x40 + 0x20 + 0x10 + 0x00 + 0x04 + 0x00 + 0x00;
static const int dq_mask_select_sip_int_1 = 0x00 + 0x40 + 0x00 + 0x00 + 0x00 + 0x00 + 0x02 + 0x00;
static const int dq_mask_select_sip_int_2 = 0x00 + 0x00 + 0x20 + 0x00 + 0x08 + 0x04 + 0x02 + 0x01;

static const _declspec(align(32)) USHORT pw_mask_0c[16] = {
    0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c,
    0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c, 0x000c,
};

static const _declspec(align(64)) BYTE STRIPE_COUNT_CHECK_MASK[][32] = {
	{ 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 
	  0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, },
	{ 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 
	  0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, }, 
};
static const _declspec(align(32)) BYTE MOTION_COUNT_CHECK[32] = {
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
};

void __stdcall afs_get_stripe_count_avx2(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
	const int y_fin = scan_h - sp0->bottom - ((scan_h - sp0->top - sp0->bottom) & 1);
	const DWORD check_mask[2] = { 0x50, 0x60 };
	__m256i yMask, y0, y1;
	for(int pos_y = sp0->top; pos_y < y_fin; pos_y++) {
		BYTE *sip = sp->map + pos_y * si_w + sp0->left;
		const int first_field_flag = !is_latter_field(pos_y, sp0->tb_order);
		yMask = _mm256_load_si256((__m256i*)STRIPE_COUNT_CHECK_MASK[first_field_flag]);
		int x_count = scan_w - sp0->right - sp0->left;
		int line_count = 0;
		BYTE *sip_fin = (BYTE *)(((size_t)sip + 31) & ~31);
		x_count -= (sip_fin - sip);
		for ( ; sip < sip_fin; sip++)
			line_count += (!(*sip & check_mask[first_field_flag]));
		sip_fin = sip + (x_count & ~63);
		for ( ; sip < sip_fin; sip += 64) {
			y0 = _mm256_load_si256((__m256i*)(sip +  0));
			y1 = _mm256_load_si256((__m256i*)(sip + 32));
			y0 = _mm256_and_si256(y0, yMask);
			y1 = _mm256_and_si256(y1, yMask);
			y0 = _mm256_cmpeq_epi8(y0, _mm256_setzero_si256());
			y1 = _mm256_cmpeq_epi8(y1, _mm256_setzero_si256());
			DWORD count0 = _mm256_movemask_epi8(y0);
			DWORD count1 = _mm256_movemask_epi8(y1);
			line_count += _mm_popcnt_u32(count0) + _mm_popcnt_u32(count1);
		}
		if (x_count & 32) {
			y0 = _mm256_load_si256((__m256i*)sip);
			y0 = _mm256_and_si256(y0, yMask);
			y0 = _mm256_cmpeq_epi8(y0, _mm256_setzero_si256());
			DWORD count0 = _mm256_movemask_epi8(y0);
			line_count += _mm_popcnt_u32(count0);
			sip += 32;
		}
		if (x_count & 16) {
			__m128i x0 = _mm_load_si128((__m128i*)sip);
			x0 = _mm_and_si128(x0, _mm256_castsi256_si128(yMask));
			x0 = _mm_cmpeq_epi8(x0, _mm_setzero_si128());
			DWORD count0 = _mm_movemask_epi8(x0);
			line_count += _mm_popcnt_u32(count0);
			sip += 16;
		}
		sip_fin = sip + (x_count & 15);
		for ( ; sip < sip_fin; sip++)
			line_count += (!(*sip & check_mask[first_field_flag]));
		count[first_field_flag] += line_count;
	}
	_mm256_zeroupper();
}

void __stdcall afs_get_motion_count_avx2(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
	const int y_fin = scan_h - sp->bottom - ((scan_h - sp->top - sp->bottom) & 1);
	__m256i yMotion = _mm256_load_si256((__m256i *)MOTION_COUNT_CHECK);
	__m256i y0, y1;
	for(int pos_y = sp->top; pos_y < y_fin; pos_y++) {
		BYTE *sip = sp->map + pos_y * si_w + sp->left;
		const int is_latter_feild = is_latter_field(pos_y, sp->tb_order);
		int x_count = scan_w - sp->right - sp->left;
		int line_count = 0;
		BYTE *sip_fin = (BYTE *)(((size_t)sip + 31) & ~31);
		x_count -= (sip_fin - sip);
		for ( ; sip < sip_fin; sip++)
			line_count += ((~*sip & 0x40) >> 6);
		sip_fin = sip + (x_count & ~63);
		for ( ; sip < sip_fin; sip += 64) {
			y0 = _mm256_load_si256((__m256i*)(sip +  0));
			y1 = _mm256_load_si256((__m256i*)(sip + 32));
			y0 = _mm256_andnot_si256(y0, yMotion);
			y1 = _mm256_andnot_si256(y1, yMotion);
			y0 = _mm256_cmpeq_epi8(y0, yMotion);
			y1 = _mm256_cmpeq_epi8(y1, yMotion);
			DWORD count0 = _mm256_movemask_epi8(y0);
			DWORD count1 = _mm256_movemask_epi8(y1);
			line_count += _mm_popcnt_u32(count0) + _mm_popcnt_u32(count1);
		}
		if (x_count & 32) {
			y0 = _mm256_load_si256((__m256i*)sip);
			y0 = _mm256_andnot_si256(y0, yMotion);
			y0 = _mm256_cmpeq_epi8(y0, yMotion);
			DWORD count0 = _mm256_movemask_epi8(y0);
			line_count += _mm_popcnt_u32(count0);
			sip += 32;
		}
		if (x_count & 16) {
			__m128i x0 = _mm_load_si128((__m128i*)sip);
			x0 = _mm_andnot_si128(x0, _mm256_castsi256_si128(yMotion));
			x0 = _mm_cmpeq_epi8(x0, _mm256_castsi256_si128(yMotion));
			DWORD count0 = _mm_movemask_epi8(x0);
			line_count += _mm_popcnt_u32(count0);
			sip += 16;
		}
		sip_fin = sip + (x_count & 15);
		for ( ; sip < sip_fin; sip++)
			line_count += ((~*sip & 0x40) >> 6);
		motion_count[is_latter_feild] += line_count;
	}
	_mm256_zeroupper();
}


//(1111110011000000 1111111100110000) blend mask 0
//(1111111100110000 0000110011111111) blend mask 1
//(0000110011111111 0000001100111111) blend mask 2

static const _declspec(align(32)) USHORT SIP_BLEND_MASK[][32] = {
	{ 0x0000, 0x0000, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff,     0x0000, 0x0000, 0x0000, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff },
	{ 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0x0000, 0x0000,     0x0000, 0x0000, 0xffff, 0x0000, 0xffff, 0xffff, 0xffff, 0xffff },
	{ 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0x0000, 0x0000, 0x0000,     0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0xffff, 0x0000, 0x0000 },
};
#define ySIPMASK(x)  (_mm256_load_si256((__m256i*)SIP_BLEND_MASK[x]))

void __stdcall afs_blend_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
	BYTE *ptr_dst  = (BYTE *)dst;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src2 = (BYTE *)src2;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_esi  = (BYTE *)sip;
	__m256i y0, y1, y2, y3, y4;
	const __m256i yMask = _mm256_unpacklo_epi8(_mm256_set1_epi32(mask), _mm256_setzero_si256());
	const __m256i yPwRoundFix2 = _mm256_load_si256((__m256i*)pw_round_fix2);

	for (int step = 0, iw = w - 16; iw >= 0; ptr_src2 += step*6, ptr_dst += step*6, ptr_src1 += step*6, ptr_src3 += step*6, ptr_esi += step, iw -= step) {
		y0 = _mm256_loadu_si256((__m256i*)(ptr_esi));
		y4 = _mm256_setzero_si256();
		y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(1,1,0,0));
		y0 = _mm256_unpacklo_epi8(y0, y4);
		y0 = _mm256_and_si256(y0, yMask);
		y0 = _mm256_cmpeq_epi16(y0, y4); //sip(ffeeddbbccbbaa99887766554433221100)

/*
blendや_mm256_permute2x128_si256では、上に出てきたものがa

blend r = (mask) ? b : a

sip(ffeeddccbbaa9988 7766554433221100)
sip(7766554433221100 7766554433221100) _mm256_permute4x64_epi64, _MM_SHUFFLE(1,0,1,0)
sip(xxxxx77665544332 xxxxx77665544332) _mm256_bsrli_epi128, 5
sip(xxxxx77665544332 7766554433221100) _mm256_blend_epi32, 0xf0
sip(6655554444333322 3333222211110000) _mm256_unpacklo_epi16
sip(554444333322xxxx 222211110000xxxx) _mm256_bslli_epi128, 4
sip(5544444433333322 2222111111000000) _mm256_blendv_epi8
   (1111110011000000 1111111100110000) blend mask
*/

		y2 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(1,0,1,0)); //y2 = sip(7766554433221100 7766554433221100)
		y3 = _mm256_bsrli_epi128(y2, 5);                         //y3 = sip(xxxxx77665544332 xxxxx77665544332)
		y2 = _mm256_blend_epi32(y2, y3, 0xf0);                   //y2 = sip(xxxxx77665544332 7766554433221100)
		y2 = _mm256_unpacklo_epi8(y2, y2);                       //y2 = sip(6655554444333322 3333222211110000)
		y3 = _mm256_bslli_epi128(y2, 4);                         //y3 = sip(554444333322xxxx 222211110000xxxx)
		y1 = _mm256_blendv_epi8(y2, y3, ySIPMASK(0));            //y1 = sip(5544444433333322 2222111111000000)

		y4 = _mm256_loadu_si256((__m256i*)(ptr_src2));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src1));
		y2 = y4;
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src3)));
		y4 = _mm256_slli_epi16(y4, 1);
		y3 = _mm256_adds_epi16(y3, y4);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix2);
		y3 = _mm256_srai_epi16(y3, 2);
		y1 = _mm256_blendv_epi8(y2, y3, y1); //y1 = sip ? y3 : y2;
		_mm256_storeu_si256((__m256i*)ptr_dst, y1);


/*
blendや_mm256_permute2x128_si256では、上に出てきたものがa

blend r = (mask) ? b : a

sip(ffeeddccbbaa9988 7766554433221100)
sip(bbaa9988bbaa9988 7766554477665544) _mm256_permute4x64_epi64, _MM_SHUFFLE(2,2,1,1)
sip(bbbbaaaa99998888 7777666655554444) _mm256_unpacklo_epi16
sip(aaaa99998888xxxx 666655554444xxxx) _mm256_bslli_epi128, 4
sip(xxxxbbbbaaaa9999 xxxx777766665555) _mm256_bsrli_epi128, 4
sip(aaaa99998888xxxx xxxx777766665555) _mm256_blend_epi32, 0x0f
sip(aaaa999999888888 7777776666665555) _mm256_blendv_epi8
   (1111111100110000 0000110011111111) blend mask
*/
		y1 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(2,2,1,1)); //y1 = sip(bbaa9988bbaa9988 7766554477665544)
		y1 = _mm256_unpacklo_epi16(y1, y1);                      //y1 = sip(bbbbaaaa99998888 7777666655554444)
		y2 = _mm256_bslli_epi128(y1, 4);                         //y2 = sip(aaaa99998888xxxx 666655554444xxxx)
		y3 = _mm256_bsrli_epi128(y1, 4);                         //y3 = sip(xxxxbbbbaaaa9999 xxxx777766665555)
		y2 = _mm256_blend_epi32(y2, y3, 0x0f);                   //y2 = sip(aaaa99998888xxxx xxxx777766665555)
		y1 = _mm256_blendv_epi8(y1, y2, ySIPMASK(1));            //y1 = sip(aaaa999999888888 7777776666665555)

		y4 = _mm256_loadu_si256((__m256i*)(ptr_src2+32));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src1+32));
		y2 = y4;
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src3+32)));
		y4 = _mm256_slli_epi16(y4, 1);
		y3 = _mm256_adds_epi16(y3, y4);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix2);
		y3 = _mm256_srai_epi16(y3, 2);
		y1 = _mm256_blendv_epi8(y2, y3, y1); //y1 = sip ? y3 : y2;
		_mm256_storeu_si256((__m256i*)(ptr_dst+32), y1);

/*
blendや_mm256_permute2x128_si256では、上に出てきたものがa

blend r = (mask) ? b : a

sip(ffeeddccbbaa9988 7766554433221100)
sip(ffeeddccffeeddcc ffeeddccbbaa9988) _mm256_permute4x64_epi64, _MM_SHUFFLE(3,3,3,2)
sip(xxxxxffeeddccffe xxxffeeddccbbaa9) _mm256_bsrli_epi128, 3
sip(ffeeddccffeeddcc xxxffeeddccbbaa9) _mm256_blend_epi32, 0x0f
sip(ffffeeeeddddcccc ddccccbbbbaaaa99) _mm256_unpacklo_epi16
sip(xxxxffffeeeedddd xxxxddccccbbbbaa) _mm256_bsrli_epi128, 4
sip(ffffffeeeeeedddd ddccccccbbbbbbaa) _mm256_blendv_epi8
   (0000110011111111 0000001100111111) blend mask
*/
		y2 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3,3,3,2)); //y2 = sip(ffeeddccffeeddcc ffeeddccbbaa9988)
		y3 = _mm256_bsrli_epi128(y2, 3);                         //y3 = sip(xxxxxffeeddccffe xxxffeeddccbbaa9)
		y2 = _mm256_blend_epi32(y2, y3, 0x0f);                   //y2 = sip(ffeeddccffeeddcc xxxffeeddccbbaa9)
		y2 = _mm256_unpacklo_epi8(y2, y2);                       //y2 = sip(ffffeeeeddddcccc ddccccbbbbaaaa99)
		y3 = _mm256_bsrli_epi128(y2, 4);                         //y3 = sip(xxxxffffeeeedddd xxxxddccccbbbbaa)
		y1 = _mm256_blendv_epi8(y2, y3, ySIPMASK(2));            //y1 = sip(ffffffeeeeeedddd ddccccccbbbbbbaa)

		y4 = _mm256_loadu_si256((__m256i*)(ptr_src2+64));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src1+64));
		y2 = y4;
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src3+64)));
		y4 = _mm256_slli_epi16(y4, 1);
		y3 = _mm256_adds_epi16(y3, y4);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix2);
		y3 = _mm256_srai_epi16(y3, 2);
		y1 = _mm256_blendv_epi8(y2, y3, y1); //y1 = sip ? y3 : y2;
		_mm256_storeu_si256((__m256i*)(ptr_dst+64), y1);

		step = limit_1_to_16(iw);
	}
    _mm256_zeroupper();
}


void __stdcall afs_mie_spot_avx2( PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w) {
	BYTE *ptr_dst  = (BYTE *)dst;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src2 = (BYTE *)src2;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_src4 = (BYTE *)src4;
	BYTE *ptr_src_spot = (BYTE *)src_spot;
	__m256i y0, y1, y2, y3, y4, y5;
	const __m256i yPwRoundFix1 = _mm256_load_si256((__m256i*)pw_round_fix1);
	const __m256i yPwRoundFix2 = _mm256_load_si256((__m256i*)pw_round_fix2);

	for (int step = 0, iw = w - 16; iw >= 0; ptr_src1 += step*6, ptr_src2 += step*6, ptr_src3 += step*6, ptr_src4 += step*6, ptr_src_spot += step*6, ptr_dst += step*6, iw -= step) {
		y0 = _mm256_loadu_si256((__m256i*)(ptr_src1 +  0));
		y1 = _mm256_loadu_si256((__m256i*)(ptr_src1 + 32));
		y2 = _mm256_loadu_si256((__m256i*)(ptr_src1 + 64));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src3 +  0));
		y4 = _mm256_loadu_si256((__m256i*)(ptr_src3 + 32));
		y5 = _mm256_loadu_si256((__m256i*)(ptr_src3 + 64));
		y0 = _mm256_adds_epi16(y0, _mm256_loadu_si256((__m256i*)(ptr_src2 +  0)));
		y1 = _mm256_adds_epi16(y1, _mm256_loadu_si256((__m256i*)(ptr_src2 + 32)));
		y2 = _mm256_adds_epi16(y2, _mm256_loadu_si256((__m256i*)(ptr_src2 + 64)));
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src4 +  0)));
		y4 = _mm256_adds_epi16(y4, _mm256_loadu_si256((__m256i*)(ptr_src4 + 32)));
		y5 = _mm256_adds_epi16(y5, _mm256_loadu_si256((__m256i*)(ptr_src4 + 64)));
		y0 = _mm256_adds_epi16(y0, y3);
		y1 = _mm256_adds_epi16(y1, y4);
		y2 = _mm256_adds_epi16(y2, y5);
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src_spot +  0));
		y4 = _mm256_loadu_si256((__m256i*)(ptr_src_spot + 32));
		y5 = _mm256_loadu_si256((__m256i*)(ptr_src_spot + 64));
		y0 = _mm256_adds_epi16(y0, yPwRoundFix2);
		y1 = _mm256_adds_epi16(y1, yPwRoundFix2);
		y2 = _mm256_adds_epi16(y2, yPwRoundFix2);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix1);
		y4 = _mm256_adds_epi16(y4, yPwRoundFix1);
		y5 = _mm256_adds_epi16(y5, yPwRoundFix1);
		y0 = _mm256_srai_epi16(y0, 2);
		y1 = _mm256_srai_epi16(y1, 2);
		y2 = _mm256_srai_epi16(y2, 2);
		y0 = _mm256_adds_epi16(y0, y3);
		y1 = _mm256_adds_epi16(y1, y4);
		y2 = _mm256_adds_epi16(y2, y5);
		y0 = _mm256_srai_epi16(y0, 1);
		y1 = _mm256_srai_epi16(y1, 1);
		y2 = _mm256_srai_epi16(y2, 1);
		_mm256_storeu_si256((__m256i*)(ptr_dst +  0), y0);
		_mm256_storeu_si256((__m256i*)(ptr_dst + 32), y1);
		_mm256_storeu_si256((__m256i*)(ptr_dst + 64), y2);
		step = limit_1_to_16(iw);
	}
    _mm256_zeroupper();
}

void __stdcall afs_mie_inter_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src2 = (BYTE *)src2;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_src4 = (BYTE *)src4;
	__m256i y0, y1, y2, y3, y4, y5;
	const __m256i yPwRoundFix2 = _mm256_load_si256((__m256i*)pw_round_fix2);

	for (int step = 0, iw = w - 16; iw >= 0; ptr_src1 += step*6, ptr_src2 += step*6, ptr_src3 += step*6, ptr_src4 += step*6, ptr_dst += step*6, iw -= step) {
		y0 = _mm256_loadu_si256((__m256i*)(ptr_src1 +  0));
		y1 = _mm256_loadu_si256((__m256i*)(ptr_src1 + 32));
		y2 = _mm256_loadu_si256((__m256i*)(ptr_src1 + 64));

		y3 = _mm256_loadu_si256((__m256i*)(ptr_src3 +  0));
		y4 = _mm256_loadu_si256((__m256i*)(ptr_src3 + 32));
		y5 = _mm256_loadu_si256((__m256i*)(ptr_src3 + 64));
		y0 = _mm256_adds_epi16(y0, _mm256_loadu_si256((__m256i*)(ptr_src2 +  0)));
		y1 = _mm256_adds_epi16(y1, _mm256_loadu_si256((__m256i*)(ptr_src2 + 32)));
		y2 = _mm256_adds_epi16(y2, _mm256_loadu_si256((__m256i*)(ptr_src2 + 64)));
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src4 +  0)));
		y4 = _mm256_adds_epi16(y4, _mm256_loadu_si256((__m256i*)(ptr_src4 + 32)));
		y5 = _mm256_adds_epi16(y5, _mm256_loadu_si256((__m256i*)(ptr_src4 + 64)));
		y0 = _mm256_adds_epi16(y0, y3);
		y1 = _mm256_adds_epi16(y1, y4);
		y2 = _mm256_adds_epi16(y2, y5);

		y0 = _mm256_adds_epi16(y0, yPwRoundFix2);
		y1 = _mm256_adds_epi16(y1, yPwRoundFix2);
		y2 = _mm256_adds_epi16(y2, yPwRoundFix2);
		y0 = _mm256_srai_epi16(y0, 2);
		y1 = _mm256_srai_epi16(y1, 2);
		y2 = _mm256_srai_epi16(y2, 2);
		_mm256_storeu_si256((__m256i*)(ptr_dst +  0), y0);
		_mm256_storeu_si256((__m256i*)(ptr_dst + 32), y1);
		_mm256_storeu_si256((__m256i*)(ptr_dst + 64), y2);
		step = limit_1_to_16(iw);
	}
    _mm256_zeroupper();
}

void __stdcall afs_deint4_avx2(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_sip = (BYTE *)sip;
	BYTE *ptr_src1 = (BYTE *)src1;
	BYTE *ptr_src3 = (BYTE *)src3;
	BYTE *ptr_src4 = (BYTE *)src4;
	BYTE *ptr_src5 = (BYTE *)src5;
	BYTE *ptr_src7 = (BYTE *)src7;
	__m256i y0, y1, y2, y3, y4;
	const __m256i yMask = _mm256_unpacklo_epi8(_mm256_set1_epi32(mask), _mm256_setzero_si256());
	const __m256i yPwRoundFix1 = _mm256_load_si256((__m256i*)pw_round_fix1);

	for (int step = 0, iw = w - 16; iw >= 0; ptr_src4 += step*6, ptr_dst += step*6, ptr_src3 += step*6, ptr_src5 += step*6, ptr_src1 += step*6, ptr_src7 += step*6, ptr_sip += step, iw -= step) {
		y0 = _mm256_loadu_si256((__m256i*)(ptr_sip));
		y4 = _mm256_setzero_si256();
		y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(1,1,0,0));
		y0 = _mm256_unpacklo_epi8(y0, y4);
		y0 = _mm256_and_si256(y0, yMask);
		y0 = _mm256_cmpeq_epi16(y0, y4); //sip(7766554433221100)

/*
blendや_mm256_permute2x128_si256では、上に出てきたものがa

blend r = (mask) ? b : a

sip(ffeeddccbbaa9988 7766554433221100)
sip(7766554433221100 7766554433221100) _mm256_permute4x64_epi64, _MM_SHUFFLE(1,0,1,0)
sip(xxxxx77665544332 xxxxx77665544332) _mm256_bsrli_epi128, 5
sip(xxxxx77665544332 7766554433221100) _mm256_blend_epi32, 0xf0
sip(6655554444333322 3333222211110000) _mm256_unpacklo_epi16
sip(554444333322xxxx 222211110000xxxx) _mm256_bslli_epi128, 4
sip(5544444433333322 2222111111000000) _mm256_blendv_epi8
   (1111110011000000 1111111100110000) blend mask
*/

		y2 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(1,0,1,0)); //y2 = sip(7766554433221100 7766554433221100)
		y3 = _mm256_bsrli_epi128(y2, 5);                         //y3 = sip(xxxxx77665544332 xxxxx77665544332)
		y2 = _mm256_blend_epi32(y2, y3, 0xf0);                   //y2 = sip(xxxxx77665544332 7766554433221100)
		y2 = _mm256_unpacklo_epi8(y2, y2);                       //y2 = sip(6655554444333322 3333222211110000)
		y3 = _mm256_bslli_epi128(y2, 4);                         //y3 = sip(554444333322xxxx 222211110000xxxx)
		y1 = _mm256_blendv_epi8(y2, y3, ySIPMASK(0));            //y1 = sip(5544444433333322 2222111111000000)

		y2 = _mm256_loadu_si256((__m256i*)(ptr_src1));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src3));
		y2 = _mm256_adds_epi16(y2, _mm256_loadu_si256((__m256i*)(ptr_src7)));
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src5)));
		y2 = _mm256_subs_epi16(y2, y3);
		y2 = _mm256_srai_epi16(y2, 3);
		y3 = _mm256_subs_epi16(y3, y2);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix1);
		y3 = _mm256_srai_epi16(y3, 1);
		y2 = _mm256_loadu_si256((__m256i*)(ptr_src4));
		y1 = _mm256_blendv_epi8(y2, y3, y1); //y1 = sip ? y3 : y2;
		_mm256_storeu_si256((__m256i*)(ptr_dst), y1);

/*
blendや_mm256_permute2x128_si256では、上に出てきたものがa

blend r = (mask) ? b : a

sip(ffeeddccbbaa9988 7766554433221100)
sip(bbaa9988bbaa9988 7766554477665544) _mm256_permute4x64_epi64, _MM_SHUFFLE(2,2,1,1)
sip(bbbbaaaa99998888 7777666655554444) _mm256_unpacklo_epi16
sip(aaaa99998888xxxx 666655554444xxxx) _mm256_bslli_epi128, 4
sip(xxxxbbbbaaaa9999 xxxx777766665555) _mm256_bsrli_epi128, 4
sip(aaaa99998888xxxx xxxx777766665555) _mm256_blend_epi32, 0x0f
sip(aaaa999999888888 7777776666665555) _mm256_blendv_epi8
   (1111111100110000 0000110011111111) blend mask
*/
		y1 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(2,2,1,1)); //y1 = sip(bbaa9988bbaa9988 7766554477665544)
		y1 = _mm256_unpacklo_epi16(y1, y1);                      //y1 = sip(bbbbaaaa99998888 7777666655554444)
		y2 = _mm256_bslli_epi128(y1, 4);                         //y2 = sip(aaaa99998888xxxx 666655554444xxxx)
		y3 = _mm256_bsrli_epi128(y1, 4);                         //y3 = sip(xxxxbbbbaaaa9999 xxxx777766665555)
		y2 = _mm256_blend_epi32(y2, y3, 0x0f);                   //y2 = sip(aaaa99998888xxxx xxxx777766665555)
		y1 = _mm256_blendv_epi8(y1, y2, ySIPMASK(1));            //y1 = sip(aaaa999999888888 7777776666665555)

		y2 = _mm256_loadu_si256((__m256i*)(ptr_src1+32));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src3+32));
		y2 = _mm256_adds_epi16(y2, _mm256_loadu_si256((__m256i*)(ptr_src7+32)));
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src5+32)));
		y2 = _mm256_subs_epi16(y2, y3);
		y2 = _mm256_srai_epi16(y2, 3);
		y3 = _mm256_subs_epi16(y3, y2);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix1);
		y3 = _mm256_srai_epi16(y3, 1);
		y2 = _mm256_loadu_si256((__m256i*)(ptr_src4+32));
		y1 = _mm256_blendv_epi8(y2, y3, y1); //y1 = sip ? y3 : y2;
		_mm256_storeu_si256((__m256i*)(ptr_dst+32), y1);

/*
blendや_mm256_permute2x128_si256では、上に出てきたものがa

blend r = (mask) ? b : a

sip(ffeeddccbbaa9988 7766554433221100)
sip(ffeeddccffeeddcc ffeeddccbbaa9988) _mm256_permute4x64_epi64, _MM_SHUFFLE(3,3,3,2)
sip(xxxxxffeeddccffe xxxffeeddccbbaa9) _mm256_bsrli_epi128, 3
sip(ffeeddccffeeddcc xxxffeeddccbbaa9) _mm256_blend_epi32, 0x0f
sip(ffffeeeeddddcccc ddccccbbbbaaaa99) _mm256_unpacklo_epi16
sip(xxxxffffeeeedddd xxxxddccccbbbbaa) _mm256_bsrli_epi128, 4
sip(ffffffeeeeeedddd ddccccccbbbbbbaa) _mm256_blendv_epi8
   (0000110011111111 0000001100111111) blend mask
*/
		y2 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3,3,3,2)); //y2 = sip(ffeeddccffeeddcc ffeeddccbbaa9988)
		y3 = _mm256_bsrli_epi128(y2, 3);                         //y3 = sip(xxxxxffeeddccffe xxxffeeddccbbaa9)
		y2 = _mm256_blend_epi32(y2, y3, 0x0f);                   //y2 = sip(ffeeddccffeeddcc xxxffeeddccbbaa9)
		y2 = _mm256_unpacklo_epi8(y2, y2);                       //y2 = sip(ffffeeeeddddcccc ddccccbbbbaaaa99)
		y3 = _mm256_bsrli_epi128(y2, 4);                         //y3 = sip(xxxxffffeeeedddd xxxxddccccbbbbaa)
		y1 = _mm256_blendv_epi8(y2, y3, ySIPMASK(2));            //y1 = sip(ffffffeeeeeedddd ddccccccbbbbbbaa)

		y2 = _mm256_loadu_si256((__m256i*)(ptr_src1+64));
		y3 = _mm256_loadu_si256((__m256i*)(ptr_src3+64));
		y2 = _mm256_adds_epi16(y2, _mm256_loadu_si256((__m256i*)(ptr_src7+64)));
		y3 = _mm256_adds_epi16(y3, _mm256_loadu_si256((__m256i*)(ptr_src5+64)));
		y2 = _mm256_subs_epi16(y2, y3);
		y2 = _mm256_srai_epi16(y2, 3);
		y3 = _mm256_subs_epi16(y3, y2);
		y3 = _mm256_adds_epi16(y3, yPwRoundFix1);
		y3 = _mm256_srai_epi16(y3, 1);
		y2 = _mm256_loadu_si256((__m256i*)(ptr_src4+64));
		y1 = _mm256_blendv_epi8(y2, y3, y1); //y1 = sip ? y3 : y2;
		_mm256_storeu_si256((__m256i*)(ptr_dst+64), y1);

		step = limit_1_to_16(iw);
	}
    _mm256_zeroupper();
}
