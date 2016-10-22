#pragma once

#include <stdint.h>
#include "afs.h"
#include "filter.h"

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

static void __forceinline __stdcall afs_get_stripe_count_simd(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
    static const _declspec(align(16)) BYTE STRIPE_COUNT_CHECK_MASK[][16] = {
        { 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50, 0x50 }, 
        { 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60 }, 
    };
    const int y_fin = scan_h - sp0->clip.bottom - ((scan_h - sp0->clip.top - sp0->clip.bottom) & 1);
    const DWORD check_mask[2] = { 0x50, 0x60 };
    __m128i xZero = _mm_setzero_si128();
    __m128i xMask, x0, x1;
    for(int pos_y = sp0->clip.top; pos_y < y_fin; pos_y++) {
        BYTE *sip = sp->map + pos_y * si_w + sp0->clip.left;
        const int first_field_flag = !is_latter_field(pos_y, sp0->tb_order);
        xMask = _mm_load_si128((__m128i*)STRIPE_COUNT_CHECK_MASK[first_field_flag]);
        const int x_count = scan_w - sp0->clip.right - sp0->clip.left;
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

static void __forceinline __stdcall afs_get_motion_count_simd(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
    static const _declspec(align(16)) BYTE MOTION_COUNT_CHECK[16] = {
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    };
    const int y_fin = scan_h - sp->clip.bottom - ((scan_h - sp->clip.top - sp->clip.bottom) & 1);
    __m128i xMotion = _mm_load_si128((__m128i *)MOTION_COUNT_CHECK);
    __m128i x0, x1;
    for(int pos_y = sp->clip.top; pos_y < y_fin; pos_y++) {
        BYTE *sip = sp->map + pos_y * si_w + sp->clip.left;
        const int is_latter_feild = is_latter_field(pos_y, sp->tb_order);
        const int x_count = scan_w - sp->clip.right - sp->clip.left;
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

#include "afs_convert_const.h"

static __forceinline void convert_range_y_yuy2_to_yc48(__m128i& x0, __m128i& x1) {
    x1 = _mm_unpackhi_epi8(x0, _mm_setzero_si128());
    x0 = _mm_unpacklo_epi8(x0, _mm_setzero_si128());
    x0 = _mm_slli_epi16(x0, 6);
    x1 = _mm_slli_epi16(x1, 6);
    x0 = _mm_mulhi_epi16(x0, _mm_set1_epi16(19152));
    x1 = _mm_mulhi_epi16(x1, _mm_set1_epi16(19152));
    x0 = _mm_sub_epi16(x0, _mm_set1_epi16(299));
    x1 = _mm_sub_epi16(x1, _mm_set1_epi16(299));
}

static __forceinline void convert_range_c_yuy2_to_yc48(__m128i& x0, __m128i& x1) {
    x1 = _mm_unpackhi_epi8(x0, _mm_setzero_si128());
    x0 = _mm_unpacklo_epi8(x0, _mm_setzero_si128());
    x0 = _mm_slli_epi16(x0, 6);
    x1 = _mm_slli_epi16(x1, 6);
    x0 = _mm_mulhi_epi16(x0, _mm_set1_epi16(18752));
    x1 = _mm_mulhi_epi16(x1, _mm_set1_epi16(18752));
    x0 = _mm_sub_epi16(x0, _mm_set1_epi16(2340));
    x1 = _mm_sub_epi16(x1, _mm_set1_epi16(2340));
}

static __forceinline __m128i afs_blend(const __m128i& msrc1, const __m128i& msrc2, const __m128i& msrc3, const __m128i& mmask) {
    __m128i x3, x4;
    x3 = _mm_add_epi16(msrc1, msrc3);
    x4 = _mm_add_epi16(msrc2, msrc2);
    x3 = _mm_add_epi16(x3, x4);
    x3 = _mm_add_epi16(x3, _mm_set1_epi16(2));
    x3 = _mm_srai_epi16(x3, 2);
    return _mm_blendv_epi8_simd(msrc2, x3, mmask); //x1 = sip ? x3 : x2;
}

static __forceinline void afs_mask_extend16(__m128i& msipa, __m128i& msipb, const __m128i& mmask8, uint8_t *sip) {
    msipa = _mm_load_si128((const __m128i *)sip);
    msipa = _mm_and_si128(msipa, mmask8);
    msipa = _mm_cmpeq_epi8(msipa, mmask8);
    msipb = _mm_unpackhi_epi8(msipa, msipa);
    msipa = _mm_unpacklo_epi8(msipa, msipa);
}

static __forceinline __m128i afs_mask_for_uv_16(const __m128i& msip) {
#if USE_SSE41
    return _mm_blend_epi16(msip, _mm_slli_epi32(msip, 16), 0x80+0x20+0x08+0x02);
#else
    __m128i mtemp0 = _mm_and_si128(msip, _mm_set1_epi32(0x0000ffff));
    __m128i mtemp1 = _mm_slli_epi32(msip, 16);
    return _mm_or_si128(mtemp0, mtemp1);
#endif
}

static __forceinline __m128i afs_uv_interp_lanczos(const __m128i& x0, const __m128i& x1, const __m128i& x2) {
    __m128i x3, x6, x7;
    x7 = x1;
    x6 = _mm_alignr_epi8_simd(x2, x1, 4);
    x7 = _mm_add_epi16(x7, x6);

    x6 = _mm_add_epi16(_mm_alignr_epi8_simd(x2, x1, 8), _mm_alignr_epi8_simd(x1, x0, 12));
    x6 = _mm_sub_epi16(x6, x7);
    x6 = _mm_srai_epi16(x6, 3);

    x3 = _mm_cmpeq_epi16(x1, x1);
    x3 = _mm_srli_epi16(x3, 15);
    x7 = _mm_add_epi16(x7, x3);
    x7 = _mm_sub_epi16(x7, x6);
    x7 = _mm_srai_epi16(x7, 1);
    return x7;
}

static __forceinline __m128i afs_uv_interp_linear(const __m128i& x1, const __m128i& x2) {
    __m128i x3, x4;
    x3 = _mm_alignr_epi8_simd(x2, x1, 4);
    x4 = _mm_add_epi16(x1, _mm_srli_epi16(_mm_cmpeq_epi16(x1, x1), 15));
    x3 = _mm_add_epi16(x3, x4);
    return _mm_srai_epi16(x3, 1);
}

static __forceinline void afs_pack_yc48(__m128i& x0, __m128i& x1, __m128i& x2, const __m128i& xY, const __m128i& xC0, __m128i& xC1) {
    __m128i xYtemp, xCtemp0, xCtemp1;
#if USE_SSSE3
    xYtemp = _mm_shuffle_epi8(xY, xC_SUFFLE_YCP_Y); //52741630
#else
    //select y
    __m128i x6, x7;
    x6 = _mm_unpacklo_epi16(xY, xY);    //33221100
    x7 = _mm_unpackhi_epi16(xY, xY);    //77665544

    x6 = _mm_shuffle_epi32(x6, _MM_SHUFFLE(0, 2, 1, 3)); //00221133
    x6 = _mm_alignr_epi8_simd(x6, x6, 2);                //30022113
    x6 = _mm_shuffle_epi32(x6, _MM_SHUFFLE(2, 1, 0, 3)); //02211330

    x7 = _mm_shuffle_epi32(x7, _MM_SHUFFLE(0, 2, 1, 3)); //44665577
    x7 = _mm_alignr_epi8_simd(x7, x7, 2);                //74466557
    x7 = _mm_shuffle_epi32(x7, _MM_SHUFFLE(0, 3, 2, 1)); //57744665

    static const __declspec(align(16)) uint16_t MASK_Y[] ={
        0x0000, 0x0000, 0xffff, 0x0000, 0xffff, 0xffff, 0x0000, 0xffff,
    };
    xYtemp = select_by_mask(x6, x7, _mm_load_si128((const __m128i *)MASK_Y)); //52741630
#endif
    xCtemp1 = _mm_shuffle_epi32(xC1, _MM_SHUFFLE(3,0,1,2));
    xCtemp0 = _mm_shuffle_epi32(xC0, _MM_SHUFFLE(1,2,3,0));
    xCtemp0 = _mm_alignr_epi8_simd(xCtemp0, xCtemp0, 14);
#if USE_SSE41
    x0 = _mm_blend_epi16(xYtemp, xCtemp0, 0x80+0x04+0x02);
    x1 = _mm_blend_epi16(xYtemp, xCtemp1, 0x08+0x04);
    x2 = _mm_blend_epi16(xYtemp, xCtemp0, 0x10+0x08);
    x2 = _mm_blend_epi16(x2,     xCtemp1, 0x80+0x40+0x02+0x01);
    x1 = _mm_blend_epi16(x1,     xCtemp0, 0x40+0x20+0x01);
    x0 = _mm_blend_epi16(x0,     xCtemp1, 0x20+0x10);
#else
    static const __declspec(align(16)) uint16_t MASK[] = {
        0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff,
        0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff
    };
    __m128i xMask = _mm_load_si128((const __m128i *)MASK);
    x0 = select_by_mask(xYtemp, xCtemp0, xMask);
    x1 = select_by_mask(xYtemp, xCtemp1, _mm_slli_si128(xMask, 2));
    x2 = select_by_mask(xYtemp, xCtemp0, _mm_slli_si128(xMask, 4));
    xMask = _mm_load_si128((const __m128i *)(MASK + 8));
    x2 = select_by_mask(x2, xCtemp1, xMask);
    x1 = select_by_mask(x1, xCtemp0, _mm_srli_si128(xMask, 2));
    x0 = select_by_mask(x0, xCtemp1, _mm_srli_si128(xMask, 4));
#endif
}

template<bool uv_upsample>
static __forceinline void afs_blend_nv16_simd_base(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 16;
    const __m128i mmask = _mm_set1_epi8(mask);
    __m128i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b;
    __m128i my0a, my0b, my1a, my1b, my2a, my2b, my3a, my3b;
    __m128i mc4a, mc4b, mc5a, mc5b, mc6a, mc6b, mc7a, mc7b;
    mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
    mc2a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels));
    mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));

    __m128i msipa, msipb;
    afs_mask_extend16(msipa, msipb, mmask, sip);

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc2a, mc2b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);

    //色差をブレンド
    mc0a = afs_blend(mc1a, mc2a, mc3a, afs_mask_for_uv_16(msipa));
    mc0b = afs_blend(mc1b, mc2b, mc3b, afs_mask_for_uv_16(msipb));
    mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));

    for (; src1 < src1_fin; src1 += 16, src2 += 16, src3 += 16, dst += 16, sip += 16) {
        my1a = _mm_loadu_si128((const __m128i *)src1);
        my2a = _mm_loadu_si128((const __m128i *)src2);
        my3a = _mm_loadu_si128((const __m128i *)src3);
        mc5a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels + 16));
        mc6a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels + 16));
        mc7a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels + 16));

        //YUY2->YC48に変換
        convert_range_y_yuy2_to_yc48(my1a, my1b);
        convert_range_y_yuy2_to_yc48(my2a, my2b);
        convert_range_y_yuy2_to_yc48(my3a, my3b);
        convert_range_c_yuy2_to_yc48(mc5a, mc5b);
        convert_range_c_yuy2_to_yc48(mc6a, mc6b);
        convert_range_c_yuy2_to_yc48(mc7a, mc7b);

        //輝度をまずブレンド
        my0a = afs_blend(my1a, my2a, my3a, msipa);
        my0b = afs_blend(my1b, my2b, my3b, msipb);

        //マスクを更新
        afs_mask_extend16(msipa, msipb, mmask, sip + 16);

        //色差をブレンド
        mc4a = afs_blend(mc5a, mc6a, mc7a, afs_mask_for_uv_16(msipa));
        mc4b = afs_blend(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipb));

        //UVは補間を行う
        if (uv_upsample) {
            mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
            mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
            mc00 = mc0b;
        } else {
            mc1a = afs_uv_interp_linear(mc0a, mc0b);
            mc1b = afs_uv_interp_linear(mc0b, mc4a);
        }

        //YC48を構築
        __m128i x0, x1, x2;
        afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

        _mm_stream_si128((__m128i*)((uint8_t *)dst +  0), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 16), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 32), x2);

        afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);
        
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 48), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 64), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 80), x2);

        mc0a = mc4a;
        mc0b = mc4b;
    }
    //終端処理
    int wfix = w & 15;
    if (wfix) {
        src1 -= 16 - wfix;
        src2 -= 16 - wfix;
        src3 -= 16 - wfix;
        dst  -= 16 - wfix;
        sip  -= 16 - wfix;

        mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
        mc2a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels));
        mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));

        __m128i msipa, msipb;
        afs_mask_extend16(msipa, msipb, mmask, sip);

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);

        //色差をブレンド
        mc0a = afs_blend(mc1a, mc2a, mc3a, afs_mask_for_uv_16(msipa));
        mc0b = afs_blend(mc1b, mc2b, mc3b, afs_mask_for_uv_16(msipb));
        mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));
    }
    my1a = _mm_loadu_si128((const __m128i *)src1);
    my2a = _mm_loadu_si128((const __m128i *)src2);
    my3a = _mm_loadu_si128((const __m128i *)src3);

    //YUY2->YC48に変換
    convert_range_y_yuy2_to_yc48(my1a, my1b);
    convert_range_y_yuy2_to_yc48(my2a, my2b);
    convert_range_y_yuy2_to_yc48(my3a, my3b);
    convert_range_c_yuy2_to_yc48(mc5a, mc5b);
    convert_range_c_yuy2_to_yc48(mc6a, mc6b);
    convert_range_c_yuy2_to_yc48(mc7a, mc7b);

    //輝度をまずブレンド
    my0a = afs_blend(my1a, my2a, my3a, msipa);
    my0b = afs_blend(my1b, my2b, my3b, msipb);

    ////マスクを更新
    //afs_mask_extend16(msipa, msipb, mmask, sip + 16);

    ////色差をブレンド
    //mc4a = afs_blend(mc5a, mc6a, mc7a, afs_mask_for_uv_16(msipa));
    //mc4b = afs_blend(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipb));
    mc4a = _mm_shuffle_epi32(my0b, _MM_SHUFFLE(3, 3, 3, 3));

    //UVは補間を行う
    if (uv_upsample) {
        mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
        mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
        mc00 = mc0b;
    } else {
        mc1a = afs_uv_interp_linear(mc0a, mc0b);
        mc1b = afs_uv_interp_linear(mc0b, mc4a);
    }

    //YC48を構築
    __m128i x0, x1, x2;
    afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst +  0), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 16), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 32), x2);

    afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 48), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 64), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 80), x2);
}

