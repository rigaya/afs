#include <nmmintrin.h> //SSE4.2
#include <immintrin.h> //AVX, AVX2
#include <Windows.h>
#include "filter.h"
#include "afs.h"
#include "simd_util.h"

#if _MSC_VER >= 1800 && !defined(__AVX__) && !defined(_DEBUG)
static_assert(false, "do not forget to set /arch:AVX or /arch:AVX2 for this file.");
#endif

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
    const int y_fin = scan_h - sp0->clip.bottom - ((scan_h - sp0->clip.top - sp0->clip.bottom) & 1);
    const DWORD check_mask[2] = { 0x50, 0x60 };
    __m256i yMask, y0, y1;
    for(int pos_y = sp0->clip.top; pos_y < y_fin; pos_y++) {
        BYTE *sip = sp->map + pos_y * si_w + sp0->clip.left;
        const int first_field_flag = !is_latter_field(pos_y, sp0->tb_order);
        yMask = _mm256_load_si256((__m256i*)STRIPE_COUNT_CHECK_MASK[first_field_flag]);
        int x_count = scan_w - sp0->clip.right - sp0->clip.left;
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
    const int y_fin = scan_h - sp->clip.bottom - ((scan_h - sp->clip.top - sp->clip.bottom) & 1);
    __m256i yMotion = _mm256_load_si256((__m256i *)MOTION_COUNT_CHECK);
    __m256i y0, y1;
    for(int pos_y = sp->clip.top; pos_y < y_fin; pos_y++) {
        BYTE *sip = sp->map + pos_y * si_w + sp->clip.left;
        const int is_latter_feild = is_latter_field(pos_y, sp->tb_order);
        int x_count = scan_w - sp->clip.right - sp->clip.left;
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

template <bool aligned_store>
void __forceinline __stdcall afs_blend_avx2_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
    BYTE *ptr_dst  = (BYTE *)dst;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src2 = (BYTE *)src2;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_esi  = (BYTE *)sip;
    __m256i y0, y1, y2, y3, y4;
    const __m256i yMask = _mm256_unpacklo_epi8(_mm256_set1_epi32(mask), _mm256_setzero_si256());
    const __m256i yPwRoundFix2 = _mm256_load_si256((__m256i*)pw_round_fix2);

    for (int iw = w - 16; iw >= 0; ptr_src2 += 96, ptr_dst += 96, ptr_src1 += 96, ptr_src3 += 96, ptr_esi += 16, iw -= 16) {
        y0 = _mm256_loadu_si256((__m256i*)(ptr_esi));
        y4 = _mm256_setzero_si256();
        y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(1,1,0,0));
        y0 = _mm256_unpacklo_epi8(y0, y4);
        y0 = _mm256_and_si256(y0, yMask);
        y0 = _mm256_cmpeq_epi16(y0, y4); //sip(ffeeddbbccbbaa99887766554433221100)

        _mm_prefetch((char *)ptr_src1 + 576, _MM_HINT_NTA);
        _mm_prefetch((char *)ptr_src2 + 576, _MM_HINT_NTA);
        _mm_prefetch((char *)ptr_src3 + 576, _MM_HINT_NTA);
        _mm_prefetch((char *)ptr_esi  +  96, _MM_HINT_NTA);
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
        _mm256_stream_switch_si256((__m256i*)ptr_dst, y1);


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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst+32), y1);

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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst+64), y1);
    }
}

void __stdcall afs_blend_avx2(void *_dst, void *_src1, void *_src2, void *_src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src2 = (PIXEL_YC *)_src2;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        afs_blend_avx2_base<false>(dst, src1, src2, src3, sip, mask, 16);
        dst += dw; src1 += dw; src2 += dw; src3 += dw; sip += dw; w -= dw;
    }
    afs_blend_avx2_base<true>(dst, src1, src2, src3, sip, mask, w & (~0x0f));
    if (w & 0x0f) {
        dst += w-16; src1 += w-16; src2 += w-16; src3 += w-16; sip += w-16;
        afs_blend_avx2_base<false>(dst, src1, src2, src3, sip, mask, 16);
    }
    _mm256_zeroupper();
}

