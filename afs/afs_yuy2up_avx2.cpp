#include <immintrin.h>
#include <Windows.h>
#include "filter.h"
#include "simd_util.h"

#if _MSC_VER >= 1800 && !defined(__AVX__) && !defined(_DEBUG)
static_assert(false, "do not forget to set /arch:AVX or /arch:AVX2 for this file.");
#endif

static __forceinline __m256i get_even_uv_avx2(BYTE *ptr) {
    const int MASK_INT_UV = 0x40 + 0x20 + 0x01;
#if 0
    __m256i yA0 = _mm256_loadu_si256((__m256i*)(ptr +  0));
    __m256i yA1 = _mm256_loadu_si256((__m256i*)(ptr + 32));
    __m256i yA2 = _mm256_loadu_si256((__m256i*)(ptr + 64));

    __m256i y0 = _mm256_permute2x128_si256(yA0, yA1, (0x03<<4) + 0x00); // 384, 0
    __m256i y1 = _mm256_permute2x128_si256(yA0, yA2, (0x02<<4) + 0x01); // 512, 128
    __m256i y2 = _mm256_permute2x128_si256(yA1, yA2, (0x03<<4) + 0x00); // 640, 256

    y0 = _mm256_blend_epi16(y0, y1, MASK_INT_UV);
    y0 = _mm256_blend_epi16(y0, y2, MASK_INT_UV>>2);

    y0 = _mm256_alignr_epi8(y0, y0, 2);
    y0 = _mm256_shuffle_epi32(y0, _MM_SHUFFLE(1, 2, 3, 0));
#else
    __m256i y0 = _mm256_set_m128i(_mm_loadu_si128((__m128i*)(ptr + 66)), _mm_loadu_si128((__m128i*)(ptr +  2)));
    __m256i y1 = _mm256_loadu_si256((__m256i*)(ptr + 34));
    __m256i y2 = _mm256_set_m128i(_mm_loadu_si128((__m128i*)(ptr + 82)), _mm_loadu_si128((__m128i*)(ptr + 18)));

    y0 = _mm256_blend_epi32(y0, y1, 0x80+0x10+0x02);
    y0 = _mm256_blend_epi32(y0, y2, 0x20+0x04);

    y0 = _mm256_shuffle_epi32(y0, _MM_SHUFFLE(1,2,3,0));
#endif
    return y0;
}