static void __forceinline __stdcall afs_blend_nv16up_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16_simd_base<true>(dst, src1, src2, src3, sip, mask, w, src_frame_pixels);
}

static void __forceinline __stdcall afs_blend_nv16_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16_simd_base<false>(dst, src1, src2, src3, sip, mask, w, src_frame_pixels);
}

static __forceinline __m128i afs_mie_spot(const __m128i& msrc1, const __m128i& msrc2, const __m128i& msrc3, const __m128i& msrc4, const __m128i& msrc_spot) {
    __m128i x0, x1, x2;
    x0 = _mm_add_epi16(msrc1, msrc2);
    x1 = _mm_add_epi16(msrc3, msrc4);
    x0 = _mm_add_epi16(x0, x1);
    x0 = _mm_add_epi16(x0, _mm_set1_epi16(2));
    x0 = _mm_srai_epi16(x0, 2);
    x2 = _mm_add_epi16(msrc_spot, _mm_set1_epi16(1));
    x0 = _mm_add_epi16(x0, x2);
    x0 = _mm_srai_epi16(x0, 1);
    return x0;
}

template<bool uv_upsample>
static __forceinline void afs_mie_spot_nv16_simd_base(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, uint8_t *src_spot, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 16;
    //コンパイラ頑張れ
    __m128i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b, mc4a, mc4b, mcsa, mcsb;
    __m128i my0a, my0b, my1a, my1b, my2a, my2b, my3a, my3b, my4a, my4b, mysa, mysb;
    __m128i mc5a, mc5b, mc6a, mc6b, mc7a, mc7b, mc8a, mc8b, mc9a, mc9b;
    mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
    mc2a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels));
    mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));
    mc4a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels));
    mcsa = _mm_loadu_si128((const __m128i *)(src_spot + src_frame_pixels));

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc2a, mc2b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);
    convert_range_c_yuy2_to_yc48(mc4a, mc4b);
    convert_range_c_yuy2_to_yc48(mcsa, mcsb);

    //色差をブレンド
    mc0a = afs_mie_spot(mc1a, mc2a, mc3a, mc4a, mcsa);
    mc0b = afs_mie_spot(mc1b, mc2b, mc3b, mc4b, mcsb);
    mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));

    for (; src1 < src1_fin; src1 += 16, src2 += 16, src3 += 16, src4 += 16, src_spot += 16, dst += 16) {
        my1a = _mm_loadu_si128((const __m128i *)src1);
        my2a = _mm_loadu_si128((const __m128i *)src2);
        my3a = _mm_loadu_si128((const __m128i *)src3);
        my4a = _mm_loadu_si128((const __m128i *)src4);
        mysa = _mm_loadu_si128((const __m128i *)src_spot);
        mc6a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels + 16));
        mc7a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels + 16));
        mc8a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels + 16));
        mc9a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels + 16));
        mcsa = _mm_loadu_si128((const __m128i *)(src_spot + src_frame_pixels + 16));

        //YUY2->YC48に変換
        convert_range_y_yuy2_to_yc48(my1a, my1b);
        convert_range_y_yuy2_to_yc48(my2a, my2b);
        convert_range_y_yuy2_to_yc48(my3a, my3b);
        convert_range_y_yuy2_to_yc48(my4a, my4b);
        convert_range_y_yuy2_to_yc48(mysa, mysb);
        convert_range_c_yuy2_to_yc48(mc6a, mc6b);
        convert_range_c_yuy2_to_yc48(mc7a, mc7b);
        convert_range_c_yuy2_to_yc48(mc8a, mc8b);
        convert_range_c_yuy2_to_yc48(mc9a, mc9b);
        convert_range_c_yuy2_to_yc48(mcsa, mcsb);

        //輝度をまずブレンド
        my0a = afs_mie_spot(my1a, my2a, my3a, my4a, mysa);
        my0b = afs_mie_spot(my1b, my2b, my3b, my4b, mysb);

        //色差をブレンド
        mc5a = afs_mie_spot(mc6a, mc7a, mc8a, mc9a, mcsa);
        mc5b = afs_mie_spot(mc6b, mc7b, mc8b, mc9b, mcsb);

        //UVは補間を行う
        if (uv_upsample) {
            mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
            mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc5a);
            mc00 = mc0b;
        } else {
            mc1a = afs_uv_interp_linear(mc0a, mc0b);
            mc1b = afs_uv_interp_linear(mc0b, mc5a);
        }

        //YC48を構築
        __m128i x0, x1, x2;
        afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

        _mm_stream_si128((__m128i*)((uint8_t *)dst +  0), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 16), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 32), x2);

        afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

        _mm_stream_si128((__m128i*)((uint8_t *)dst + 48), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 64), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 80), x2);

        mc0a = mc5a;
        mc0b = mc5b;
    }
    //終端処理
    int wfix = w & 15;
    if (wfix) {
        src1 -= 16 - wfix;
        src2 -= 16 - wfix;
        src3 -= 16 - wfix;
        src4 -= 16 - wfix;
        src_spot -= 16 - wfix;
        dst  -= 16 - wfix;

        mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
        mc2a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels));
        mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));
        mc4a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels));
        mcsa = _mm_loadu_si128((const __m128i *)(src_spot + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);
        convert_range_c_yuy2_to_yc48(mcsa, mcsb);

        //色差をブレンド
        mc0a = afs_mie_spot(mc1a, mc2a, mc3a, mc4a, mcsa);
        mc0b = afs_mie_spot(mc1b, mc2b, mc3b, mc4b, mcsb);
        mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));
    }
    my1a = _mm_loadu_si128((const __m128i *)src1);
    my2a = _mm_loadu_si128((const __m128i *)src2);
    my3a = _mm_loadu_si128((const __m128i *)src3);
    my4a = _mm_loadu_si128((const __m128i *)src4);
    mysa = _mm_loadu_si128((const __m128i *)src_spot);

    //YUY2->YC48に変換
    convert_range_y_yuy2_to_yc48(my1a, my1b);
    convert_range_y_yuy2_to_yc48(my2a, my2b);
    convert_range_y_yuy2_to_yc48(my3a, my3b);
    convert_range_y_yuy2_to_yc48(my4a, my4b);
    convert_range_y_yuy2_to_yc48(mysa, mysb);
    convert_range_c_yuy2_to_yc48(mc6a, mc6b);
    convert_range_c_yuy2_to_yc48(mc7a, mc7b);
    convert_range_c_yuy2_to_yc48(mc8a, mc8b);
    convert_range_c_yuy2_to_yc48(mc9a, mc9b);
    convert_range_c_yuy2_to_yc48(mcsa, mcsb);

    //輝度をまずブレンド
    my0a = afs_mie_spot(my1a, my2a, my3a, my4a, mysa);
    my0b = afs_mie_spot(my1b, my2b, my3b, my4b, mysb);

    ////色差をブレンド
    //mc4a = afs_mie_spot(mc5a, mc6a, mc7a, afs_mask_for_uv_16(msipa));
    //mc4b = afs_mie_spot(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipa));
    mc4a = _mm_shuffle_epi32(my0b, _MM_SHUFFLE(3, 3, 3, 3));

    //UVは補間を行う
    if (uv_upsample) {
        mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
        mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
        mc00 = mc0b;
    } else {
        mc1a = afs_uv_interp_linear(mc0a, mc0b);
        mc1b = afs_uv_interp_linear(mc0b, mc4a);
    }

    //YC48を構築
    __m128i x0, x1, x2;
    afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst +  0), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 16), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 32), x2);

    afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 48), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 64), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 80), x2);
}