template <bool aligned_store>
void __forceinline __stdcall afs_mie_spot_avx2_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w) {
    BYTE *ptr_dst  = (BYTE *)dst;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src2 = (BYTE *)src2;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_src4 = (BYTE *)src4;
    BYTE *ptr_src_spot = (BYTE *)src_spot;
    __m256i y0, y1, y2, y3, y4, y5;
    const __m256i yPwRoundFix1 = _mm256_load_si256((__m256i*)pw_round_fix1);
    const __m256i yPwRoundFix2 = _mm256_load_si256((__m256i*)pw_round_fix2);

    for (int iw = w - 16; iw >= 0; ptr_src1 += 96, ptr_src2 += 96, ptr_src3 += 96, ptr_src4 += 96, ptr_src_spot += 96, ptr_dst += 96, iw -= 16) {
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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst +  0), y0);
        _mm256_stream_switch_si256((__m256i*)(ptr_dst + 32), y1);
        _mm256_stream_switch_si256((__m256i*)(ptr_dst + 64), y2);
    }
}

void __stdcall afs_mie_spot_avx2(void *_dst, void *_src1, void *_src2, void *_src3, void *_src4, void *_src_spot,int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src2 = (PIXEL_YC *)_src2;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    PIXEL_YC *src4 = (PIXEL_YC *)_src4;
    PIXEL_YC *src_spot = (PIXEL_YC *)_src_spot;
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        afs_mie_spot_avx2_base<false>(dst, src1, src2, src3, src4, src_spot, 16);
        dst += dw; src1 += dw; src2 += dw; src3 += dw; src4 += dw; src_spot += dw; w -= dw;
    }
    afs_mie_spot_avx2_base<true>(dst, src1, src2, src3, src4, src_spot, w & (~0x0f));
    if (w & 0x0f) {
        dst += w-16; src1 += w-16; src2 += w-16; src3 += w-16; src4 += w-16; src_spot += w-16;
        afs_mie_spot_avx2_base<false>(dst, src1, src2, src3, src4, src_spot, 16);
    }
    _mm256_zeroupper();
}

template <bool aligned_store>
void __forceinline __stdcall afs_mie_inter_avx2_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src2 = (BYTE *)src2;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_src4 = (BYTE *)src4;
    __m256i y0, y1, y2, y3, y4, y5;
    const __m256i yPwRoundFix2 = _mm256_load_si256((__m256i*)pw_round_fix2);

    for (int iw = w - 16; iw >= 0; ptr_src1 += 96, ptr_src2 += 96, ptr_src3 += 96, ptr_src4 += 96, ptr_dst += 96, iw -= 16) {
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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst +  0), y0);
        _mm256_stream_switch_si256((__m256i*)(ptr_dst + 32), y1);
        _mm256_stream_switch_si256((__m256i*)(ptr_dst + 64), y2);
    }
    _mm256_zeroupper();
}

void __stdcall afs_mie_inter_avx2(void *_dst, void *_src1, void *_src2, void *_src3, void *_src4, int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src2 = (PIXEL_YC *)_src2;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    PIXEL_YC *src4 = (PIXEL_YC *)_src4;
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        afs_mie_inter_avx2_base<false>(dst, src1, src2, src3, src4, 16);
        dst += dw; src1 += dw; src2 += dw; src3 += dw; src4 += dw; w -= dw;
    }
    afs_mie_inter_avx2_base<true>(dst, src1, src2, src3, src4, w & (~0x0f));
    if (w & 0x0f) {
        dst += w-16; src1 += w-16; src2 += w-16; src3 += w-16; src4 += w-16;
        afs_mie_inter_avx2_base<false>(dst, src1, src2, src3, src4, 16);
    }
    _mm256_zeroupper();
}

template <bool aligned_store>
void __forceinline __stdcall afs_deint4_avx2_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
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

    for (int iw = w - 16; iw >= 0; ptr_src4 += 96, ptr_dst += 96, ptr_src3 += 96, ptr_src5 += 96, ptr_src1 += 96, ptr_src7 += 96, ptr_sip += 16, iw -= 16) {
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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst), y1);

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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst+32), y1);

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
        _mm256_stream_switch_si256((__m256i*)(ptr_dst+64), y1);
    }
}

