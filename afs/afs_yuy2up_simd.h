#pragma once

#include "filter.h"

static __forceinline __m128i get_even_uv(BYTE *ptr) {
    __m128i x0, x1, x2;
    x0 = _mm_loadu_si128((__m128i*)(ptr +  0));
    x1 = _mm_loadu_si128((__m128i*)(ptr + 16));
    x2 = _mm_loadu_si128((__m128i*)(ptr + 32));
#if USE_SSE41
    const int MASK_INT_UV = 0x40 + 0x20 + 0x01;
    x0 = _mm_blend_epi16(x0, x1, MASK_INT_UV);
    x0 = _mm_blend_epi16(x0, x2, MASK_INT_UV >> 2);
#else
    static const _declspec(align(16)) USHORT maskUV[8] = { 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000 };
    __m128i xMask = _mm_load_si128((__m128i*)maskUV);
    x0 = select_by_mask(x0, x1, xMask);
    x0 = select_by_mask(x0, x2, _mm_srli_si128(xMask, 4));
#endif
    x0 = _mm_alignr_epi8_simd(x0, x0, 2);
    x0 = _mm_shuffle_epi32(x0, _MM_SHUFFLE(1, 2, 3, 0));
    return x0;
}

static void __forceinline __stdcall afs_yuy2up_frame_simd(PIXEL_YC *dst, int dst_pitch, PIXEL_YC *src, int width, int src_pitch, int y_start, int y_fin) {
    static const _declspec(align(16)) USHORT maskUV_store[24] = {
        0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000,
        0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
        0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff,
    };    
    const int MASK_UV_STORE_0 = 0x20 + 0x10;
    const int MASK_UV_STORE_1 = 0x08 + 0x04;
    const int MASK_UV_STORE_2 = 0x80 + 0x40 + 0x02 + 0x01;
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    x3 = _mm_setzero_si128();
    //x0 = _mm_slli_si128(x1, 4);
    src += y_start * src_pitch;
    dst += y_start * dst_pitch;

    for (int jh = y_start; jh < y_fin; jh++, src += src_pitch, dst += dst_pitch) {
        BYTE *ptr_src = (BYTE *)src;
        BYTE *ptr_dst = (BYTE *)dst;
        x1 = get_even_uv(ptr_src +  0);
        x0 = _mm_shuffle_epi32(x1, _MM_SHUFFLE(0,0,0,0));
        BYTE *ptr_fin = (BYTE *)src + width * 6 - 48;
        for ( ; ptr_src <= ptr_fin; ptr_src += 48, ptr_dst += 48) {
            x2 = get_even_uv(ptr_src + 48);
            x7 = x1;
            x6 = _mm_alignr_epi8_simd(x2, x1, 4);
            x7 = _mm_adds_epi16(x7, x6);
        
            x6 = _mm_adds_epi16(_mm_alignr_epi8_simd(x2, x1, 8), _mm_alignr_epi8_simd(x1, x0, 12));
            x6 = _mm_subs_epi16(x6, x7);
            x6 = _mm_srai_epi16(x6, 3);
        
            x3 = _mm_cmpeq_epi16(x3, x3);
            x3 = _mm_srli_epi16(x3, 15); //create 16bit "0x0001"
            x7 = _mm_adds_epi16(x7, x3);
            x7 = _mm_subs_epi16(x7, x6);
            x7 = _mm_srai_epi16(x7, 1);
        
            x3 = _mm_loadu_si128((__m128i*)(ptr_src +  0));
            x4 = _mm_loadu_si128((__m128i*)(ptr_src + 16));
            x5 = _mm_loadu_si128((__m128i*)(ptr_src + 32));

            x7 = _mm_shuffle_epi32(x7, _MM_SHUFFLE(3,0,1,2));

#if USE_SSE41
            //r0 := (mask0 == 0) ? a0 : b0
            x3 = _mm_blend_epi16(x3, x7, MASK_UV_STORE_0);
            x4 = _mm_blend_epi16(x4, x7, MASK_UV_STORE_1);
            x5 = _mm_blend_epi16(x5, x7, MASK_UV_STORE_2);
#else
            //r0 := (mask0 == 0) ? a0 : b0
            x3 = select_by_mask(x3, x7, _mm_load_si128((__m128i*)&maskUV_store[0 * 8]));
            x4 = select_by_mask(x4, x7, _mm_load_si128((__m128i*)&maskUV_store[1 * 8]));
            x5 = select_by_mask(x5, x7, _mm_load_si128((__m128i*)&maskUV_store[2 * 8]));
#endif
        
            _mm_storeu_si128((__m128i*)(ptr_dst +  0), x3);
            _mm_storeu_si128((__m128i*)(ptr_dst + 16), x4);
            _mm_storeu_si128((__m128i*)(ptr_dst + 32), x5);
        
            x0 = x1;
            x1 = x2;
        }
    
        //last 8 pixels
        ptr_fin = (BYTE *)src + width * 6;
        int remain_width = (ptr_fin - ptr_src) / sizeof(PIXEL_YC);
        if (remain_width < 8) {
            int offset = (8 - (remain_width & (~1))) * 6;
            ptr_src -= offset;
            ptr_dst -= offset;
            x0 = get_even_uv(ptr_src - 48);
            x1 = get_even_uv(ptr_src);
        }
    
        //set the last valid uv value
        x2 = _mm_set1_epi32(*(int *)((BYTE *)src + ((width-1) & (~1)) * 6 + 2));
        x7 = x1;
        x6 = _mm_alignr_epi8_simd(x2, x1, 4);
        x7 = _mm_adds_epi16(x7, x6);
    
        x6 = _mm_adds_epi16(_mm_alignr_epi8_simd(x2, x1, 8), _mm_alignr_epi8_simd(x1, x0, 12));
        x6 = _mm_subs_epi16(x6, x7);
        x6 = _mm_srai_epi16(x6, 3);
        
        x3 = _mm_cmpeq_epi16(x3, x3);
        x3 = _mm_srli_epi16(x3, 15); //create 16bit "0x0001"
        x7 = _mm_adds_epi16(x7, x3);
        x7 = _mm_subs_epi16(x7, x6);
        x7 = _mm_srai_epi16(x7, 1);
    
        x3 = _mm_loadu_si128((__m128i*)(ptr_src +  0));
        x4 = _mm_loadu_si128((__m128i*)(ptr_src + 16));
        x5 = _mm_loadu_si128((__m128i*)(ptr_src + 32));

        x7 = _mm_shuffle_epi32(x7, _MM_SHUFFLE(3,0,1,2));
    
        //r0 := (mask0 == 0) ? a0 : b0
#if USE_SSE41
        x3 = _mm_blend_epi16(x3, x7, MASK_UV_STORE_0);
        x4 = _mm_blend_epi16(x4, x7, MASK_UV_STORE_1);
        x5 = _mm_blend_epi16(x5, x7, MASK_UV_STORE_2);
#else
        //r0 := (mask0 == 0) ? a0 : b0
        x3 = select_by_mask(x3, x7, _mm_load_si128((__m128i*)&maskUV_store[0 * 8]));
        x4 = select_by_mask(x4, x7, _mm_load_si128((__m128i*)&maskUV_store[1 * 8]));
        x5 = select_by_mask(x5, x7, _mm_load_si128((__m128i*)&maskUV_store[2 * 8]));
#endif
    
        _mm_storeu_si128((__m128i*)(ptr_dst +  0), x3);
        _mm_storeu_si128((__m128i*)(ptr_dst + 16), x4);
        _mm_storeu_si128((__m128i*)(ptr_dst + 32), x5);
    }
}
