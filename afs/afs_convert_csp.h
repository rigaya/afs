#pragma once
#include <emmintrin.h> //イントリンシック命令 SSE2
#if USE_SSSE3
#include <tmmintrin.h> //イントリンシック命令 SSSE3
#endif
#if USE_SSE41
#include <smmintrin.h> //イントリンシック命令 SSE4.1
#endif
#include "afs_convert_const.h"

#define _mm_store_switch_si128(ptr, xmm) ((aligned_store) ? _mm_store_si128(ptr, xmm) : _mm_storeu_si128(ptr, xmm))

static __forceinline __m128i _mm_packus_epi32_simd(__m128i a, __m128i b) {
#if USE_SSE41
    return _mm_packus_epi32(a, b);
#else
    static const _declspec(align(64)) uint32_t VAL[2][4] ={
        { 0x00008000, 0x00008000, 0x00008000, 0x00008000 },
        { 0x80008000, 0x80008000, 0x80008000, 0x80008000 }
    };
#define LOAD_32BIT_0x8000 _mm_load_si128((__m128i *)VAL[0])
#define LOAD_16BIT_0x8000 _mm_load_si128((__m128i *)VAL[1])
    a = _mm_sub_epi32(a, LOAD_32BIT_0x8000);
    b = _mm_sub_epi32(b, LOAD_32BIT_0x8000);
    a = _mm_packs_epi32(a, b);
    return _mm_add_epi16(a, LOAD_16BIT_0x8000);
#undef LOAD_32BIT_0x8000
#undef LOAD_16BIT_0x8000
#endif
}

static __forceinline void separate_low_up(__m128i& x0_return_lower, __m128i& x1_return_upper) {
    __m128i x4, x5;
    const __m128i xMaskLowByte = _mm_srli_epi16(_mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128()), 8);
    x4 = _mm_srli_epi16(x0_return_lower, 8);
    x5 = _mm_srli_epi16(x1_return_upper, 8);

    x0_return_lower = _mm_and_si128(x0_return_lower, xMaskLowByte);
    x1_return_upper = _mm_and_si128(x1_return_upper, xMaskLowByte);

    x0_return_lower = _mm_packus_epi16(x0_return_lower, x1_return_upper);
    x1_return_upper = _mm_packus_epi16(x4, x5);
}

static __forceinline void separate_low_up_16bit(__m128i& x0_return_lower, __m128i& x1_return_upper) {
    __m128i x4, x5;
    const __m128i xMaskLowByte = _mm_srli_epi32(_mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128()), 16);
    x4 = _mm_srli_epi32(x0_return_lower, 16);
    x5 = _mm_srli_epi32(x1_return_upper, 16);

    x0_return_lower = _mm_and_si128(x0_return_lower, xMaskLowByte);
    x1_return_upper = _mm_and_si128(x1_return_upper, xMaskLowByte);

    x0_return_lower = _mm_packus_epi32_simd(x0_return_lower, x1_return_upper);
    x1_return_upper = _mm_packus_epi32_simd(x4, x5);
}

static __forceinline void gather_y_uv_from_yc48(__m128i& x0, __m128i& x1, __m128i x2) {
#if USE_SSE41
    __m128i x3;
    const int MASK_INT_Y  = 0x80 + 0x10 + 0x02;
    const int MASK_INT_UV = 0x40 + 0x20 + 0x01;
    x3 = _mm_blend_epi16(x0, x1, MASK_INT_Y);
    x3 = _mm_blend_epi16(x3, x2, MASK_INT_Y>>2);

    x1 = _mm_blend_epi16(x0, x1, MASK_INT_UV);
    x1 = _mm_blend_epi16(x1, x2, MASK_INT_UV>>2);
    x1 = _mm_alignr_epi8_simd(x1, x1, 2);
    x1 = _mm_shuffle_epi32(x1, _MM_SHUFFLE(1, 2, 3, 0));//UV1行目

    x0 = _mm_shuffle_epi8(x3, xC_SUFFLE_YCP_Y);
#else
    __m128i x3;
    x3 = select_by_mask(x0, x1, xC_MASK_YCP2Y(0));
    x3 = select_by_mask(x3, x2, xC_MASK_YCP2Y(1));

    x1 = select_by_mask(x0, x1, xC_MASK_YCP2UV(0));
    x1 = select_by_mask(x1, x2, xC_MASK_YCP2UV(1));
    x1 = _mm_alignr_epi8_simd(x1, x1, 2);
    x1 = _mm_shuffle_epi32(x1, _MM_SHUFFLE(1, 2, 3, 0));
#if USE_SSSE3
    x0 = _mm_shuffle_epi8(x3, xC_SUFFLE_YCP_Y);
#else
    x0 = _mm_shuffle_epi32(x3,   _MM_SHUFFLE(3, 1, 2, 0));
    x0 = _mm_shufflehi_epi16(x0, _MM_SHUFFLE(1, 2, 3, 0));
    x0 = _mm_shuffle_epi32(x0,   _MM_SHUFFLE(1, 2, 3, 0));
    x0 = _mm_shufflelo_epi16(x0, _MM_SHUFFLE(1, 2, 3, 0));
    x0 = _mm_shufflehi_epi16(x0, _MM_SHUFFLE(3, 0, 1, 2));
#endif //USE_SSSE3
#endif //USE_SSE41
}