void __stdcall afs_deint4_avx2(void *_dst, void *_src1, void *_src3, void *_src4, void *_src5, void *_src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    PIXEL_YC *src4 = (PIXEL_YC *)_src4;
    PIXEL_YC *src5 = (PIXEL_YC *)_src5;
    PIXEL_YC *src7 = (PIXEL_YC *)_src7;
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        afs_deint4_avx2_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, 16);
        dst += dw; src1 += dw; src3 += dw; src4 += dw; src5 += dw; src7 += dw; sip += dw; w -= dw;
    }
    afs_deint4_avx2_base<true>(dst, src1, src3, src4, src5, src7, sip, mask, w & (~0x0f));
    if (w & 0x0f) {
        dst += w-16; src1 += w-16; src3 += w-16; src4 += w-16; src5 += w-16; src7 += w-16; sip += w-16;
        afs_deint4_avx2_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, 16);
    }
    _mm256_zeroupper();
}

void __stdcall afs_copy_yc48_line_avx2(void *dst, void *src1, int w, int src_frame_pixels) {
    memcpy_avx2<true>(dst, src1, w * sizeof(PIXEL_YC));
}

#include "afs_convert_const.h"

static __forceinline void convert_range_y_yuy2_to_yc48(__m256i& y0, __m256i& y1) {
    y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3, 1, 2, 0));
    y1 = _mm256_unpackhi_epi8(y0, _mm256_setzero_si256());
    y0 = _mm256_unpacklo_epi8(y0, _mm256_setzero_si256());
    y0 = _mm256_slli_epi16(y0, 6);
    y1 = _mm256_slli_epi16(y1, 6);
    y0 = _mm256_mulhi_epi16(y0, _mm256_set1_epi16(19152));
    y1 = _mm256_mulhi_epi16(y1, _mm256_set1_epi16(19152));
    y0 = _mm256_sub_epi16(y0, _mm256_set1_epi16(299));
    y1 = _mm256_sub_epi16(y1, _mm256_set1_epi16(299));
}

static __forceinline void convert_range_c_yuy2_to_yc48(__m256i& y0, __m256i& y1) {
    y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3, 1, 2, 0));
    y1 = _mm256_unpackhi_epi8(y0, _mm256_setzero_si256());
    y0 = _mm256_unpacklo_epi8(y0, _mm256_setzero_si256());
    y0 = _mm256_slli_epi16(y0, 6);
    y1 = _mm256_slli_epi16(y1, 6);
    y0 = _mm256_mulhi_epi16(y0, _mm256_set1_epi16(18752));
    y1 = _mm256_mulhi_epi16(y1, _mm256_set1_epi16(18752));
    y0 = _mm256_sub_epi16(y0, _mm256_set1_epi16(2340));
    y1 = _mm256_sub_epi16(y1, _mm256_set1_epi16(2340));
}

static __forceinline __m256i afs_blend(const __m256i& msrc1, const __m256i& msrc2, const __m256i& msrc3, const __m256i& mmask) {
    __m256i y3, y4;
    y3 = _mm256_add_epi16(msrc1, msrc3);
    y4 = _mm256_add_epi16(msrc2, msrc2);
    y3 = _mm256_add_epi16(y3, y4);
    y3 = _mm256_add_epi16(y3, _mm256_set1_epi16(2));
    y3 = _mm256_srai_epi16(y3, 2);
    return _mm256_blendv_epi8(msrc2, y3, mmask); //x1 = sip ? x3 : x2;
}

static __forceinline __m256i afs_mask_for_uv_16(const __m256i& msip) {
    return _mm256_blend_epi16(msip, _mm256_slli_epi32(msip, 16), 0x80+0x20+0x08+0x02);
}

static __forceinline void afs_mask_extend16(__m256i& msipa, __m256i& msipb, const __m256i& mmask8, uint8_t *sip) {
    msipa = _mm256_loadu_si256((const __m256i *)sip);
    msipa = _mm256_permute4x64_epi64(msipa, _MM_SHUFFLE(3, 1, 2, 0));
    msipa = _mm256_and_si256(msipa, mmask8);
    msipa = _mm256_cmpeq_epi8(msipa, mmask8);
    msipb = _mm256_unpackhi_epi8(msipa, msipa);
    msipa = _mm256_unpacklo_epi8(msipa, msipa);
}

