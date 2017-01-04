#define USE_AVX   1
#define USE_AVX2  1
#include <immintrin.h> //イントリンシック命令 AVX / AVX2
#include <Windows.h>
#include "afs.h"
#include "afs_convert_const.h"
#include "simd_util.h"


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

static __forceinline void separate_low_up(__m256i& y0_return_lower, __m256i& y1_return_upper) {
    __m256i y4, y5;
    const __m256i xMaskLowByte = _mm256_srli_epi16(_mm256_cmpeq_epi8(_mm256_setzero_si256(), _mm256_setzero_si256()), 8);
    y4 = _mm256_srli_epi16(y0_return_lower, 8);
    y5 = _mm256_srli_epi16(y1_return_upper, 8);

    y0_return_lower = _mm256_and_si256(y0_return_lower, xMaskLowByte);
    y1_return_upper = _mm256_and_si256(y1_return_upper, xMaskLowByte);

    y0_return_lower = _mm256_packus_epi16(y0_return_lower, y1_return_upper);
    y1_return_upper = _mm256_packus_epi16(y4, y5);
}
static __forceinline void separate_low_up_16bit(__m256i& y0_return_lower, __m256i& y1_return_upper) {
    __m256i y4, y5;
    const __m256i xMaskLowByte = _mm256_srli_epi32(_mm256_cmpeq_epi8(_mm256_setzero_si256(), _mm256_setzero_si256()), 16);

    y4 = y0_return_lower; //128,   0
    y5 = y1_return_upper; //384, 256
    y0_return_lower = _mm256_permute2x128_si256(y4, y5, (2<<4)+0); //256,   0
    y1_return_upper = _mm256_permute2x128_si256(y4, y5, (3<<4)+1); //384, 128

    y4 = _mm256_srli_epi32(y0_return_lower, 16);
    y5 = _mm256_srli_epi32(y1_return_upper, 16);

    y0_return_lower = _mm256_and_si256(y0_return_lower, xMaskLowByte);
    y1_return_upper = _mm256_and_si256(y1_return_upper, xMaskLowByte);

    y0_return_lower = _mm256_packus_epi32(y0_return_lower, y1_return_upper);
    y1_return_upper = _mm256_packus_epi32(y4, y5);
}

static __forceinline void gather_y_uv_from_yc48(__m256i& y0, __m256i& y1, __m256i y2) {
    const int MASK_INT_Y  = 0x80 + 0x10 + 0x02;
    const int MASK_INT_UV = 0x40 + 0x20 + 0x01;
    __m256i y3 = y0;
    __m256i y4 = y1;
    __m256i y5 = y2;

    y0 = _mm256_blend_epi32(y3, y4, 0xf0);                    // 384, 0
    y1 = _mm256_permute2x128_si256(y3, y5, (0x02<<4) + 0x01); // 512, 128
    y2 = _mm256_blend_epi32(y4, y5, 0xf0);                    // 640, 256

    y3 = _mm256_blend_epi16(y0, y1, MASK_INT_Y);
    y3 = _mm256_blend_epi16(y3, y2, MASK_INT_Y>>2);

    y1 = _mm256_blend_epi16(y0, y1, MASK_INT_UV);
    y1 = _mm256_blend_epi16(y1, y2, MASK_INT_UV>>2);
    y1 = _mm256_alignr_epi8(y1, y1, 2);
    y1 = _mm256_shuffle_epi32(y1, _MM_SHUFFLE(1, 2, 3, 0));//UV1行目

    y0 = _mm256_shuffle_epi8(y3, yC_SUFFLE_YCP_Y);
}

