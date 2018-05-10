#include <immintrin.h> //AVX, AVX2, AVX512
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "filter.h"
#include "afs.h"
#include "simd_util.h"

#if _MSC_VER >= 1800 && !defined(__AVX__) && !defined(_DEBUG)
static_assert(false, "do not forget to set /arch:AVX or /arch:AVX2 for this file.");
#endif

#define _mm512_stream_switch_si512(x, zmm) ((aligned_store) ? _mm512_stream_si512((x), (zmm)) : _mm512_store_si512((x), (zmm)))

template<bool aligned_store>
static void __forceinline memcpy_avx512(void *_dst, void *_src, int size) {
    uint8_t *dst = (uint8_t *)_dst;
    uint8_t *src = (uint8_t *)_src;
    if (size < 256) {
        memcpy(dst, src, size);
        return;
    }
    uint8_t *dst_fin = dst + size;
    uint8_t *dst_aligned_fin = (uint8_t *)(((size_t)(dst_fin + 63) & ~63) - 256);
    __m512i z0, z1, z2, z3;
    const int start_align_diff = (int)((size_t)dst & 63);
    if (start_align_diff) {
        z0 = _mm512_loadu_si512((__m512i*)src);
        _mm512_storeu_si512((__m512i*)dst, z0);
        dst += 64 - start_align_diff;
        src += 64 - start_align_diff;
    }
    for (; dst < dst_aligned_fin; dst += 256, src += 256) {
        z0 = _mm512_loadu_si512((__m512i*)(src +   0));
        z1 = _mm512_loadu_si512((__m512i*)(src +  64));
        z2 = _mm512_loadu_si512((__m512i*)(src + 128));
        z3 = _mm512_loadu_si512((__m512i*)(src + 192));
        _mm512_stream_switch_si512((__m512i*)(dst +   0), z0);
        _mm512_stream_switch_si512((__m512i*)(dst +  64), z1);
        _mm512_stream_switch_si512((__m512i*)(dst + 128), z2);
        _mm512_stream_switch_si512((__m512i*)(dst + 192), z3);
    }
    uint8_t *dst_tmp = dst_fin - 256;
    src -= (dst - dst_tmp);
    z0 = _mm512_loadu_si512((__m512i*)(src +   0));
    z1 = _mm512_loadu_si512((__m512i*)(src +  64));
    z2 = _mm512_loadu_si512((__m512i*)(src + 128));
    z3 = _mm512_loadu_si512((__m512i*)(src + 192));
    _mm512_storeu_si512((__m512i*)(dst_tmp +   0), z0);
    _mm512_storeu_si512((__m512i*)(dst_tmp +  64), z1);
    _mm512_storeu_si512((__m512i*)(dst_tmp + 128), z2);
    _mm512_storeu_si512((__m512i*)(dst_tmp + 192), z3);
}

static const _declspec(align(64)) USHORT SIP_BLEND_MASK[] = {
	0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x0a, 0x0a,
	0x0a, 0x0b, 0x0b, 0x0b, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x15,
	0x15, 0x15, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x1a, 0x1a, 0x1a, 0x1b, 0x1b, 0x1b, 0x1c, 0x1c, 0x1c, 0x1d, 0x1d, 0x1d, 0x1e, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f
};
#define zSIPMASK(x)  (_mm512_load_si512((__m512i*)SIP_BLEND_MASK[x]))