static __forceinline __m256i afs_uv_interp_lanczos(const __m256i& y0, const __m256i& y1, const __m256i& y2) {
    __m256i y3, y6, y7;
    y7 = y1;
    y6 = _mm256_alignr256_epi8(y2, y1, 4);
    y7 = _mm256_add_epi16(y7, y6);

    y6 = _mm256_add_epi16(_mm256_alignr256_epi8(y2, y1, 8), _mm256_alignr256_epi8(y1, y0, 12));
    y6 = _mm256_sub_epi16(y6, y7);
    y6 = _mm256_srai_epi16(y6, 3);

    y3 = _mm256_cmpeq_epi16(y1, y1);
    y3 = _mm256_srli_epi16(y3, 15);
    y7 = _mm256_add_epi16(y7, y3);
    y7 = _mm256_sub_epi16(y7, y6);
    y7 = _mm256_srai_epi16(y7, 1);
    return y7;
}

static __forceinline __m256i afs_uv_interp_linear(const __m256i& y1, const __m256i& y2) {
    //y1 ... | 1 | 0 |
    //y2 ... | 3 | 2 |
    __m256i y3, y4;
    y3 = _mm256_alignr256_epi8(y2, y1, 4); // | 2 | 1 |
    y4 = _mm256_add_epi16(y1, _mm256_srli_epi16(_mm256_cmpeq_epi16(y1, y1), 15));
    y3 = _mm256_add_epi16(y3, y4);
    return _mm256_srai_epi16(y3, 1);
}

static __forceinline void afs_pack_yc48(__m256i& y0, __m256i& y1, __m256i& y2, const __m256i& yY, const __m256i& yC0, const __m256i& yC1) {
    __m256i yYtemp, yCtemp0, yCtemp1;

    yCtemp0 = yC0; // _mm256_permute2x128_si256(yC0, yC1, (2<<4) | 0);
    yCtemp1 = yC1; // _mm256_permute2x128_si256(yC0, yC1, (3<<4) | 1);

    yYtemp = _mm256_shuffle_epi8(yY, yC_SUFFLE_YCP_Y); //52741630

    yCtemp1 = _mm256_shuffle_epi32(yCtemp1, _MM_SHUFFLE(3, 0, 1, 2));
    yCtemp0 = _mm256_shuffle_epi32(yCtemp0, _MM_SHUFFLE(1, 2, 3, 0));
    yCtemp0 = _mm256_alignr_epi8(yCtemp0, yCtemp0, 14);

    __m256i yA0, yA1, yA2;
    yA0 = _mm256_blend_epi16(yYtemp, yCtemp0, 0x80+0x04+0x02);
    yA1 = _mm256_blend_epi16(yYtemp, yCtemp1, 0x08+0x04);
    yA2 = _mm256_blend_epi16(yYtemp, yCtemp0, 0x10+0x08);
    yA2 = _mm256_blend_epi16(yA2, yCtemp1, 0x80+0x40+0x02+0x01);
    yA1 = _mm256_blend_epi16(yA1, yCtemp0, 0x40+0x20+0x01);
    yA0 = _mm256_blend_epi16(yA0, yCtemp1, 0x20+0x10);

    y0 = _mm256_permute2x128_si256(yA0, yA1, (2<<4)|0);
    y1 = _mm256_blend_epi32(yA0, yA2, 0x0f);
    y2 = _mm256_permute2x128_si256(yA1, yA2, (3<<4)|1);
}

template<bool aligned_store>
static __forceinline void afs_pack_store_yc48(uint8_t *dst, const __m256i& my0a, const __m256i& my0b, const __m256i& mc0a, const __m256i& mc0b, const __m256i& mc1a, const __m256i& mc1b) {
    __m256i y0, y1, y2;
    afs_pack_yc48(y0, y1, y2, my0a, mc0a, mc1a);

    _mm256_stream_switch_si256((__m256i*)((uint8_t *)dst +  0), y0);
    _mm256_stream_switch_si256((__m256i*)((uint8_t *)dst + 32), y1);
    _mm256_stream_switch_si256((__m256i*)((uint8_t *)dst + 64), y2);

    afs_pack_yc48(y0, y1, y2, my0b, mc0b, mc1b);

    _mm256_stream_switch_si256((__m256i*)((uint8_t *)dst +  96), y0);
    _mm256_stream_switch_si256((__m256i*)((uint8_t *)dst + 128), y1);
    _mm256_stream_switch_si256((__m256i*)((uint8_t *)dst + 160), y2);
}