static void __forceinline __stdcall afs_mie_spot_nv16up_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, uint8_t *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16_simd_base<true>(dst, src1, src2, src3, src4, src_spot, w, src_frame_pixels);
}

static void __forceinline __stdcall afs_mie_spot_nv16_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, uint8_t *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16_simd_base<false>(dst, src1, src2, src3, src4, src_spot, w, src_frame_pixels);
}


static __forceinline __m128i afs_mie_inter(const __m128i& msrc1, const __m128i& msrc2, const __m128i& msrc3, const __m128i& msrc4) {
    __m128i x0, x1;
    x0 = _mm_add_epi16(msrc1, msrc2);
    x1 = _mm_add_epi16(msrc3, msrc4);
    x0 = _mm_add_epi16(x0, x1);
    x0 = _mm_add_epi16(x0, _mm_set1_epi16(2));
    x0 = _mm_srai_epi16(x0, 2);
    return x0;
}

template<bool uv_upsample>
static __forceinline void afs_mie_inter_nv16_simd_base(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 16;
    //コンパイラ頑張れ
    __m128i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b, mc4a, mc4b;
    __m128i my0a, my0b, my1a, my1b, my2a, my2b, my3a, my3b, my4a, my4b;
    __m128i mc5a, mc5b, mc6a, mc6b, mc7a, mc7b, mc8a, mc8b, mc9a, mc9b;
    mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
    mc2a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels));
    mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));
    mc4a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels));

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc2a, mc2b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);
    convert_range_c_yuy2_to_yc48(mc4a, mc4b);

    //色差をブレンド
    mc0a = afs_mie_inter(mc1a, mc2a, mc3a, mc4a);
    mc0b = afs_mie_inter(mc1b, mc2b, mc3b, mc4b);
    mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));

    for (; src1 < src1_fin; src1 += 16, src2 += 16, src3 += 16, src4 += 16, dst += 16) {
        my1a = _mm_loadu_si128((const __m128i *)src1);
        my2a = _mm_loadu_si128((const __m128i *)src2);
        my3a = _mm_loadu_si128((const __m128i *)src3);
        my4a = _mm_loadu_si128((const __m128i *)src4);
        mc6a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels + 16));
        mc7a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels + 16));
        mc8a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels + 16));
        mc9a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels + 16));

        //YUY2->YC48に変換
        convert_range_y_yuy2_to_yc48(my1a, my1b);
        convert_range_y_yuy2_to_yc48(my2a, my2b);
        convert_range_y_yuy2_to_yc48(my3a, my3b);
        convert_range_y_yuy2_to_yc48(my4a, my4b);
        convert_range_c_yuy2_to_yc48(mc6a, mc6b);
        convert_range_c_yuy2_to_yc48(mc7a, mc7b);
        convert_range_c_yuy2_to_yc48(mc8a, mc8b);
        convert_range_c_yuy2_to_yc48(mc9a, mc9b);

        //輝度をまずブレンド
        my0a = afs_mie_inter(my1a, my2a, my3a, my4a);
        my0b = afs_mie_inter(my1b, my2b, my3b, my4b);

        //色差をブレンド
        mc5a = afs_mie_inter(mc6a, mc7a, mc8a, mc9a);
        mc5b = afs_mie_inter(mc6b, mc7b, mc8b, mc9b);

        //UVは補間を行う
        if (uv_upsample) {
            mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
            mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc5a);
            mc00 = mc0b;
        } else {
            mc1a = afs_uv_interp_linear(mc0a, mc0b);
            mc1b = afs_uv_interp_linear(mc0b, mc5a);
        }

        //YC48を構築
        __m128i x0, x1, x2;
        afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

        _mm_stream_si128((__m128i*)((uint8_t *)dst +  0), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 16), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 32), x2);

        afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

        _mm_stream_si128((__m128i*)((uint8_t *)dst + 48), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 64), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 80), x2);

        mc0a = mc5a;
        mc0b = mc5b;
    }
    //終端処理
    int wfix = w & 15;
    if (wfix) {
        src1 -= 16 - wfix;
        src2 -= 16 - wfix;
        src3 -= 16 - wfix;
        src4 -= 16 - wfix;
        dst  -= 16 - wfix;

        mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
        mc2a = _mm_loadu_si128((const __m128i *)(src2 + src_frame_pixels));
        mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));
        mc4a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);

        //色差をブレンド
        mc0a = afs_mie_inter(mc1a, mc2a, mc3a, mc4a);
        mc0b = afs_mie_inter(mc1b, mc2b, mc3b, mc4b);
        mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));
    }
    my1a = _mm_loadu_si128((const __m128i *)src1);
    my2a = _mm_loadu_si128((const __m128i *)src2);
    my3a = _mm_loadu_si128((const __m128i *)src3);
    my4a = _mm_loadu_si128((const __m128i *)src4);

    //YUY2->YC48に変換
    convert_range_y_yuy2_to_yc48(my1a, my1b);
    convert_range_y_yuy2_to_yc48(my2a, my2b);
    convert_range_y_yuy2_to_yc48(my3a, my3b);
    convert_range_y_yuy2_to_yc48(my4a, my4b);
    convert_range_c_yuy2_to_yc48(mc6a, mc6b);
    convert_range_c_yuy2_to_yc48(mc7a, mc7b);
    convert_range_c_yuy2_to_yc48(mc8a, mc8b);
    convert_range_c_yuy2_to_yc48(mc9a, mc9b);

    //輝度をまずブレンド
    my0a = afs_mie_inter(my1a, my2a, my3a, my4a);
    my0b = afs_mie_inter(my1b, my2b, my3b, my4b);

    ////色差をブレンド
    //mc4a = afs_mie_inter(mc5a, mc6a, mc7a, afs_mask_for_uv_16(msipa));
    //mc4b = afs_mie_inter(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipa));
    mc4a = _mm_shuffle_epi32(my0b, _MM_SHUFFLE(3, 3, 3, 3));

    //UVは補間を行う
    if (uv_upsample) {
        mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
        mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
        mc00 = mc0b;
    } else {
        mc1a = afs_uv_interp_linear(mc0a, mc0b);
        mc1b = afs_uv_interp_linear(mc0b, mc4a);
    }

    //YC48を構築
    __m128i x0, x1, x2;
    afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst +  0), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 16), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 32), x2);

    afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 48), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 64), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 80), x2);
}