template <bool aligned_store>
void __forceinline __stdcall afs_blend_avx512_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, BYTE *sip, unsigned int mask, int w) {
    BYTE *ptr_dst  = (BYTE *)dst;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src2 = (BYTE *)src2;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_esi  = (BYTE *)sip;
    __m512i z0, z1, z2, z3, z4;
    __mmask32 k1;
    const __m512i zPwRoundFix2 = _mm512_set1_epi16(2);

    for (int iw = w - 32; iw >= 0; ptr_dst += 192, ptr_src1 += 192, ptr_src2 += 192, ptr_src3 += 192, ptr_esi += 32, iw -= 32) {
        z0 = _mm512_cvtepi8_epi16(_mm256_loadu_si256((__m256i*)(ptr_esi)));

        _mm_prefetch((char *)ptr_src1 + 576, _MM_HINT_NTA);
        _mm_prefetch((char *)ptr_src2 + 576, _MM_HINT_NTA);
        _mm_prefetch((char *)ptr_src3 + 576, _MM_HINT_NTA);
        _mm_prefetch((char *)ptr_esi  +  96, _MM_HINT_NTA);

        k1 = _mm512_movepi16_mask(_mm512_permutex2var_epi16(z0, z0, zSIPMASK(0)));
        z4 = _mm512_loadu_si512((__m512i*)(ptr_src2));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src1));
        z2 = z4;
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src3)));
        z4 = _mm512_slli_epi16(z4, 1);
        z3 = _mm512_adds_epi16(z3, z4);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix2);
        z1 = _mm512_mask_srai_epi16(z2, k1, z3, 2); //z1 = sip ? z3 >> 2 : z2;
        _mm512_stream_switch_si512((__m512i*)ptr_dst, z1);

        k1 = _mm512_movepi16_mask(_mm512_permutex2var_epi16(z0, z0, zSIPMASK(1)));
        z4 = _mm512_loadu_si512((__m512i*)(ptr_src2+64));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src1+64));
        z2 = z4;
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src3+64)));
        z4 = _mm512_slli_epi16(z4, 1);
        z3 = _mm512_adds_epi16(z3, z4);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix2);
        z3 = _mm512_srai_epi16(z3, 2);
        z1 = _mm512_mask_srai_epi16(z2, k1, z3, 2); //z1 = sip ? z3 >> 2 : z2;
        _mm512_stream_switch_si512((__m512i*)(ptr_dst+64), z1);

        k1 = _mm512_movepi16_mask(_mm512_permutex2var_epi16(z0, z0, zSIPMASK(2)));
        z4 = _mm512_loadu_si512((__m512i*)(ptr_src2+128));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src1+128));
        z2 = z4;
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src3+128)));
        z4 = _mm512_slli_epi16(z4, 1);
        z3 = _mm512_adds_epi16(z3, z4);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix2);
        z1 = _mm512_mask_srai_epi16(z2, k1, z3, 2); //z1 = sip ? z3 >> 2 : z2;
        _mm512_stream_switch_si512((__m512i*)(ptr_dst+128), z1);
    }
}

void __stdcall afs_blend_avx512(void *_dst, void *_src1, void *_src2, void *_src3, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src2 = (PIXEL_YC *)_src2;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    const int dst_mod64 = (size_t)dst & 0x3f;
    if (dst_mod64) {
        int mod6 = dst_mod64 % 6;
        int dw = (64 * (((mod6) ? mod6 : 6)>>1)-dst_mod64) / 6;
        afs_blend_avx512_base<false>(dst, src1, src2, src3, sip, mask, 32);
        dst += dw; src1 += dw; src2 += dw; src3 += dw; sip += dw; w -= dw;
    }
    afs_blend_avx512_base<false>(dst, src1, src2, src3, sip, mask, w & (~0x1f));
    if (w & 0x1f) {
        dst += w-32; src1 += w-32; src2 += w-32; src3 += w-32; sip += w-32;
        afs_blend_avx512_base<false>(dst, src1, src2, src3, sip, mask, 32);
    }
}