template<bool uv_upsample>
static __forceinline void afs_blend_nv16(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *sip, unsigned int mask, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 32;
    const __m256i mmask = _mm256_set1_epi8(mask);
    //コンパイラ頑張れ
    __m256i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b;
    __m256i my0a, my0b, my1a, my1b, my2a, my2b, my3a, my3b;
    __m256i mc4a, mc4b, mc5a, mc5b, mc6a, mc6b, mc7a, mc7b;
    mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
    mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
    mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));

    __m256i msipa, msipb;
    afs_mask_extend16(msipa, msipb, mmask, sip);

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc2a, mc2b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);

    //色差をブレンド
    mc0a = afs_blend(mc1a, mc2a, mc3a, afs_mask_for_uv_16(msipa));
    mc0b = afs_blend(mc1b, mc2b, mc3b, afs_mask_for_uv_16(msipb));
    mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        my2a = _mm256_loadu_si256((const __m256i *)src2);
        my3a = _mm256_loadu_si256((const __m256i *)src3);
        mc5a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));
        mc6a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels + 32));
        mc7a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels + 32));

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
        afs_mask_extend16(msipa, msipb, mmask, sip + 32);

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
        afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc4a;
        mc0b = mc4b;

        //ずれ修正
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        dst += dw;
        src1 += dw;
        src2 += dw;
        src3 += dw;
        sip += dw;

        __m256i msipa, msipb;
        afs_mask_extend16(msipa, msipb, mmask, sip);

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
        mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
        mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);

        mc0a = afs_blend(mc1a, mc2a, mc3a, afs_mask_for_uv_16(msipa));
        mc0b = afs_blend(mc1b, mc2b, mc3b, afs_mask_for_uv_16(msipb));
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }

    for (; src1 < src1_fin; src1 += 32, src2 += 32, src3 += 32, sip += 32, dst += 32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        my2a = _mm256_loadu_si256((const __m256i *)src2);
        my3a = _mm256_loadu_si256((const __m256i *)src3);
        mc5a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));
        mc6a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels + 32));
        mc7a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels + 32));

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
        afs_mask_extend16(msipa, msipb, mmask, sip + 32);

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
        afs_pack_store_yc48<true>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc4a;
        mc0b = mc4b;
    }
    //終端処理
    if (src1_fin < src1) {
        int offset = src1 - src1_fin;
        src1 -= offset;
        src2 -= offset;
        src3 -= offset;
        dst  -= offset;
        sip  -= offset;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
        mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
        mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);

        __m256i msipa, msipb;
        afs_mask_extend16(msipa, msipb, mmask, sip);

        mc0a = afs_blend(mc1a, mc2a, mc3a, afs_mask_for_uv_16(msipa));
        mc0b = afs_blend(mc1b, mc2b, mc3b, afs_mask_for_uv_16(msipb));
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }
    my1a = _mm256_loadu_si256((const __m256i *)src1);
    my2a = _mm256_loadu_si256((const __m256i *)src2);
    my3a = _mm256_loadu_si256((const __m256i *)src3);

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
    //mc4b = afs_blend(mc5b, mc6b, mc7b, afs_mask_for_uv_16(msipa));
    mc4a = _mm256_permute4x64_epi64(_mm256_shuffle_epi32(mc0b, _MM_SHUFFLE(3, 3, 3, 3)), _MM_SHUFFLE(3, 3, 3, 3));

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
    afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);
}