static void __forceinline __stdcall afs_mie_inter_nv16up_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16_simd_base<true>(dst, src1, src2, src3, src4, w, src_frame_pixels);
}

static void __forceinline __stdcall afs_mie_inter_nv16_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16_simd_base<false>(dst, src1, src2, src3, src4, w, src_frame_pixels);
}


static __forceinline __m128i afs_deint4(const __m128i& msrc1, const __m128i& msrc3, const __m128i& msrc4, const __m128i& msrc5, const __m128i& msrc7, const __m128i& mmask) {
    __m128i x2, x3;
    x2 = _mm_add_epi16(msrc1, msrc7);
    x3 = _mm_add_epi16(msrc3, msrc5);
    x2 = _mm_sub_epi16(x2, x3);
    x2 = _mm_srai_epi16(x2, 3);
    x3 = _mm_sub_epi16(x3, x2);
    x3 = _mm_add_epi16(x3, _mm_set1_epi16(1));
    x3 = _mm_srai_epi16(x3, 1);
    return _mm_blendv_epi8_simd(msrc4, x3, mmask); //x1 = sip ? x3 : x2;
}

template<bool uv_upsample>
static __forceinline void afs_deint4_nv16_simd_base(PIXEL_YC *dst, uint8_t *src1, uint8_t *src3, uint8_t *src4, uint8_t *src5, uint8_t *src7, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 16;
    const __m128i mmask = _mm_set1_epi8(mask);
    __m128i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b;
    __m128i my0a, my0b, my1a, my1b, my3a, my3b, my4a, my4b, my5a, my5b, my7a, my7b;
    __m128i mc4a, mc4b, mc5a, mc5b, mc6a, mc6b, mc7a, mc7b, mc8a, mc8b, mc9a, mc9b, mcaa, mcab, mcca, mccb;
    mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
    mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));
    mc4a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels));
    mc5a = _mm_loadu_si128((const __m128i *)(src5 + src_frame_pixels));
    mc7a = _mm_loadu_si128((const __m128i *)(src7 + src_frame_pixels));

    __m128i msipa, msipb;
    afs_mask_extend16(msipa, msipb, mmask, sip);

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);
    convert_range_c_yuy2_to_yc48(mc4a, mc4b);
    convert_range_c_yuy2_to_yc48(mc5a, mc5b);
    convert_range_c_yuy2_to_yc48(mc7a, mc7b);

    //色差をブレンド
    mc0a = afs_deint4(mc1a, mc3a, mc4a, mc5a, mc7a, afs_mask_for_uv_16(msipa));
    mc0b = afs_deint4(mc1b, mc3b, mc4b, mc5b, mc7b, afs_mask_for_uv_16(msipb));
    mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));

    for (; src1 < src1_fin; src1 += 16, src3 += 16, src4 += 16, src5 += 16, src7 += 16, dst += 16, sip += 16) {
        my1a = _mm_loadu_si128((const __m128i *)src1);
        my3a = _mm_loadu_si128((const __m128i *)src3);
        my4a = _mm_loadu_si128((const __m128i *)src4);
        my5a = _mm_loadu_si128((const __m128i *)src5);
        my7a = _mm_loadu_si128((const __m128i *)src7);
        mc6a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels + 16));
        mc8a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels + 16));
        mc9a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels + 16));
        mcaa = _mm_loadu_si128((const __m128i *)(src5 + src_frame_pixels + 16));
        mcca = _mm_loadu_si128((const __m128i *)(src7 + src_frame_pixels + 16));

        //YUY2->YC48に変換
        convert_range_y_yuy2_to_yc48(my1a, my1b);
        convert_range_y_yuy2_to_yc48(my3a, my3b);
        convert_range_y_yuy2_to_yc48(my4a, my4b);
        convert_range_y_yuy2_to_yc48(my5a, my5b);
        convert_range_y_yuy2_to_yc48(my7a, my7b);
        convert_range_c_yuy2_to_yc48(mc6a, mc6b);
        convert_range_c_yuy2_to_yc48(mc8a, mc8b);
        convert_range_c_yuy2_to_yc48(mc9a, mc9b);
        convert_range_c_yuy2_to_yc48(mcaa, mcab);
        convert_range_c_yuy2_to_yc48(mcca, mccb);

        //輝度をまずブレンド
        my0a = afs_deint4(my1a, my3a, my4a, my5a, my7a, msipa);
        my0b = afs_deint4(my1b, my3b, my4b, my5b, my7b, msipb);

        //マスクを更新
        afs_mask_extend16(msipa, msipb, mmask, sip + 16);

        //色差をブレンド
        mc2a = afs_deint4(mc6a, mc8a, mc9a, mcaa, mcca, afs_mask_for_uv_16(msipa));
        mc2b = afs_deint4(mc6b, mc8b, mc9b, mcaa, mcca, afs_mask_for_uv_16(msipb));

        //UVは補間を行う
        if (uv_upsample) {
            mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
            mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc2a);
            mc00 = mc0b;
        } else {
            mc1a = afs_uv_interp_linear(mc0a, mc0b);
            mc1b = afs_uv_interp_linear(mc0b, mc2a);
        }

        //YC48を構築
        __m128i x0, x1, x2;
        afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

        _mm_stream_si128((__m128i*)((uint8_t *)dst +  0), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 16), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 32), x2);

        afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

        _mm_stream_si128((__m128i*)((uint8_t *)dst + 48), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 64), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 80), x2);

        mc0a = mc2a;
        mc0b = mc2b;
    }
    //終端処理
    int wfix = w & 15;
    if (wfix) {
        src1 -= 16 - wfix;
        src3 -= 16 - wfix;
        src4 -= 16 - wfix;
        src5 -= 16 - wfix;
        src7 -= 16 - wfix;
        dst  -= 16 - wfix;
        sip  -= 16 - wfix;

        mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));
        mc3a = _mm_loadu_si128((const __m128i *)(src3 + src_frame_pixels));
        mc4a = _mm_loadu_si128((const __m128i *)(src4 + src_frame_pixels));
        mc5a = _mm_loadu_si128((const __m128i *)(src5 + src_frame_pixels));
        mc7a = _mm_loadu_si128((const __m128i *)(src7 + src_frame_pixels));

        __m128i msipa, msipb;
        afs_mask_extend16(msipa, msipb, mmask, sip);

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);
        convert_range_c_yuy2_to_yc48(mc5a, mc5b);
        convert_range_c_yuy2_to_yc48(mc7a, mc7b);

        //色差をブレンド
        mc0a = afs_deint4(mc1a, mc3a, mc4a, mc5a, mc7a, afs_mask_for_uv_16(msipa));
        mc0b = afs_deint4(mc1b, mc3b, mc4b, mc5b, mc7b, afs_mask_for_uv_16(msipb));
        mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));
    }
    my1a = _mm_loadu_si128((const __m128i *)src1);
    my3a = _mm_loadu_si128((const __m128i *)src3);
    my4a = _mm_loadu_si128((const __m128i *)src4);
    my5a = _mm_loadu_si128((const __m128i *)src5);
    my7a = _mm_loadu_si128((const __m128i *)src7);

    //YUY2->YC48に変換
    convert_range_y_yuy2_to_yc48(my1a, my1b);
    convert_range_y_yuy2_to_yc48(my3a, my3b);
    convert_range_y_yuy2_to_yc48(my4a, my4b);
    convert_range_y_yuy2_to_yc48(my5a, my5b);
    convert_range_y_yuy2_to_yc48(my7a, my7b);
    convert_range_c_yuy2_to_yc48(mc6a, mc6b);
    convert_range_c_yuy2_to_yc48(mc8a, mc8b);
    convert_range_c_yuy2_to_yc48(mc9a, mc9b);
    convert_range_c_yuy2_to_yc48(mcaa, mcab);
    convert_range_c_yuy2_to_yc48(mcca, mccb);

    //輝度をまずブレンド
    my0a = afs_deint4(my1a, my3a, my4a, my5a, my7a, msipa);
    my0b = afs_deint4(my1b, my3b, my4b, my5b, my7b, msipb);

    ////マスクを更新
    //afs_mask_extend16(msipa, msipb, mmask, sip + 16);

    ////色差をブレンド
    //mc4a = afs_deint4(mc5a, mc6a, mc7a, afs_mask_for_uv_16(msipa));
    //mc4b = afs_deint4(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipa));
    mc4a = _mm_shuffle_epi32(my0b, _MM_SHUFFLE(3, 3, 3, 3));

    //UVは補間を行う
    if (uv_upsample) {
        mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
        mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
        mc00 = mc0b;
    } else {
        mc1a = afs_uv_interp_linear(mc0a, mc0b);
        mc1b = afs_uv_interp_linear(mc0b, mc4a);
    }

    //YC48を構築
    __m128i x0, x1, x2;
    afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst +  0), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 16), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 32), x2);

    afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 48), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 64), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 80), x2);
}

