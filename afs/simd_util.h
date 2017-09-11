#pragma once
#ifndef _SIMD_UTIL_H_
#define _SIMD_UTIL_H_

#include <stdint.h>
#include <emmintrin.h> //イントリンシック命令 SSE2
#if USE_SSSE3
#include <tmmintrin.h> //イントリンシック命令 SSSE3
#endif
#if USE_SSE41
#include <smmintrin.h> //イントリンシック命令 SSE4.1
#endif
#if USE_POPCNT
#include <nmmintrin.h> //イントリンシック命令 SSE4.2
#endif

#if defined (__cplusplus)
extern "C" {
#endif  /* defined (__cplusplus) */
    void __stosb(unsigned char *, unsigned char, size_t);
    void __stosd(unsigned long *, unsigned long, size_t);
    void __stosw(unsigned short *, unsigned short, size_t);
    void __movsb(unsigned char *, unsigned char const *, size_t);
    void __movsd(unsigned long *, unsigned long const *, size_t);
    void __movsw(unsigned short *, unsigned short const *, size_t);
#if defined (__cplusplus)
}
#endif  /* defined (__cplusplus) */

enum {
    NONE     = 0x0000,
    SSE2     = 0x0001,
    SSE3     = 0x0002,
    SSSE3    = 0x0004,
    SSE41    = 0x0008,
    SSE42    = 0x0010,
    POPCNT   = 0x0020,
    AVX      = 0x0040,
    AVX2     = 0x0080,
    AVX2FAST = 0x0100,
};

#define CLAMP(x, low, high) (((x) > (high))? (high) : ((x) < (low))? (low) : (x))
#define SWAP(type,x,y) { type temp = x; x = y; y = temp; }

static inline int popcnt32_c(DWORD bits) {
    bits = (bits & 0x55555555) + (bits >> 1 & 0x55555555);
    bits = (bits & 0x33333333) + (bits >> 2 & 0x33333333);
    bits = (bits & 0x0f0f0f0f) + (bits >> 4 & 0x0f0f0f0f);
    bits = (bits & 0x00ff00ff) + (bits >> 8 & 0x00ff00ff);
    bits = (bits & 0x0000ffff) + (bits >>16 & 0x0000ffff);
    return bits;
}
#if USE_POPCNT
#define popcnt32(x) _mm_popcnt_u32(x)
#else
#define popcnt32(x) popcnt32_c(x)
#endif

#define _mm_store_switch_si128(ptr, xmm)  ((aligned_store) ? _mm_store_si128(ptr, xmm)  : _mm_storeu_si128(ptr, xmm))
#define _mm_stream_switch_si128(ptr, xmm) ((aligned_store) ? _mm_stream_si128(ptr, xmm) : _mm_storeu_si128(ptr, xmm))

//r0 := (mask0 & 0x80) ? b0 : a0
//SSE4.1の_mm_blendv_epi8(__m128i a, __m128i b, __m128i mask) のSSE2版のようなもの
static inline __m128i select_by_mask(__m128i a, __m128i b, __m128i mask) {
    return _mm_or_si128( _mm_andnot_si128(mask,a), _mm_and_si128(b,mask) );
}
//SSSE3のpalignrもどき
#define palignr_sse2(a,b,i) _mm_or_si128( _mm_slli_si128(a, 16-i), _mm_srli_si128(b, i) )

#if USE_SSSE3
#define _mm_alignr_epi8_simd _mm_alignr_epi8
#else
#define _mm_alignr_epi8_simd palignr_sse2
#endif

static inline __m128i _mm_abs_epi16_simd(__m128i x0) {
#if USE_SSSE3
    x0 = _mm_abs_epi16(x0);
#else
    __m128i x1;
    x1 = _mm_setzero_si128();
    x1 = _mm_cmpgt_epi16(x1, x0);
    x0 = _mm_xor_si128(x0, x1);
    x0 = _mm_subs_epi16(x0, x1);
#endif
    return x0;
}

#if USE_AVX2
#define _mm256_store_switch_si256(ptr, ymm)  ((aligned_store) ? _mm256_store_si256(ptr, ymm)  : _mm256_storeu_si256(ptr, ymm))
#define _mm256_stream_switch_si256(ptr, ymm) ((aligned_store) ? _mm256_stream_si256(ptr, ymm) : _mm256_storeu_si256(ptr, ymm))
//本来の256bit alignr
#define MM_ABS(x) (((x) < 0) ? -(x) : (x))
#define _mm256_alignr256_epi8(a, b, i) ((i<=16) ? _mm256_alignr_epi8(_mm256_permute2x128_si256(a, b, (0x00<<4) + 0x03), b, i) : _mm256_alignr_epi8(a, _mm256_permute2x128_si256(a, b, (0x00<<4) + 0x03), MM_ABS(i-16)))

//_mm256_srli_si256, _mm256_slli_si256は
//単に128bitシフト×2をするだけの命令である
#define _mm256_bsrli_epi128 _mm256_srli_si256
#define _mm256_bslli_epi128 _mm256_slli_si256
//本当の256bitシフト
#define _mm256_srli256_si256(a, i) ((i<=16) ? _mm256_alignr_epi8(_mm256_permute2x128_si256(a, a, (0x08<<4) + 0x03), a, i) : _mm256_bsrli_epi128(_mm256_permute2x128_si256(a, a, (0x08<<4) + 0x03), MM_ABS(i-16)))
#define _mm256_slli256_si256(a, i) ((i<=16) ? _mm256_alignr_epi8(a, _mm256_permute2x128_si256(a, a, (0x00<<4) + 0x08), MM_ABS(16-i)) : _mm256_bslli_epi128(_mm256_permute2x128_si256(a, a, (0x00<<4) + 0x08), MM_ABS(i-16)))