void __stdcall afs_blend_nv16up_avx2(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16<true>((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, sip, mask, w, src_frame_pixels);
}

void __stdcall afs_blend_nv16_avx2(void *dst, void *src1, void *src2, void *src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    afs_blend_nv16<false>((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, sip, mask, w, src_frame_pixels);
}

static __forceinline __m256i afs_mie_spot(const __m256i& msrc1, const __m256i& msrc2, const __m256i& msrc3, const __m256i& msrc4, const __m256i& msrc_spot) {
    __m256i y0, y1, y2;
    y0 = _mm256_add_epi16(msrc1, msrc2);
    y1 = _mm256_add_epi16(msrc3, msrc4);
    y0 = _mm256_add_epi16(y0, y1);
    y0 = _mm256_add_epi16(y0, _mm256_set1_epi16(2));
    y0 = _mm256_srai_epi16(y0, 2);
    y2 = _mm256_add_epi16(msrc_spot, _mm256_set1_epi16(1));
    y0 = _mm256_add_epi16(y0, y2);
    y0 = _mm256_srai_epi16(y0, 1);
    return y0;
}

template<bool uv_upsample>
static __forceinline void afs_mie_spot_nv16(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, uint8_t *src_spot, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 32;
    //コンパイラ頑張れ
    __m256i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b, mc4a, mc4b, mcsa, mcsb;
    __m256i my0a, my0b, my1a, my1b, my2a, my2b, my3a, my3b, my4a, my4b, mysa, mysb;
    __m256i mc5a, mc5b, mc6a, mc6b, mc7a, mc7b, mc8a, mc8b, mc9a, mc9b;
    mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
    mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
    mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));
    mc4a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels));
    mcsa = _mm256_loadu_si256((const __m256i *)(src_spot + src_frame_pixels));

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc2a, mc2b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);
    convert_range_c_yuy2_to_yc48(mc4a, mc4b);
    convert_range_c_yuy2_to_yc48(mcsa, mcsb);

    //色差をブレンド
    mc0a = afs_mie_spot(mc1a, mc2a, mc3a, mc4a, mcsa);
    mc0b = afs_mie_spot(mc1b, mc2b, mc3b, mc4b, mcsb);
    mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        my2a = _mm256_loadu_si256((const __m256i *)src2);
        my3a = _mm256_loadu_si256((const __m256i *)src3);
        my4a = _mm256_loadu_si256((const __m256i *)src4);
        mysa = _mm256_loadu_si256((const __m256i *)src_spot);
        mc6a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));
        mc7a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels + 32));
        mc8a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels + 32));
        mc9a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels + 32));
        mcsa = _mm256_loadu_si256((const __m256i *)(src_spot + src_frame_pixels + 32));

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
        afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc5a;
        mc0b = mc5b;

        //ずれ修正
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        dst += dw;
        src1 += dw;
        src2 += dw;
        src3 += dw;
        src4 += dw;
        src_spot += dw;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
        mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
        mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));
        mc4a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels));
        mcsa = _mm256_loadu_si256((const __m256i *)(src_spot + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);
        convert_range_c_yuy2_to_yc48(mcsa, mcsb);

        mc0a = afs_mie_spot(mc1a, mc2a, mc3a, mc4a, mcsa);
        mc0b = afs_mie_spot(mc1b, mc2b, mc3b, mc4b, mcsb);
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }

    for (; src1 < src1_fin; src1 += 32, src2 += 32, src3 += 32, src4 += 32, src_spot += 32, dst += 32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        my2a = _mm256_loadu_si256((const __m256i *)src2);
        my3a = _mm256_loadu_si256((const __m256i *)src3);
        my4a = _mm256_loadu_si256((const __m256i *)src4);
        mysa = _mm256_loadu_si256((const __m256i *)src_spot);
        mc6a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));
        mc7a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels + 32));
        mc8a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels + 32));
        mc9a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels + 32));
        mcsa = _mm256_loadu_si256((const __m256i *)(src_spot + src_frame_pixels + 32));

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
        afs_pack_store_yc48<true>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc5a;
        mc0b = mc5b;
    }
    //終端処理
    if (src1_fin < src1) {
        int offset = src1 - src1_fin;
        src1 -= offset;
        src2 -= offset;
        src3 -= offset;
        src4 -= offset;
        dst  -= offset;
        src_spot -= offset;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
        mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
        mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));
        mc4a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels));
        mcsa = _mm256_loadu_si256((const __m256i *)(src_spot + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);
        convert_range_c_yuy2_to_yc48(mcsa, mcsb);

        mc0a = afs_mie_spot(mc1a, mc2a, mc3a, mc4a, mcsa);
        mc0b = afs_mie_spot(mc1b, mc2b, mc3b, mc4b, mcsb);
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }
    my1a = _mm256_loadu_si256((const __m256i *)src1);
    my2a = _mm256_loadu_si256((const __m256i *)src2);
    my3a = _mm256_loadu_si256((const __m256i *)src3);
    my4a = _mm256_loadu_si256((const __m256i *)src4);
    mysa = _mm256_loadu_si256((const __m256i *)src_spot);

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
    mc5a = _mm256_permute4x64_epi64(_mm256_shuffle_epi32(mc0b, _MM_SHUFFLE(3, 3, 3, 3)), _MM_SHUFFLE(3, 3, 3, 3));

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
    afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);
}