static void __forceinline __stdcall afs_deint4_nv16up_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src3, uint8_t *src4, uint8_t *src5, uint8_t *src7, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_nv16_simd_base<true>(dst, src1, src3, src4, src5, src7, sip, mask, w, src_frame_pixels);
}

static void __forceinline __stdcall afs_deint4_nv16_simd(PIXEL_YC *dst, uint8_t *src1, uint8_t *src3, uint8_t *src4, uint8_t *src5, uint8_t *src7, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_deint4_nv16_simd_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, w, src_frame_pixels);
}

template<bool uv_upsample>
static __forceinline void afs_convert_nv16_yc48(PIXEL_YC *dst, uint8_t *src1, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 16;
    //コンパイラ頑張れ
    __m128i mc00, mc0a, mc0b, mc1a, mc1b;
    __m128i my0a, my0b, my1a, my1b;
    __m128i mc4a, mc4b, mc5a, mc5b;
    mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);

    //色差をブレンド
    mc0a = mc1a;
    mc0b = mc1b;
    mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0,0,0,0));

    for (; src1 < src1_fin; src1 += 16, dst += 16) {
        my1a = _mm_loadu_si128((const __m128i *)src1);
        mc5a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels + 16));

        //YUY2->YC48に変換
        convert_range_y_yuy2_to_yc48(my1a, my1b);
        convert_range_c_yuy2_to_yc48(mc5a, mc5b);

        my0a = my1a;
        my0b = my1b;
        mc4a = mc5a;
        mc4b = mc5b;

        //UVは補間を行う
        if (uv_upsample) {
            mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
            mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
            mc00 = mc0b;
        } else {
            mc1a = afs_uv_interp_linear(mc0a, mc0b);
            mc1b = afs_uv_interp_linear(mc0b, mc4a);
        }

        //YC48を構築
        __m128i x0, x1, x2;
        afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

        _mm_stream_si128((__m128i*)((uint8_t *)dst +  0), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 16), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 32), x2);

        afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

        _mm_stream_si128((__m128i*)((uint8_t *)dst + 48), x0);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 64), x1);
        _mm_stream_si128((__m128i*)((uint8_t *)dst + 80), x2);

        mc0a = mc4a;
        mc0b = mc4b;
    }
    //終端処理
    int wfix = w & 15;
    if (wfix) {
        src1 -= 16 - wfix;
        dst  -= 16 - wfix;

        mc1a = _mm_loadu_si128((const __m128i *)(src1 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);

        mc0a = mc1a;
        mc0b = mc1b;
        mc00 = _mm_shuffle_epi32(mc0a, _MM_SHUFFLE(0, 0, 0, 0));
    }
    my1a = _mm_loadu_si128((const __m128i *)src1);

    //YUY2->YC48に変換
    convert_range_y_yuy2_to_yc48(my1a, my1b);
    convert_range_c_yuy2_to_yc48(mc5a, mc5b);

    //輝度をまずブレンド
    my0a = my1a;
    my0b = my1b;

    ////マスクを更新
    //afs_mask_extend16(msipa, msipb, mmask, sip + 16);

    ////色差をブレンド
    //mc4a = afs_blend(mc5a, mc6a, mc7a, afs_mask_for_uv_16(msipa));
    //mc4b = afs_blend(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipa));
    mc4a = _mm_shuffle_epi32(my0b, _MM_SHUFFLE(3, 3, 3, 3));

    //UVは補間を行う
    if (uv_upsample) {
        mc1a = afs_uv_interp_lanczos(mc00, mc0a, mc0b);
        mc1b = afs_uv_interp_lanczos(mc0a, mc0b, mc4a);
        mc00 = mc0b;
    } else {
        mc1a = afs_uv_interp_linear(mc0a, mc0b);
        mc1b = afs_uv_interp_linear(mc0b, mc4a);
    }

    //YC48を構築
    __m128i x0, x1, x2;
    afs_pack_yc48(x0, x1, x2, my0a, mc0a, mc1a);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst +  0), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 16), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 32), x2);

    afs_pack_yc48(x0, x1, x2, my0b, mc0b, mc1b);

    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 48), x0);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 64), x1);
    _mm_storeu_si128((__m128i*)((uint8_t *)dst + 80), x2);
}

static void __forceinline __stdcall afs_convert_nv16_yc48up_simd(PIXEL_YC *dst, uint8_t *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48<true>(dst, src1, w, src_frame_pixels);
}

static void __forceinline __stdcall afs_convert_nv16_yc48_simd(PIXEL_YC *dst, uint8_t *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48<false>(dst, src1, w, src_frame_pixels);
}