template <bool aligned_store>
void __forceinline __stdcall afs_mie_spot_avx512_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w) {
    BYTE *ptr_dst  = (BYTE *)dst;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src2 = (BYTE *)src2;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_src4 = (BYTE *)src4;
    BYTE *ptr_src_spot = (BYTE *)src_spot;
    __m512i z0, z1, z2, z3, z4, z5;
    const __m512i zPwRoundFix1 = _mm512_set1_epi16(1);
    const __m512i zPwRoundFix2 = _mm512_set1_epi16(2);

    for (int iw = w - 32; iw >= 0; ptr_src1 += 192, ptr_src2 += 192, ptr_src3 += 192, ptr_src4 += 192, ptr_src_spot += 192, ptr_dst += 192, iw -= 32) {
        z0 = _mm512_loadu_si512((__m512i*)(ptr_src1 +   0));
        z1 = _mm512_loadu_si512((__m512i*)(ptr_src1 +  64));
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src1 + 128));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src3 +   0));
        z4 = _mm512_loadu_si512((__m512i*)(ptr_src3 +  64));
        z5 = _mm512_loadu_si512((__m512i*)(ptr_src3 + 128));
        z0 = _mm512_adds_epi16(z0, _mm512_loadu_si512((__m512i*)(ptr_src2 +   0)));
        z1 = _mm512_adds_epi16(z1, _mm512_loadu_si512((__m512i*)(ptr_src2 +  64)));
        z2 = _mm512_adds_epi16(z2, _mm512_loadu_si512((__m512i*)(ptr_src2 + 128)));
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src4 +   0)));
        z4 = _mm512_adds_epi16(z4, _mm512_loadu_si512((__m512i*)(ptr_src4 +  64)));
        z5 = _mm512_adds_epi16(z5, _mm512_loadu_si512((__m512i*)(ptr_src4 + 128)));
        z0 = _mm512_adds_epi16(z0, z3);
        z1 = _mm512_adds_epi16(z1, z4);
        z2 = _mm512_adds_epi16(z2, z5);
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src_spot +   0));
        z4 = _mm512_loadu_si512((__m512i*)(ptr_src_spot +  64));
        z5 = _mm512_loadu_si512((__m512i*)(ptr_src_spot + 128));
        z0 = _mm512_adds_epi16(z0, zPwRoundFix2);
        z1 = _mm512_adds_epi16(z1, zPwRoundFix2);
        z2 = _mm512_adds_epi16(z2, zPwRoundFix2);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix1);
        z4 = _mm512_adds_epi16(z4, zPwRoundFix1);
        z5 = _mm512_adds_epi16(z5, zPwRoundFix1);
        z0 = _mm512_srai_epi16(z0, 2);
        z1 = _mm512_srai_epi16(z1, 2);
        z2 = _mm512_srai_epi16(z2, 2);
        z0 = _mm512_adds_epi16(z0, z3);
        z1 = _mm512_adds_epi16(z1, z4);
        z2 = _mm512_adds_epi16(z2, z5);
        z0 = _mm512_srai_epi16(z0, 1);
        z1 = _mm512_srai_epi16(z1, 1);
        z2 = _mm512_srai_epi16(z2, 1);
        _mm512_stream_switch_si512((__m512i*)(ptr_dst +   0), z0);
        _mm512_stream_switch_si512((__m512i*)(ptr_dst +  64), z1);
        _mm512_stream_switch_si512((__m512i*)(ptr_dst + 128), z2);
    }
}

void __stdcall afs_mie_spot_avx512(void *_dst, void *_src1, void *_src2, void *_src3, void *_src4, void *_src_spot,int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src2 = (PIXEL_YC *)_src2;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    PIXEL_YC *src4 = (PIXEL_YC *)_src4;
    PIXEL_YC *src_spot = (PIXEL_YC *)_src_spot;
    const int dst_mod64 = (size_t)dst & 0x3f;
    if (dst_mod64) {
        int mod6 = dst_mod64 % 6;
        int dw = (64 * (((mod6) ? mod6 : 6)>>1)-dst_mod64) / 6;
        afs_mie_spot_avx512_base<false>(dst, src1, src2, src3, src4, src_spot, 32);
        dst += dw; src1 += dw; src2 += dw; src3 += dw; src4 += dw; src_spot += dw; w -= dw;
    }
    afs_mie_spot_avx512_base<false>(dst, src1, src2, src3, src4, src_spot, w & (~0x1f));
    if (w & 0x1f) {
        dst += w-32; src1 += w-32; src2 += w-32; src3 += w-32; src4 += w-32; src_spot += w-32;
        afs_mie_spot_avx512_base<false>(dst, src1, src2, src3, src4, src_spot, 32);
    }
}