void __stdcall afs_yuy2up_frame_avx2(PIXEL_YC *dst, PIXEL_YC *src, int width, int pitch, int y_start, int y_fin) {    
    const int MASK_UV_STORE_0 = 0x20 + 0x10;
    const int MASK_UV_STORE_1 = 0x08 + 0x04;
    const int MASK_UV_STORE_2 = 0x80 + 0x40 + 0x02 + 0x01;
    __m256i y0, y10, y1, y21, y2, y3, y6, y7, yA0, yA1, yA2;
#if 0
    __m256i y4, y5;
#endif
    y3 = _mm256_srli_epi16(_mm256_cmpeq_epi16(_mm256_setzero_si256(), _mm256_setzero_si256()), 15);
    //y0 = _mm256_slli_si256(y1, 4);
    src += y_start * pitch;
    dst += y_start * pitch;

    for (int jh = y_start; jh < y_fin; jh++, src += pitch, dst += pitch) {
        BYTE *ptr_src = (BYTE *)src;
        BYTE *ptr_dst = (BYTE *)dst;
        y1 = get_even_uv_avx2(ptr_src +  0);
        y10 = _mm256_permute4x64_epi64(y1, _MM_SHUFFLE(1,0,1,0));
        y0 = _mm256_shuffle_epi32(y10, _MM_SHUFFLE(0,0,0,0));
        y10 = _mm256_blend_epi32(y10, y0, 0x0f);
        BYTE *ptr_fin = (BYTE *)src + width * 6 - 96;
        for ( ; ptr_src <= ptr_fin; ptr_src += 96, ptr_dst += 96) {
            y2 = get_even_uv_avx2(ptr_src + 96);
            y7 = y1;
            y21 = _mm256_permute2x128_si256(y2, y1, (0x00<<4) + 0x03);
            y6 = _mm256_alignr_epi8(y21, y1, 4);
            y7 = _mm256_adds_epi16(y7, y6);

            y6 = _mm256_adds_epi16(_mm256_alignr_epi8(y21, y1, 8), _mm256_alignr_epi8(y1, y10, 12));
            y6 = _mm256_subs_epi16(y6, y7);
            y6 = _mm256_srai_epi16(y6, 3);

            y7 = _mm256_adds_epi16(y7, y3);
            y7 = _mm256_subs_epi16(y7, y6);
            y7 = _mm256_srai_epi16(y7, 1);

            yA0 = _mm256_loadu_si256((__m256i*)(ptr_src +  0));
            yA1 = _mm256_loadu_si256((__m256i*)(ptr_src + 32));
            yA2 = _mm256_loadu_si256((__m256i*)(ptr_src + 64));
#if 0
            y3 = _mm256_permute2x128_si256(yA0, yA1, (0x03<<4) + 0x00); // 384, 0
            y4 = _mm256_permute2x128_si256(yA0, yA2, (0x02<<4) + 0x01); // 512, 128
            y5 = _mm256_permute2x128_si256(yA1, yA2, (0x03<<4) + 0x00); // 640, 256

            y7 = _mm256_shuffle_epi32(y7, _MM_SHUFFLE(3,0,1,2));
        
            //r0 := (mask0 == 0) ? a0 : b0
            y3 = _mm256_blend_epi16(y3, y7, MASK_UV_STORE_0);
            y4 = _mm256_blend_epi16(y4, y7, MASK_UV_STORE_1);
            y5 = _mm256_blend_epi16(y5, y7, MASK_UV_STORE_2);

            yA0 = _mm256_permute2x128_si256(y3, y4, (0x02<<4) + 0x00); // 128, 0
            yA1 = _mm256_permute2x128_si256(y3, y5, (0x01<<4) + 0x02); // 384, 256
            yA2 = _mm256_permute2x128_si256(y4, y5, (0x03<<4) + 0x01); // 640, 512
#else
            y6 = _mm256_permute4x64_epi64(y7, _MM_SHUFFLE(1,0,3,2));
            y7 = _mm256_blend_epi32(y7, y6, 0x22);
            y7 = _mm256_shuffle_epi32(y7, _MM_SHUFFLE(3,0,1,2));

            yA0 = _mm256_blend_epi32(yA0, y7, 0x20+0x04);
            yA1 = _mm256_blend_epi32(yA1, y7, 0x40+0x08+0x01);
            yA2 = _mm256_blend_epi32(yA2, y7, 0x80+0x10+0x02);
#endif
            _mm256_storeu_si256((__m256i*)(ptr_dst +  0), yA0);
            _mm256_storeu_si256((__m256i*)(ptr_dst + 32), yA1);
            _mm256_storeu_si256((__m256i*)(ptr_dst + 64), yA2);
        
            y0 = y1;
            y1 = y2;
            y10 = y21;
        }
    
        //last 8 pixels
        ptr_fin = (BYTE *)src + width * 6;
        int remain_width = (ptr_fin - ptr_src) / sizeof(PIXEL_YC);
        if (remain_width < 16) {
            int offset = (16 - (remain_width & (~1))) * 6;
            ptr_src -= offset;
            ptr_dst -= offset;
            y0 = get_even_uv_avx2(ptr_src - 96);
            y1 = get_even_uv_avx2(ptr_src);
        }
    
        //set the last valid uv value
        y2 = _mm256_set1_epi32(*(int *)((BYTE *)src + ((width-1) & (~1)) * 6 + 2));
        y7 = y1;
        y6 = _mm256_alignr256_epi8(y2, y1, 4);
        y7 = _mm256_adds_epi16(y7, y6);
    
        y6 = _mm256_adds_epi16(_mm256_alignr256_epi8(y2, y1, 8), _mm256_alignr256_epi8(y1, y0, 28));
        y6 = _mm256_subs_epi16(y6, y7);
        y6 = _mm256_srai_epi16(y6, 3);
        
        y3 = _mm256_cmpeq_epi16(y3, y3);
        y3 = _mm256_srli_epi16(y3, 15); //create 16bit "0x0001"
        y7 = _mm256_adds_epi16(y7, y3);
        y7 = _mm256_subs_epi16(y7, y6);
        y7 = _mm256_srai_epi16(y7, 1);
    
        yA0 = _mm256_loadu_si256((__m256i*)(ptr_src +  0));
        yA1 = _mm256_loadu_si256((__m256i*)(ptr_src + 32));
        yA2 = _mm256_loadu_si256((__m256i*)(ptr_src + 64));
#if 0
        y3 = _mm256_permute2x128_si256(yA0, yA1, (0x03<<4) + 0x00); // 384, 0
        y4 = _mm256_permute2x128_si256(yA0, yA2, (0x02<<4) + 0x01); // 512, 128
        y5 = _mm256_permute2x128_si256(yA1, yA2, (0x03<<4) + 0x00); // 640, 256

        y7 = _mm256_shuffle_epi32(y7, _MM_SHUFFLE(3,0,1,2));
    
        //r0 := (mask0 == 0) ? a0 : b0
        y3 = _mm256_blend_epi16(y3, y7, MASK_UV_STORE_0);
        y4 = _mm256_blend_epi16(y4, y7, MASK_UV_STORE_1);
        y5 = _mm256_blend_epi16(y5, y7, MASK_UV_STORE_2);

        yA0 = _mm256_permute2x128_si256(y3, y4, (0x02<<4) + 0x00); // 128, 0
        yA1 = _mm256_permute2x128_si256(y3, y5, (0x01<<4) + 0x02); // 384, 256
        yA2 = _mm256_permute2x128_si256(y4, y5, (0x03<<4) + 0x01); // 640, 512
#else
        y6 = _mm256_permute4x64_epi64(y7, _MM_SHUFFLE(1,0,3,2));
        y7 = _mm256_blend_epi32(y7, y6, 0x22);
        y7 = _mm256_shuffle_epi32(y7, _MM_SHUFFLE(3,0,1,2));

        yA0 = _mm256_blend_epi32(yA0, y7, 0x20+0x04);
        yA1 = _mm256_blend_epi32(yA1, y7, 0x40+0x08+0x01);
        yA2 = _mm256_blend_epi32(yA2, y7, 0x80+0x10+0x02);
#endif    
        _mm256_storeu_si256((__m256i*)(ptr_dst +  0), yA0);
        _mm256_storeu_si256((__m256i*)(ptr_dst + 32), yA1);
        _mm256_storeu_si256((__m256i*)(ptr_dst + 64), yA2);
    }
    _mm256_zeroupper();
}