void __stdcall afs_mie_spot_nv16up_avx2(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16<true>((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src_spot, w, src_frame_pixels);
}

void __stdcall afs_mie_spot_nv16_avx2(void *dst, void *src1, void *src2, void *src3, void *src4, void *src_spot, int w, int src_frame_pixels) {
    afs_mie_spot_nv16<false>((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, (uint8_t *)src_spot, w, src_frame_pixels);
}

static __forceinline __m256i afs_mie_inter(const __m256i& msrc1, const __m256i& msrc2, const __m256i& msrc3, const __m256i& msrc4) {
    __m256i y0, y1;
    y0 = _mm256_add_epi16(msrc1, msrc2);
    y1 = _mm256_add_epi16(msrc3, msrc4);
    y0 = _mm256_add_epi16(y0, y1);
    y0 = _mm256_add_epi16(y0, _mm256_set1_epi16(2));
    y0 = _mm256_srai_epi16(y0, 2);
    return y0;
}

template<bool uv_upsample>
static __forceinline void afs_mie_inter_nv16(PIXEL_YC *dst, uint8_t *src1, uint8_t *src2, uint8_t *src3, uint8_t *src4, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 32;
    //コンパイラ頑張れ
    __m256i mc00, mc0a, mc0b, mc1a, mc1b, mc2a, mc2b, mc3a, mc3b, mc4a, mc4b;
    __m256i my0a, my0b, my1a, my1b, my2a, my2b, my3a, my3b, my4a, my4b;
    __m256i mc5a, mc5b, mc6a, mc6b, mc7a, mc7b, mc8a, mc8b, mc9a, mc9b;
    mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
    mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
    mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));
    mc4a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels));

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);
    convert_range_c_yuy2_to_yc48(mc2a, mc2b);
    convert_range_c_yuy2_to_yc48(mc3a, mc3b);
    convert_range_c_yuy2_to_yc48(mc4a, mc4b);

    //色差をブレンド
    mc0a = afs_mie_inter(mc1a, mc2a, mc3a, mc4a);
    mc0b = afs_mie_inter(mc1b, mc2b, mc3b, mc4b);
    mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        my2a = _mm256_loadu_si256((const __m256i *)src2);
        my3a = _mm256_loadu_si256((const __m256i *)src3);
        my4a = _mm256_loadu_si256((const __m256i *)src4);
        mc6a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));
        mc7a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels + 32));
        mc8a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels + 32));
        mc9a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels + 32));

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
        afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc5a;
        mc0b = mc5b;

        //ずれ修正
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        dst += dw;
        src1 += dw;
        src2 += dw;
        src3 += dw;
        src4 += dw;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
        mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
        mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));
        mc4a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);

        mc0a = afs_mie_inter(mc1a, mc2a, mc3a, mc4a);
        mc0b = afs_mie_inter(mc1b, mc2b, mc3b, mc4b);
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }

    for (; src1 < src1_fin; src1 += 32, src2 += 32, src3 += 32, src4 += 32, dst += 32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        my2a = _mm256_loadu_si256((const __m256i *)src2);
        my3a = _mm256_loadu_si256((const __m256i *)src3);
        my4a = _mm256_loadu_si256((const __m256i *)src4);
        mc6a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));
        mc7a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels + 32));
        mc8a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels + 32));
        mc9a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels + 32));

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
        afs_pack_store_yc48<true>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc5a;
        mc0b = mc5b;
    }
    //終端処理
    if (src1_fin < src1) {
        int offset = src1 - src1_fin;
        src1 -= offset;
        src2 -= offset;
        src3 -= offset;
        src4 -= offset;
        dst  -= offset;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));
        mc2a = _mm256_loadu_si256((const __m256i *)(src2 + src_frame_pixels));
        mc3a = _mm256_loadu_si256((const __m256i *)(src3 + src_frame_pixels));
        mc4a = _mm256_loadu_si256((const __m256i *)(src4 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);
        convert_range_c_yuy2_to_yc48(mc2a, mc2b);
        convert_range_c_yuy2_to_yc48(mc3a, mc3b);
        convert_range_c_yuy2_to_yc48(mc4a, mc4b);

        mc0a = afs_mie_inter(mc1a, mc2a, mc3a, mc4a);
        mc0b = afs_mie_inter(mc1b, mc2b, mc3b, mc4b);
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }
    my1a = _mm256_loadu_si256((const __m256i *)src1);
    my2a = _mm256_loadu_si256((const __m256i *)src2);
    my3a = _mm256_loadu_si256((const __m256i *)src3);
    my4a = _mm256_loadu_si256((const __m256i *)src4);

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
    mc5a = _mm256_permute4x64_epi64(_mm256_shuffle_epi32(mc0b, _MM_SHUFFLE(3, 3, 3, 3)), _MM_SHUFFLE(3, 3, 3, 3));

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
    afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);
}