template <bool aligned_store>
void __forceinline __stdcall afs_mie_inter_avx512_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3, PIXEL_YC *src4, int w) {
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src2 = (BYTE *)src2;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_src4 = (BYTE *)src4;
    __m512i z0, z1, z2, z3, z4, z5;
    const __m512i zPwRoundFix2 = _mm512_set1_epi16(2);

    for (int iw = w - 32; iw >= 0; ptr_src1 += 192, ptr_src2 += 192, ptr_src3 += 192, ptr_src4 += 192, ptr_dst += 192, iw -= 32) {
        z0 = _mm512_loadu_si512((__m512i*)(ptr_src1 +   0));
        z1 = _mm512_loadu_si512((__m512i*)(ptr_src1 +  64));
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src1 + 128));

        z3 = _mm512_loadu_si512((__m512i*)(ptr_src3 +   0));
        z4 = _mm512_loadu_si512((__m512i*)(ptr_src3 +  64));
        z5 = _mm512_loadu_si512((__m512i*)(ptr_src3 + 128));
        z0 = _mm512_adds_epi16(z0, _mm512_loadu_si512((__m512i*)(ptr_src2 +   0)));
        z1 = _mm512_adds_epi16(z1, _mm512_loadu_si512((__m512i*)(ptr_src2 +  64)));
        z2 = _mm512_adds_epi16(z2, _mm512_loadu_si512((__m512i*)(ptr_src2 + 128)));
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src4 +   0)));
        z4 = _mm512_adds_epi16(z4, _mm512_loadu_si512((__m512i*)(ptr_src4 +  64)));
        z5 = _mm512_adds_epi16(z5, _mm512_loadu_si512((__m512i*)(ptr_src4 + 128)));
        z0 = _mm512_adds_epi16(z0, z3);
        z1 = _mm512_adds_epi16(z1, z4);
        z2 = _mm512_adds_epi16(z2, z5);

        z0 = _mm512_adds_epi16(z0, zPwRoundFix2);
        z1 = _mm512_adds_epi16(z1, zPwRoundFix2);
        z2 = _mm512_adds_epi16(z2, zPwRoundFix2);
        z0 = _mm512_srai_epi16(z0, 2);
        z1 = _mm512_srai_epi16(z1, 2);
        z2 = _mm512_srai_epi16(z2, 2);
        _mm512_stream_switch_si512((__m512i*)(ptr_dst +   0), z0);
        _mm512_stream_switch_si512((__m512i*)(ptr_dst +  64), z1);
        _mm512_stream_switch_si512((__m512i*)(ptr_dst + 128), z2);
    }
}

void __stdcall afs_mie_inter_avx512(void *_dst, void *_src1, void *_src2, void *_src3, void *_src4, int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src2 = (PIXEL_YC *)_src2;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    PIXEL_YC *src4 = (PIXEL_YC *)_src4;
    const int dst_mod64 = (size_t)dst & 0x3f;
    if (dst_mod64) {
        int mod6 = dst_mod64 % 6;
        int dw = (64 * (((mod6) ? mod6 : 6)>>1)-dst_mod64) / 6;
        afs_mie_inter_avx512_base<false>(dst, src1, src2, src3, src4, 32);
        dst += dw; src1 += dw; src2 += dw; src3 += dw; src4 += dw; w -= dw;
    }
    afs_mie_inter_avx512_base<true>(dst, src1, src2, src3, src4, w & (~0x0f));
    if (w & 0x0f) {
        dst += w-32; src1 += w-32; src2 += w-32; src3 += w-32; src4 += w-32;
        afs_mie_inter_avx512_base<false>(dst, src1, src2, src3, src4, 32);
    }
}