static __forceinline __m128i convert_y_range_from_yc48(__m128i x0, const __m128i& xC_Y_MA_16, int Y_RSH_16, const __m128i& xC_YCC, const __m128i& xC_pw_one) {
    __m128i x7;
    x7 = _mm_unpackhi_epi16(x0, xC_pw_one);
    x0 = _mm_unpacklo_epi16(x0, xC_pw_one);

    x0 = _mm_madd_epi16(x0, xC_Y_MA_16);
    x7 = _mm_madd_epi16(x7, xC_Y_MA_16);
    x0 = _mm_srai_epi32(x0, Y_RSH_16);
    x7 = _mm_srai_epi32(x7, Y_RSH_16);
    x0 = _mm_add_epi32(x0, xC_YCC);
    x7 = _mm_add_epi32(x7, xC_YCC);

    x0 = _mm_packus_epi32_simd(x0, x7);

    return x0;
}
static __forceinline __m128i convert_uv_range_after_adding_offset(__m128i x0, const __m128i& xC_UV_MA_16, int UV_RSH_16, const __m128i& xC_YCC, const __m128i& xC_pw_one) {
    __m128i x1;
    x1 = _mm_unpackhi_epi16(x0, xC_pw_one);
    x0 = _mm_unpacklo_epi16(x0, xC_pw_one);

    x0 = _mm_madd_epi16(x0, xC_UV_MA_16);
    x1 = _mm_madd_epi16(x1, xC_UV_MA_16);
    x0 = _mm_srai_epi32(x0, UV_RSH_16);
    x1 = _mm_srai_epi32(x1, UV_RSH_16);
    x0 = _mm_add_epi32(x0, xC_YCC);
    x1 = _mm_add_epi32(x1, xC_YCC);

    x0 = _mm_packus_epi32_simd(x0, x1);

    return x0;
}
static __forceinline __m128i convert_uv_range_from_yc48(__m128i x0, const __m128i& xC_UV_OFFSET_x1, const __m128i& xC_UV_MA_16, int UV_RSH_16, __m128i xC_YCC, const __m128i& xC_pw_one) {
    x0 = _mm_add_epi16(x0, xC_UV_OFFSET_x1);

    return convert_uv_range_after_adding_offset(x0, xC_UV_MA_16, UV_RSH_16, xC_YCC, xC_pw_one);
}
static __forceinline __m128i convert_uv_range_from_yc48_yuv420p(__m128i x0, __m128i x1, const __m128i& xC_UV_OFFSET_x2, const __m128i& xC_UV_MA_16, int UV_RSH_16, const __m128i& xC_YCC, const __m128i& xC_pw_one) {
    x0 = _mm_add_epi16(x0, x1);
    x0 = _mm_add_epi16(x0, xC_UV_OFFSET_x2);

    return convert_uv_range_after_adding_offset(x0, xC_UV_MA_16, UV_RSH_16, xC_YCC, xC_pw_one);
}
static __forceinline __m128i convert_uv_range_from_yc48_420i(__m128i x0, __m128i x1, const __m128i& xC_UV_OFFSET_x1, const __m128i& xC_UV_MA_16_0, const __m128i& xC_UV_MA_16_1, int UV_RSH_16, const __m128i& xC_YCC, const __m128i& xC_pw_one) {
    __m128i x2, x3, x6, x7;
    x0 = _mm_add_epi16(x0, xC_UV_OFFSET_x1);
    x1 = _mm_add_epi16(x1, xC_UV_OFFSET_x1);

    x7 = _mm_unpackhi_epi16(x0, xC_pw_one);
    x6 = _mm_unpacklo_epi16(x0, xC_pw_one);
    x3 = _mm_unpackhi_epi16(x1, xC_pw_one);
    x2 = _mm_unpacklo_epi16(x1, xC_pw_one);

    x6 = _mm_madd_epi16(x6, xC_UV_MA_16_0);
    x7 = _mm_madd_epi16(x7, xC_UV_MA_16_0);
    x2 = _mm_madd_epi16(x2, xC_UV_MA_16_1);
    x3 = _mm_madd_epi16(x3, xC_UV_MA_16_1);
    x0 = _mm_add_epi32(x6, x2);
    x7 = _mm_add_epi32(x7, x3);
    x0 = _mm_srai_epi32(x0, UV_RSH_16);
    x7 = _mm_srai_epi32(x7, UV_RSH_16);
    x0 = _mm_add_epi32(x0, xC_YCC);
    x7 = _mm_add_epi32(x7, xC_YCC);

    x0 = _mm_packus_epi32_simd(x0, x7);

    return x0;
}