static __forceinline __m256i convert_y_range_from_yc48(__m256i y0, __m256i yC_Y_MA_16, int Y_RSH_16, const __m256i& yC_YCC, const __m256i& yC_pw_one) {
    __m256i y7;

    y7 = _mm256_unpackhi_epi16(y0, yC_pw_one);
    y0 = _mm256_unpacklo_epi16(y0, yC_pw_one);

    y0 = _mm256_madd_epi16(y0, yC_Y_MA_16);
    y7 = _mm256_madd_epi16(y7, yC_Y_MA_16);
    y0 = _mm256_srai_epi32(y0, Y_RSH_16);
    y7 = _mm256_srai_epi32(y7, Y_RSH_16);
    y0 = _mm256_add_epi32(y0, yC_YCC);
    y7 = _mm256_add_epi32(y7, yC_YCC);

    y0 = _mm256_packus_epi32(y0, y7);

    return y0;
}
static __forceinline __m256i convert_uv_range_after_adding_offset(__m256i y0, const __m256i& yC_UV_MA_16, int UV_RSH_16, const __m256i& yC_YCC, const __m256i& yC_pw_one) {
    __m256i y7;
    y7 = _mm256_unpackhi_epi16(y0, yC_pw_one);
    y0 = _mm256_unpacklo_epi16(y0, yC_pw_one);

    y0 = _mm256_madd_epi16(y0, yC_UV_MA_16);
    y7 = _mm256_madd_epi16(y7, yC_UV_MA_16);
    y0 = _mm256_srai_epi32(y0, UV_RSH_16);
    y7 = _mm256_srai_epi32(y7, UV_RSH_16);
    y0 = _mm256_add_epi32(y0, yC_YCC);
    y7 = _mm256_add_epi32(y7, yC_YCC);

    y0 = _mm256_packus_epi32(y0, y7);

    return y0;
}
static __forceinline __m256i convert_uv_range_from_yc48(__m256i y0, const __m256i& yC_UV_OFFSET_x1, const __m256i& yC_UV_MA_16, int UV_RSH_16, const __m256i& yC_YCC, const __m256i& yC_pw_one) {
    y0 = _mm256_add_epi16(y0, yC_UV_OFFSET_x1);

    return convert_uv_range_after_adding_offset(y0, yC_UV_MA_16, UV_RSH_16, yC_YCC, yC_pw_one);
}
static __forceinline __m256i convert_uv_range_from_yc48_yuv420p(__m256i y0, __m256i y1, const __m256i& yC_UV_OFFSET_x2, __m256i yC_UV_MA_16, int UV_RSH_16, const __m256i& yC_YCC, const __m256i& yC_pw_one) {
    y0 = _mm256_add_epi16(y0, y1);
    y0 = _mm256_add_epi16(y0, yC_UV_OFFSET_x2);

    return convert_uv_range_after_adding_offset(y0, yC_UV_MA_16, UV_RSH_16, yC_YCC, yC_pw_one);
}
static __forceinline __m256i convert_uv_range_from_yc48_420i(__m256i y0, __m256i y1, const __m256i& yC_UV_OFFSET_x1, const __m256i& yC_UV_MA_16_0, const __m256i& yC_UV_MA_16_1, int UV_RSH_16, const __m256i& yC_YCC, const __m256i& yC_pw_one) {
    __m256i y2, y3, y6, y7;

    y0 = _mm256_add_epi16(y0, yC_UV_OFFSET_x1);
    y1 = _mm256_add_epi16(y1, yC_UV_OFFSET_x1);

    y7 = _mm256_unpackhi_epi16(y0, yC_pw_one);
    y6 = _mm256_unpacklo_epi16(y0, yC_pw_one);
    y3 = _mm256_unpackhi_epi16(y1, yC_pw_one);
    y2 = _mm256_unpacklo_epi16(y1, yC_pw_one);

    y6 = _mm256_madd_epi16(y6, yC_UV_MA_16_0);
    y7 = _mm256_madd_epi16(y7, yC_UV_MA_16_0);
    y2 = _mm256_madd_epi16(y2, yC_UV_MA_16_1);
    y3 = _mm256_madd_epi16(y3, yC_UV_MA_16_1);
    y0 = _mm256_add_epi32(y6, y2);
    y7 = _mm256_add_epi32(y7, y3);
    y0 = _mm256_srai_epi32(y0, UV_RSH_16);
    y7 = _mm256_srai_epi32(y7, UV_RSH_16);
    y0 = _mm256_add_epi32(y0, yC_YCC);
    y7 = _mm256_add_epi32(y7, yC_YCC);

    y0 = _mm256_packus_epi32(y0, y7);

    return y0;
}