template <bool aligned_store>
void __forceinline __stdcall afs_deint4_avx512_base(PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7, BYTE *sip, unsigned int mask, int w) {
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_sip = (BYTE *)sip;
    BYTE *ptr_src1 = (BYTE *)src1;
    BYTE *ptr_src3 = (BYTE *)src3;
    BYTE *ptr_src4 = (BYTE *)src4;
    BYTE *ptr_src5 = (BYTE *)src5;
    BYTE *ptr_src7 = (BYTE *)src7;
    __m512i z0, z1, z2, z3;
    __mmask32 k1;
    const __m512i zPwRoundFix1 = _mm512_set1_epi16(1);

    for (int iw = w - 32; iw >= 0; ptr_src4 += 192, ptr_dst += 192, ptr_src3 += 192, ptr_src5 += 192, ptr_src1 += 192, ptr_src7 += 192, ptr_sip += 32, iw -= 32) {
        z0 = _mm512_cvtepi8_epi16(_mm256_loadu_si256((__m256i*)(ptr_sip)));

        k1 = _mm512_movepi16_mask(_mm512_permutex2var_epi16(z0, z0, zSIPMASK(0)));
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src1));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src3));
        z2 = _mm512_adds_epi16(z2, _mm512_loadu_si512((__m512i*)(ptr_src7)));
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src5)));
        z2 = _mm512_subs_epi16(z2, z3);
        z2 = _mm512_srai_epi16(z2, 3);
        z3 = _mm512_subs_epi16(z3, z2);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix1);
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src4));
        z1 = _mm512_mask_srai_epi16(z2, k1, z3, 1); //z1 = sip ? z3 >> 1 : z2;
        _mm512_stream_switch_si512((__m512i*)(ptr_dst), z1);

        k1 = _mm512_movepi16_mask(_mm512_permutex2var_epi16(z0, z0, zSIPMASK(1)));
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src1+64));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src3+64));
        z2 = _mm512_adds_epi16(z2, _mm512_loadu_si512((__m512i*)(ptr_src7+64)));
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src5+64)));
        z2 = _mm512_subs_epi16(z2, z3);
        z2 = _mm512_srai_epi16(z2, 3);
        z3 = _mm512_subs_epi16(z3, z2);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix1);
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src4+64));
        z1 = _mm512_mask_srai_epi16(z2, k1, z3, 1); //z1 = sip ? z3 >> 1 : z2;
        _mm512_stream_switch_si512((__m512i*)(ptr_dst+64), z1);

        k1 = _mm512_movepi16_mask(_mm512_permutex2var_epi16(z0, z0, zSIPMASK(2)));
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src1+128));
        z3 = _mm512_loadu_si512((__m512i*)(ptr_src3+128));
        z2 = _mm512_adds_epi16(z2, _mm512_loadu_si512((__m512i*)(ptr_src7+128)));
        z3 = _mm512_adds_epi16(z3, _mm512_loadu_si512((__m512i*)(ptr_src5+128)));
        z2 = _mm512_subs_epi16(z2, z3);
        z2 = _mm512_srai_epi16(z2, 3);
        z3 = _mm512_subs_epi16(z3, z2);
        z3 = _mm512_adds_epi16(z3, zPwRoundFix1);
        z2 = _mm512_loadu_si512((__m512i*)(ptr_src4+128));
        z1 = _mm512_mask_srai_epi16(z2, k1, z3, 1); //z1 = sip ? z3 >> 1 : z2;
        _mm512_stream_switch_si512((__m512i*)(ptr_dst+128), z1);
    }
}

void __stdcall afs_deint4_avx512(void *_dst, void *_src1, void *_src3, void *_src4, void *_src5, void *_src7, BYTE *sip, unsigned int mask, int w, int src_frame_pixels) {
    PIXEL_YC *dst  = (PIXEL_YC *)_dst;
    PIXEL_YC *src1 = (PIXEL_YC *)_src1;
    PIXEL_YC *src3 = (PIXEL_YC *)_src3;
    PIXEL_YC *src4 = (PIXEL_YC *)_src4;
    PIXEL_YC *src5 = (PIXEL_YC *)_src5;
    PIXEL_YC *src7 = (PIXEL_YC *)_src7;
    const int dst_mod64 = (size_t)dst & 0x3f;
    if (dst_mod64) {
        int mod6 = dst_mod64 % 6;
        int dw = (64 * (((mod6) ? mod6 : 6)>>1)-dst_mod64) / 6;
        afs_deint4_avx512_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, 32);
        dst += dw; src1 += dw; src3 += dw; src4 += dw; src5 += dw; src7 += dw; sip += dw; w -= dw;
    }
    afs_deint4_avx512_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, w & (~0x1f));
    if (w & 0x1f) {
        dst += w-32; src1 += w-32; src3 += w-32; src4 += w-32; src5 += w-32; src7 += w-32; sip += w-32;
        afs_deint4_avx512_base<false>(dst, src1, src3, src4, src5, src7, sip, mask, 32);
    }
}

void __stdcall afs_copy_yc48_line_avx512(void *dst, void *src1, int w, int src_frame_pixels) {
    memcpy_avx512<true>(dst, src1, w * sizeof(PIXEL_YC));
}