static __forceinline void afs_convert_yc48_to_nv16_simd(void *pixel, int dst_pitch, int dst_frame_pixels, const PIXEL_YC *src, int width, int src_pitch, int y_start, int y_fin) {
    uint8_t *dst_Y_line = (uint8_t *)pixel + y_start * dst_pitch;
    uint8_t *dst_C_line = dst_Y_line + dst_frame_pixels;
    const PIXEL_YC *ycp_line = src + y_start * src_pitch;
    const PIXEL_YC *ycp_fin  = src + y_fin * src_pitch;
    const __m128i xC_pw_one = _mm_set1_epi16(1);
    const __m128i xC_YCC = _mm_set1_epi32(1<<LSFT_YCC_8);
    for (; ycp_line < ycp_fin; dst_Y_line += dst_pitch, dst_C_line += dst_pitch, ycp_line += src_pitch) {
        uint8_t *dst_Y = dst_Y_line;
        uint8_t *dst_C = dst_C_line;
        const short *ycp = (const short *)ycp_line;
        int i_step = 0;
        for (int x = width - 16; x >= 0; x -= i_step, ycp += i_step * 3, dst_Y += i_step, dst_C += i_step) {
            __m128i x1, x2, x3, x4, x5, x6;
            x1 = _mm_loadu_si128((const __m128i *)(ycp +  0));
            x2 = _mm_loadu_si128((const __m128i *)(ycp +  8));
            x3 = _mm_loadu_si128((const __m128i *)(ycp + 16));
            gather_y_uv_from_yc48(x1, x2, x3);
            x4 = _mm_loadu_si128((const __m128i *)(ycp + 24));
            x5 = _mm_loadu_si128((const __m128i *)(ycp + 32));
            x6 = _mm_loadu_si128((const __m128i *)(ycp + 40));
            gather_y_uv_from_yc48(x4, x5, x6);

            x1 = convert_y_range_from_yc48(x1, xC_Y_L_MA_8, Y_L_RSH_8, xC_YCC, xC_pw_one);
            x4 = convert_y_range_from_yc48(x4, xC_Y_L_MA_8, Y_L_RSH_8, xC_YCC, xC_pw_one);
            x2 = convert_uv_range_from_yc48(x2, _mm_set1_epi16(UV_OFFSET_x1), xC_UV_L_MA_8_444, UV_L_RSH_8_444, xC_YCC, xC_pw_one);
            x5 = convert_uv_range_from_yc48(x5, _mm_set1_epi16(UV_OFFSET_x1), xC_UV_L_MA_8_444, UV_L_RSH_8_444, xC_YCC, xC_pw_one);
            _mm_storeu_si128((__m128i *)dst_Y, _mm_packus_epi16(x1, x4));
            _mm_storeu_si128((__m128i *)dst_C, _mm_packus_epi16(x2, x5));

            i_step = limit_1_to_16(x);
        }
    }
}