template<bool aligned_store>
static void __forceinline memcpy_avx2(void *_dst, void *_src, int size) {
    uint8_t *dst = (uint8_t *)_dst;
    uint8_t *src = (uint8_t *)_src;
    if (size < 128) {
        for (int i = 0; i < size; i++)
            dst[i] = src[i];
        return;
    }
    uint8_t *dst_fin = dst + size;
    uint8_t *dst_aligned_fin = (uint8_t *)(((size_t)(dst_fin + 31) & ~31) - 128);
    __m256i y0, y1, y2, y3;
    const int start_align_diff = (int)((size_t)dst & 31);
    if (start_align_diff) {
        y0 = _mm256_loadu_si256((__m256i*)src);
        _mm256_storeu_si256((__m256i*)dst, y0);
        dst += 32 - start_align_diff;
        src += 32 - start_align_diff;
    }
    for (; dst < dst_aligned_fin; dst += 128, src += 128) {
        y0 = _mm256_loadu_si256((__m256i*)(src +  0));
        y1 = _mm256_loadu_si256((__m256i*)(src + 32));
        y2 = _mm256_loadu_si256((__m256i*)(src + 64));
        y3 = _mm256_loadu_si256((__m256i*)(src + 96));
        _mm256_stream_switch_si256((__m256i*)(dst +  0), y0);
        _mm256_stream_switch_si256((__m256i*)(dst + 32), y1);
        _mm256_stream_switch_si256((__m256i*)(dst + 64), y2);
        _mm256_stream_switch_si256((__m256i*)(dst + 96), y3);
    }
    uint8_t *dst_tmp = dst_fin - 128;
    src -= (dst - dst_tmp);
    y0 = _mm256_loadu_si256((__m256i*)(src +  0));
    y1 = _mm256_loadu_si256((__m256i*)(src + 32));
    y2 = _mm256_loadu_si256((__m256i*)(src + 64));
    y3 = _mm256_loadu_si256((__m256i*)(src + 96));
    _mm256_storeu_si256((__m256i*)(dst_tmp +  0), y0);
    _mm256_storeu_si256((__m256i*)(dst_tmp + 32), y1);
    _mm256_storeu_si256((__m256i*)(dst_tmp + 64), y2);
    _mm256_storeu_si256((__m256i*)(dst_tmp + 96), y3);
    _mm256_zeroupper();
}


static inline int limit_1_to_16(int value) {
    int cmp_ret = (value>=16);
    return (cmp_ret<<4) + ((value & 0x0f) & (cmp_ret-1)) + (value == 0);
}
static inline int limit_1_to_32(int value) {
    int cmp_ret = (value>=32);
    return (cmp_ret<<5) + ((value & 0x1f) & (cmp_ret-1)) + (value == 0);
}
#else
template<bool use_stream>
static void __forceinline memcpy_sse(void *_dst, void *_src, int size) {
    uint8_t *dst = (uint8_t *)_dst;
    uint8_t *src = (uint8_t *)_src;
    if (size < 64) {
        for (int i = 0; i < size; i++)
            dst[i] = src[i];
        return;
    }
    uint8_t *dst_fin = dst + size;
    uint8_t *dst_aligned_fin = (uint8_t *)(((size_t)(dst_fin + 15) & ~15) - 64);
    __m128 x0, x1, x2, x3;
    const int start_align_diff = (int)((size_t)dst & 15);
    if (start_align_diff) {
        x0 = _mm_loadu_ps((float*)src);
        _mm_storeu_ps((float*)dst, x0);
        dst += 16 - start_align_diff;
        src += 16 - start_align_diff;
    }
#define _mm_stream_switch_ps(x, ymm) ((use_stream) ? _mm_stream_ps((x), (ymm)) : _mm_store_ps((x), (ymm)))
    for (; dst < dst_aligned_fin; dst += 64, src += 64) {
        x0 = _mm_loadu_ps((float*)(src +  0));
        x1 = _mm_loadu_ps((float*)(src + 16));
        x2 = _mm_loadu_ps((float*)(src + 32));
        x3 = _mm_loadu_ps((float*)(src + 48));
        _mm_stream_switch_ps((float*)(dst +  0), x0);
        _mm_stream_switch_ps((float*)(dst + 16), x1);
        _mm_stream_switch_ps((float*)(dst + 32), x2);
        _mm_stream_switch_ps((float*)(dst + 48), x3);
    }
    uint8_t *dst_tmp = dst_fin - 64;
    src -= (dst - dst_tmp);
    x0 = _mm_loadu_ps((float*)(src +  0));
    x1 = _mm_loadu_ps((float*)(src + 16));
    x2 = _mm_loadu_ps((float*)(src + 32));
    x3 = _mm_loadu_ps((float*)(src + 48));
    _mm_storeu_ps((float*)(dst_tmp +  0), x0);
    _mm_storeu_ps((float*)(dst_tmp + 16), x1);
    _mm_storeu_ps((float*)(dst_tmp + 32), x2);
    _mm_storeu_ps((float*)(dst_tmp + 48), x3);
}

static inline int limit_1_to_8(int value) {
    int cmp_ret = (value>=8);
    return (cmp_ret<<3) + ((value & 0x07) & (cmp_ret-1)) + (value == 0);
}
static inline int limit_1_to_16(int value) {
    int cmp_ret = (value>=16);
    return (cmp_ret<<4) + ((value & 0x0f) & (cmp_ret-1)) + (value == 0);
}
#endif //_INCLUDED_IMM

#endif //_SIMD_UTIL_H_