void __stdcall afs_convert_yc48_to_nv16_avx2(void *pixel, int dst_pitch, int dst_frame_pixels, const void *_src, int width, int src_pitch, int y_start, int y_fin) {
    const PIXEL_YC *src = (PIXEL_YC *)_src;
    uint8_t *dst_Y_line = (uint8_t *)pixel + y_start * dst_pitch;
    uint8_t *dst_C_line = dst_Y_line + dst_frame_pixels; //これに置き換えないといけない
    const PIXEL_YC *ycp_line = src + y_start * src_pitch;
    const PIXEL_YC *ycp_fin  = src + y_fin * src_pitch;
    const __m256i yC_pw_one = _mm256_set1_epi16(1);
    const __m256i yC_YCC = _mm256_set1_epi32(1<<LSFT_YCC_8);
    for (; ycp_line < ycp_fin; dst_Y_line += dst_pitch, dst_C_line += dst_pitch, ycp_line += src_pitch) {
        uint8_t *dst_Y = dst_Y_line;
        uint8_t *dst_C = dst_C_line;
        const short *ycp = (const short *)ycp_line;
        for (int i_step = 0, x = width - 32; x >= 0; x -= i_step, ycp += i_step * 3, dst_Y += i_step, dst_C += i_step) {
            __m256i y1, y2, y3, y4, y5, y6;
            y1 = _mm256_loadu_si256((const __m256i *)(ycp +  0));
            y2 = _mm256_loadu_si256((const __m256i *)(ycp + 16));
            y3 = _mm256_loadu_si256((const __m256i *)(ycp + 32));
            gather_y_uv_from_yc48(y1, y2, y3);
            y4 = _mm256_loadu_si256((const __m256i *)(ycp + 48));
            y5 = _mm256_loadu_si256((const __m256i *)(ycp + 64));
            y6 = _mm256_loadu_si256((const __m256i *)(ycp + 80));
            gather_y_uv_from_yc48(y4, y5, y6);

            y1 = convert_y_range_from_yc48(y1, yC_Y_L_MA_8, Y_L_RSH_8, yC_YCC, yC_pw_one);
            y4 = convert_y_range_from_yc48(y4, yC_Y_L_MA_8, Y_L_RSH_8, yC_YCC, yC_pw_one);
            y2 = convert_uv_range_from_yc48(y2, _mm256_set1_epi16(UV_OFFSET_x1), yC_UV_L_MA_8_444, UV_L_RSH_8_444, yC_YCC, yC_pw_one);
            y5 = convert_uv_range_from_yc48(y5, _mm256_set1_epi16(UV_OFFSET_x1), yC_UV_L_MA_8_444, UV_L_RSH_8_444, yC_YCC, yC_pw_one);
            y1 = _mm256_packus_epi16(y1, y4);
            y2 = _mm256_packus_epi16(y2, y5);
            y1 = _mm256_permute4x64_epi64(y1, _MM_SHUFFLE(3, 1, 2, 0));
            y2 = _mm256_permute4x64_epi64(y2, _MM_SHUFFLE(3, 1, 2, 0));
            _mm256_storeu_si256((__m256i *)dst_Y, y1);
            _mm256_storeu_si256((__m256i *)dst_C, y2);

            i_step = limit_1_to_32(x);
        }
    }
    _mm256_zeroupper();
}

void __stdcall afs_copy_yuy2_nv16_avx2(void *pixel, int dst_pitch, int dst_frame_pixels, const void *_src, int width, int src_pitch, int y_start, int y_fin) {
    const uint8_t *src = (const uint8_t *)_src;
    uint8_t *dst = (uint8_t *)pixel;
    src_pitch *= 2;
    src += y_start * src_pitch;
    dst += y_start * dst_pitch;

    for (int jh = y_start; jh < y_fin; jh++, src += src_pitch, dst += dst_pitch) {
        __m256i y0, y1, y2, y3;
        BYTE *ptr_src = (BYTE *)src;
        BYTE *ptr_dst = (BYTE *)dst;
        BYTE *ptr_fin = (BYTE *)ptr_dst + width - 16;
        for (; ptr_dst <= ptr_fin; ptr_src += 64, ptr_dst += 32) {
            y0 = _mm256_loadu2_m128i((__m128i *)(ptr_src + 32), (__m128i *)(ptr_src +  0));
            y1 = _mm256_loadu2_m128i((__m128i *)(ptr_src + 48), (__m128i *)(ptr_src + 16));

            y2 = _mm256_and_si256(y0, _mm256_set1_epi16(0x00ff));
            y3 = _mm256_and_si256(y1, _mm256_set1_epi16(0x00ff));

            y0 = _mm256_srli_epi16(y0, 8);
            y1 = _mm256_srli_epi16(y1, 8);

            _mm256_storeu_si256((__m256i *)(ptr_dst +                0), _mm256_packus_epi16(y2, y3));
            _mm256_storeu_si256((__m256i *)(ptr_dst + dst_frame_pixels), _mm256_packus_epi16(y0, y1));
        }
        if (width & 31) {
            ptr_src -= (32 - (width & 31)) * 2;
            ptr_dst -=  32 - (width & 31);
        }
        y0 = _mm256_loadu2_m128i((__m128i *)(ptr_src + 32), (__m128i *)(ptr_src +  0));
        y1 = _mm256_loadu2_m128i((__m128i *)(ptr_src + 48), (__m128i *)(ptr_src + 16));

        y2 = _mm256_and_si256(y0, _mm256_set1_epi16(0x00ff));
        y3 = _mm256_and_si256(y1, _mm256_set1_epi16(0x00ff));

        y0 = _mm256_srli_epi16(y0, 8);
        y1 = _mm256_srli_epi16(y1, 8);

        _mm256_storeu_si256((__m256i *)(ptr_dst +                0), _mm256_packus_epi16(y2, y3));
        _mm256_storeu_si256((__m256i *)(ptr_dst + dst_frame_pixels), _mm256_packus_epi16(y0, y1));
    }
}