void __stdcall afs_mie_inter_nv16up_avx2(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16<true>((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, w, src_frame_pixels);
}

void __stdcall afs_mie_inter_nv16_avx2(void *dst, void *src1, void *src2, void *src3, void *src4, int w, int src_frame_pixels) {
    afs_mie_inter_nv16<false>((PIXEL_YC *)dst, (uint8_t *)src1, (uint8_t *)src2, (uint8_t *)src3, (uint8_t *)src4, w, src_frame_pixels);
}

template<bool uv_upsample>
static __forceinline void afs_convert_nv16_yc48(PIXEL_YC *dst, uint8_t *src1, int w, int src_frame_pixels) {
    const uint8_t *src1_fin = src1 + w - 32;
    //コンパイラ頑張れ
    __m256i mc00, mc0a, mc0b, mc1a, mc1b;
    __m256i my0a, my0b, my1a, my1b;
    __m256i mc4a, mc4b, mc5a, mc5b;
    mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));

    convert_range_c_yuy2_to_yc48(mc1a, mc1b);

    //色差をブレンド
    mc0a = mc1a;
    mc0b = mc1b;
    mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    const int dst_mod32 = (size_t)dst & 0x1f;
    if (dst_mod32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        mc5a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));

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
        afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc4a;
        mc0b = mc4b;

        //ずれ修正
        int mod6 = dst_mod32 % 6;
        int dw = (32 * (((mod6) ? mod6 : 6)>>1)-dst_mod32) / 6;
        dst += dw;
        src1 += dw;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);

        mc0a = mc1a;
        mc0b = mc1b;
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }

    for (; src1 < src1_fin; src1 += 32, dst += 32) {
        my1a = _mm256_loadu_si256((const __m256i *)src1);
        mc5a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels + 32));

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
        afs_pack_store_yc48<true>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);

        mc0a = mc4a;
        mc0b = mc4b;
    }
    //終端処理
    if (src1_fin < src1) {
        int offset = src1 - src1_fin;
        src1 -= offset;
        dst  -= offset;

        mc1a = _mm256_loadu_si256((const __m256i *)(src1 + src_frame_pixels));

        convert_range_c_yuy2_to_yc48(mc1a, mc1b);

        mc0a = mc1a;
        mc0b = mc1b;
        mc00 = _mm256_broadcastd_epi32(_mm256_castsi256_si128(mc0a));
    }
    my1a = _mm256_loadu_si256((const __m256i *)src1);

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
    mc4a = _mm256_permute4x64_epi64(_mm256_shuffle_epi32(mc0b, _MM_SHUFFLE(3, 3, 3, 3)), _MM_SHUFFLE(3,3,3,3));

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
    afs_pack_store_yc48<false>((uint8_t *)dst, my0a, my0b, mc0a, mc0b, mc1a, mc1b);
}

void __stdcall afs_convert_nv16_yc48up_avx2(void *dst, void *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48<true>((PIXEL_YC *)dst, (uint8_t *)src1, w, src_frame_pixels);
}

void __stdcall afs_convert_nv16_yc48_avx2(void *dst, void *src1, int w, int src_frame_pixels) {
    afs_convert_nv16_yc48<false>((PIXEL_YC *)dst, (uint8_t *)src1, w, src_frame_pixels);
}
