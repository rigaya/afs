#pragma once

#include <algorithm>
#include "simd_util.h"
#include "afs.h"
#include "afs_convert_const.h"

void __forceinline afs_analyze_get_local_scan_clip(AFS_SCAN_CLIP *clip_thread, const AFS_SCAN_CLIP *clip, int pos_x, int analyze_block, int scan_w, int left_additional_clip) {
    int left_clip = clip->left - pos_x;
    int right_clip = pos_x + analyze_block - (scan_w - clip->right);
    clip_thread->left  = std::max(0, left_clip) + left_additional_clip;
    clip_thread->right = std::max(0, right_clip);
}

static const _declspec(align(16)) BYTE pb_thre_count[16]       = { 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03 };

static const _declspec(align(16)) BYTE pw_thre_count2[16]      = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(16)) BYTE pw_thre_count1[16]      = { 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_2stripe_0[16]   = { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_2stripe_1[16]   = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_1stripe_0[16]   = { 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_1stripe_1[16]   = { 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_2motion_0[16]   = { 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04 };
static const _declspec(align(16)) BYTE pw_mask_1motion_0[16]   = { 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40 };

static const _declspec(align(16)) BYTE pw_mask_12stripe_01[16] = { 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_12motion_01[16] = { 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_lowbyte[16]     = { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00 };

static _declspec(align(16)) USHORT pw_thre_shift[8]       = { 0 };
static _declspec(align(16)) USHORT pw_thre_deint[8]       = { 0 };
static _declspec(align(16)) USHORT pw_thre_motion[3][8]   = { 0 };

static const _declspec(align(16)) BYTE pb_mask_2motion_0[16]   = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
static const _declspec(align(16)) BYTE pb_mask_1motion_0[16]   = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

static _declspec(align(16)) BYTE pb_thre_shift[16]       = { 0 };
static _declspec(align(16)) BYTE pb_thre_deint[16]       = { 0 };
static _declspec(align(16)) BYTE pb_thre_motion[3][16]   = { 0 };

static const _declspec(align(16)) BYTE pw_mask_12stripe_0[16]  = { 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00 };
static const _declspec(align(16)) BYTE pw_mask_12stripe_1[16]  = { 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00 };

static const _declspec(align(16)) BYTE pb_mask_1stripe_01[16]  = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };
static const _declspec(align(16)) BYTE pb_mask_12stripe_01[16] = { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 };
static const _declspec(align(16)) BYTE pb_mask_12motion_01[16] = { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc };

static __forceinline int count_motion(__m128i x0, BYTE mc_mask[BLOCK_SIZE_YCP], int x, int y, int y_limit, int top) {
    DWORD heightMask = ((DWORD)(y - top) < (DWORD)y_limit) ? 0xffffffff : 0x00;
#if 1
    //_mm256_movemask_epi8は最上位ビットの取り出しであるから、最上位ビットさえ意識すればよい
    //ここで行いたいのは、「0x40ビットが立っていないこと」であるから、
    //左シフトで最上位ビットに移動させたのち、andnotで反転させながらmc_maskとの論理積をとればよい
    x0 = _mm_andnot_si128(_mm_slli_epi16(x0, 1), _mm_load_si128((__m128i *)(mc_mask + x)));
#else
    static const _declspec(align(16)) BYTE MOTION_COUNT_CHECK[16] = {
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
    };
    
    const __m128i xMotion = _mm_load_si128((__m128i *)MOTION_COUNT_CHECK);
    x0 = _mm_andnot_si128(x0, xMotion);
    x0 = _mm_cmpeq_epi8(x0, xMotion);
    x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)(mc_mask + x)));
#endif
    return popcnt32(heightMask & _mm_movemask_epi8(x0));
}

static void __forceinline __stdcall afs_analyze_set_threshold_simd(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    __m128i x0, x1;
    x0 = _mm_set1_epi16((short)thre_shift);
    _mm_stream_si128((__m128i *)pw_thre_shift, x0);
    
    x1 = _mm_set1_epi16((short)thre_deint);
    _mm_stream_si128((__m128i *)pw_thre_deint, x1);
    
    pw_thre_motion[0][0] = thre_Ymotion;
    pw_thre_motion[0][1] = thre_Cmotion;
    pw_thre_motion[0][2] = thre_Cmotion;
    pw_thre_motion[0][3] = thre_Ymotion;
    pw_thre_motion[0][4] = thre_Cmotion;
    pw_thre_motion[0][5] = thre_Cmotion;
    pw_thre_motion[0][6] = thre_Ymotion;
    pw_thre_motion[0][7] = thre_Cmotion;
    
    pw_thre_motion[1][0] = thre_Cmotion;
    pw_thre_motion[1][1] = thre_Ymotion;
    pw_thre_motion[1][2] = thre_Cmotion;
    pw_thre_motion[1][3] = thre_Cmotion;
    pw_thre_motion[1][4] = thre_Ymotion;
    pw_thre_motion[1][5] = thre_Cmotion;
    pw_thre_motion[1][6] = thre_Cmotion;
    pw_thre_motion[1][7] = thre_Ymotion;
    
    pw_thre_motion[2][0] = thre_Cmotion;
    pw_thre_motion[2][1] = thre_Cmotion;
    pw_thre_motion[2][2] = thre_Ymotion;
    pw_thre_motion[2][3] = thre_Cmotion;
    pw_thre_motion[2][4] = thre_Cmotion;
    pw_thre_motion[2][5] = thre_Ymotion;
    pw_thre_motion[2][6] = thre_Cmotion;
    pw_thre_motion[2][7] = thre_Cmotion;
}

static void __forceinline afs_analyze_set_threshold_nv16_simd(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    __m128i x0, x1;
#if 0
    thre_shift   = CLAMP((thre_shift  *219 + (1<<11))>>12, 0, 127);
    thre_deint   = CLAMP((thre_deint  *219 + (1<<11))>>12, 0, 127);
    thre_Ymotion = CLAMP((thre_Ymotion*219 + (1<<11))>>12, 0, 127);
    thre_Cmotion = CLAMP((thre_Cmotion*  7 + (1<< 6))>> 7, 0, 127);
#else
    thre_shift   = CLAMP((thre_shift  *219 + 383)>>12, 0, 127);
    thre_deint   = CLAMP((thre_deint  *219 + 383)>>12, 0, 127);
    thre_Ymotion = CLAMP((thre_Ymotion*219 + 383)>>12, 0, 127);
    thre_Cmotion = CLAMP((thre_Cmotion*  7 +  66)>> 7, 0, 127);
#endif

    x0 = _mm_set1_epi8((char)thre_shift);
    _mm_stream_si128((__m128i *)pb_thre_shift, x0);

    x1 = _mm_set1_epi8((char)thre_deint);
    _mm_stream_si128((__m128i *)pb_thre_deint, x1);

    x0 = _mm_set1_epi8((char)thre_Ymotion);
    _mm_stream_si128((__m128i *)pb_thre_motion[0], x0);

    x1 = _mm_set1_epi8((char)thre_Cmotion);
    _mm_stream_si128((__m128i *)pb_thre_motion[1], x1);
}

template <BOOL aligned>
static void __forceinline afs_analyze_shrink_info_sub(BYTE *src, __m128i &x0, __m128i &x1) {
    __m128i x2, x3, x6, x7;
    x1 = (aligned) ? _mm_load_si128((__m128i *)(src +  0)) : _mm_loadu_si128((__m128i *)(src +  0));
    x2 = (aligned) ? _mm_load_si128((__m128i *)(src + 16)) : _mm_loadu_si128((__m128i *)(src + 16));
    x3 = (aligned) ? _mm_load_si128((__m128i *)(src + 32)) : _mm_loadu_si128((__m128i *)(src + 32));
#if USE_SSSE3
    __m128i xShuffleArray = _mm_load_si128((__m128i*)Array_SUFFLE_YCP_Y);
  #if USE_SSE41
    const int MASK_INT = 0x40 + 0x08 + 0x01;

    x0 = _mm_blend_epi16(x3, x1, MASK_INT);
    x6 = _mm_blend_epi16(x2, x3, MASK_INT);
    x7 = _mm_blend_epi16(x1, x2, MASK_INT);

    x0 = _mm_blend_epi16(x0, x2, MASK_INT << 1);
    x6 = _mm_blend_epi16(x6, x1, MASK_INT << 1);
    x7 = _mm_blend_epi16(x7, x3, MASK_INT << 1);
  #else
    static const _declspec(align(16)) USHORT maskY[8] = { 0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0x0000 };
    __m128i xMask = _mm_load_si128((__m128i*)maskY);

    x0 = select_by_mask(x3, x1, xMask);
    x6 = select_by_mask(x2, x3, xMask);
    x7 = select_by_mask(x1, x2, xMask);

    xMask = _mm_slli_si128(xMask, 2);

    x0 = select_by_mask(x0, x2, xMask);
    x6 = select_by_mask(x6, x1, xMask);
    x7 = select_by_mask(x7, x3, xMask);
  #endif //USE_SSE41

    x0 = _mm_shuffle_epi8(x0, xShuffleArray); //Y
    x6 = _mm_shuffle_epi8(x6, _mm_alignr_epi8(xShuffleArray, xShuffleArray, 6)); //Cb
    x7 = _mm_shuffle_epi8(x7, _mm_alignr_epi8(xShuffleArray, xShuffleArray, 12)); //Cr
#else
    //select y
    static const _declspec(align(16)) USHORT maskY_select[8] = { 0xffff, 0x0000, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0x0000 };
    __m128i xMask = _mm_load_si128((__m128i*)maskY_select);

    x0 = select_by_mask(x3, x1, xMask);
    xMask = _mm_slli_si128(xMask, 2);
    x0 = select_by_mask(x0, x2, xMask); //52741630

    x6 = _mm_unpacklo_epi16(x0, x0);    //11663300
    x7 = _mm_unpackhi_epi16(x0, x0);    //55227744
    
    static const _declspec(align(16)) USHORT maskY_shuffle[8] = { 0xffff, 0x0000, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000 };
    xMask = _mm_load_si128((__m128i*)maskY_shuffle);
    x0 = select_by_mask(x7, x6, xMask);                 //51627340
    x0 = _mm_shuffle_epi32(x0, _MM_SHUFFLE(1,2,3,0));   //73625140

    x0 = _mm_unpacklo_epi16(x0, _mm_srli_si128(x0, 8)); //75316420
    x0 = _mm_unpacklo_epi16(x0, _mm_srli_si128(x0, 8)); //76543210

    //select uv
    xMask = _mm_srli_si128(_mm_cmpeq_epi8(xMask, xMask), 8); //0x00000000, 0x00000000, 0xffffffff, 0xffffffff
    x6 = select_by_mask(_mm_srli_si128(x2, 2), _mm_srli_si128(x3, 2), xMask); //x  x v4 u4 v6 u6 x  x 
    x7 = select_by_mask(x1, x2, xMask);               //x  x  v1 u1 v3 u3 x  x
    xMask = _mm_slli_si128(xMask, 4);                 //0x00000000, 0xffffffff, 0xffffffff, 0x00000000
    x1 = palignr_sse2(x2, x1, 2);                     //v2 u2  x  x  x  x v0 u0
    x6 = select_by_mask(x1, x6, xMask);               //v2 u2 v4 u4 v6 u6 v0 u0
    x7 = select_by_mask(x3, x7, xMask);               //v7 u7 v1 u1 v3 u3 v5 u5
    x1 = _mm_shuffle_epi32(x6, _MM_SHUFFLE(1,2,3,0)); //v6 u6 v4 u4 v2 u2 v0 u0
    x2 = _mm_shuffle_epi32(x7, _MM_SHUFFLE(3,0,1,2)); //v7 u7 v5 u5 v3 u3 v1 u1

    x6 = _mm_unpacklo_epi16(x1, x2); //v3 v2 u3 u2 v1 v0 u1 u0
    x7 = _mm_unpackhi_epi16(x1, x2); //v7 v6 u7 u6 v5 v4 u5 u4

    x1 = _mm_unpacklo_epi32(x6, x7); //v5 v4 v1 v0 u5 u4 u1 u0
    x2 = _mm_unpackhi_epi32(x6, x7); //v7 v6 v3 v2 u7 u6 u3 u2

    x6 = _mm_unpacklo_epi32(x1, x2); //u7 u6 u5 u4 u3 u2 u1 u0
    x7 = _mm_unpackhi_epi32(x1, x2); //v7 v6 v5 v4 v3 v2 v1 v0
#endif //USE_SSSE3

    x1 = _mm_or_si128(x0, x6);
    x0 = _mm_and_si128(x0, x6);
    x1 = _mm_or_si128(x1, x7);
    x0 = _mm_and_si128(x0, x7);

    x1 = _mm_slli_epi16(x1, 8);
    x0 = _mm_srai_epi16(x0, 8);
    x1 = _mm_srai_epi16(x1, 8);

    x0 = _mm_packs_epi16(x0, x0);
    x1 = _mm_packs_epi16(x1, x1);

    x0 = _mm_and_si128(x0, _mm_load_si128((__m128i*)pb_mask_12motion_01));
    x1 = _mm_and_si128(x1, _mm_load_si128((__m128i*)pb_mask_12stripe_01));

    x0 = _mm_or_si128(x0, x1);
}

//各8pixel分の16bit幅のマスクデータを2つ受け取り、x0とx1に16pixel分の8bit幅の出力を返す
static void __forceinline afs_analyze_shrink_info_sub_nv16(const __m128i& xY0, const __m128i& xY1, const __m128i& xC0, const __m128i& xC1, __m128i &x0, __m128i &x1) {
    const __m128i xLowMask = _mm_set1_epi32(0x0000ffff);
    __m128i x2, x3, x6, x7;
#if USE_SSE41
    x6 = _mm_blend_epi16(xC0, _mm_slli_epi32(xC0, 16), 0x80+0x20+0x08+0x02);
    x7 = _mm_blend_epi16(xC0, _mm_srli_epi32(xC0, 16), 0x40+0x10+0x04+0x01);
#else
    x6 = _mm_or_si128(_mm_and_si128(   xLowMask, xC0), _mm_slli_epi32(xC0, 16));
    x7 = _mm_or_si128(_mm_andnot_si128(xLowMask, xC0), _mm_srli_epi32(xC0, 16));
#endif
    x0 = xY0;

    x1 = _mm_or_si128(x0, x6);
    //YUV422で間引かれている色差は、1ピクセル分マスクを広げて論理和をとる
    x6 = _mm_or_si128(x6, _mm_srli_si128(x6, 2));
    x0 = _mm_and_si128(x0, x6);
    x1 = _mm_or_si128(x1, x7);
    //YUV422で間引かれている色差は、1ピクセル分マスクを広げて論理和をとる
    x7 = _mm_or_si128(x7, _mm_srli_si128(x7, 2));
    x0 = _mm_and_si128(x0, x7);
    x1 = _mm_slli_epi16(x1, 8);
    x2 = _mm_srai_epi16(x0, 8);
    x3 = _mm_srai_epi16(x1, 8);

#if USE_SSE41
    x6 = _mm_blend_epi16(xC1, _mm_slli_epi32(xC1, 16), 0x80+0x20+0x08+0x02);
    x7 = _mm_blend_epi16(xC1, _mm_srli_epi32(xC1, 16), 0x40+0x10+0x04+0x01);
#else
    x6 = _mm_or_si128(_mm_and_si128(   xLowMask, xC1), _mm_slli_epi32(xC1, 16));
    x7 = _mm_or_si128(_mm_andnot_si128(xLowMask, xC1), _mm_srli_epi32(xC1, 16));
#endif
    x0 = xY1;

    x1 = _mm_or_si128(x0, x6);
    //YUV422で間引かれている色差は、1ピクセル分マスクを広げて論理和をとる
    x6 = _mm_or_si128(x6, _mm_srli_si128(x6, 2));
    x0 = _mm_and_si128(x0, x6);
    x1 = _mm_or_si128(x1, x7);
    //YUV422で間引かれている色差は、1ピクセル分マスクを広げて論理和をとる
    x7 = _mm_or_si128(x7, _mm_srli_si128(x7, 2));
    x0 = _mm_and_si128(x0, x7);
    x1 = _mm_slli_epi16(x1, 8);
    x0 = _mm_srai_epi16(x0, 8);
    x1 = _mm_srai_epi16(x1, 8);

    x0 = _mm_packs_epi16(x2, x0);
    x1 = _mm_packs_epi16(x3, x1);

    x0 = _mm_and_si128(x0, _mm_load_si128((__m128i*)pb_mask_12motion_01));
    x1 = _mm_and_si128(x1, _mm_load_si128((__m128i*)pb_mask_12stripe_01));

    x0 = _mm_or_si128(x0, x1);
}

/*
以下 高速化版+6までの旧コード
static void __forceinline __stdcall afs_analyze_shrink_info_simd(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch, DWORD simd) {
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_src = (BYTE *)src;
    __m128i x0, x1, x2, x4, x5, x6, x7;
    __m128i buf[4];

    int ih = 0;
    for (; ih < 4; ih++, ptr_src += 48) {
        afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
        buf[ih] = x0;
    }
    x7 = buf[0];
    x6 = buf[1];
    x5 = buf[2];
    x4 = buf[3];
    for (; ih < h; ih++, ptr_src += 48, ptr_dst += si_pitch) {
        afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
        x2 = x6;
        x2 = _mm_or_si128(x2, x5);
        x2 = _mm_or_si128(x2, x4);
        x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
        x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
        x2 = _mm_or_si128(x2, x1);
        x2 = _mm_or_si128(x2, x7);
        _mm_storel_epi64((__m128i*)ptr_dst, x2);
        x7 = x6;
        x6 = x5;
        x5 = x4;
        x4 = x0;
    }

    x2 = x6;
    x2 = _mm_or_si128(x2, x5);
    x2 = _mm_or_si128(x2, x4);
    x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
    x2 = _mm_or_si128(x2, x7);
    _mm_storel_epi64((__m128i*)ptr_dst, x2);
    ptr_dst += si_pitch;

    x2 = x5;
    x2 = _mm_or_si128(x2, x4);
    x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
    x2 = _mm_or_si128(x2, x6);
    _mm_storel_epi64((__m128i*)ptr_dst, x2);
    ptr_dst += si_pitch;

    x2 = x4;
    x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
    x2 = _mm_or_si128(x2, x5);
    _mm_storel_epi64((__m128i*)ptr_dst, x2);
    ptr_dst += si_pitch;
    
    _mm_storel_epi64((__m128i*)ptr_dst, x4);
}

static void __forceinline __stdcall afs_analyze_shrink_info_simd_plus(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch, DWORD simd) {
    __m128i x0, x1, x2, x4, x5, x6, x7;
    __m128i buf[4];

    for (int jw = 0; jw < width; jw += 8, dst += 8, src += 8 * h) {
        BYTE *ptr_dst = (BYTE *)dst;
        BYTE *ptr_src = (BYTE *)src;
        int ih = 0;
        for (; ih < 4; ih++, ptr_src += 48) {
            afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
            buf[ih] = x0;
        }
        x7 = buf[0];
        x6 = buf[1];
        x5 = buf[2];
        x4 = buf[3];
        for (; ih < h; ih++, ptr_src += 48, ptr_dst += si_pitch) {
            afs_analyze_shrink_info_sub_simd(ptr_src, x0, x1);
            x2 = x6;
            x2 = _mm_or_si128(x2, x5);
            x2 = _mm_or_si128(x2, x4);
            x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
            x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
            x2 = _mm_or_si128(x2, x1);
            x2 = _mm_or_si128(x2, x7);
            _mm_storel_epi64((__m128i*)ptr_dst, x2);
            x7 = x6;
            x6 = x5;
            x5 = x4;
            x4 = x0;
        }

        x2 = x6;
        x2 = _mm_or_si128(x2, x5);
        x2 = _mm_or_si128(x2, x4);
        x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
        x2 = _mm_or_si128(x2, x7);
        _mm_storel_epi64((__m128i*)ptr_dst, x2);
        ptr_dst += si_pitch;

        x2 = x5;
        x2 = _mm_or_si128(x2, x4);
        x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
        x2 = _mm_or_si128(x2, x6);
        _mm_storel_epi64((__m128i*)ptr_dst, x2);
        ptr_dst += si_pitch;

        x2 = x4;
        x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
        x2 = _mm_or_si128(x2, x5);
        _mm_storel_epi64((__m128i*)ptr_dst, x2);
        ptr_dst += si_pitch;
    
        _mm_storel_epi64((__m128i*)ptr_dst, x4);        
    }
}
*/
static void __forceinline __stdcall afs_analyze_12_simd_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    const int step6 = step * 6;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * ((COMPRESS_BUF) ? 2 : 4);
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (height - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    BYTE *buf_ptr, *buf2_ptr;
    BYTE *ptr[2];
    int ih;
    
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    BYTE __declspec(align(16)) tmp8pix[48];
    BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;
    
    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;

    if (h_start == 0) {
        for (int kw = 0; kw < width; kw += 8, buf2_ptr += 8) {
            _mm_prefetch((char *)ptr_p0 + step6, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + step6, _MM_HINT_T0);
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
                x0 = _mm_loadu_si128((__m128i *)ptr_p0);
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x0);
#else
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0);
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1);
#endif
                x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
                x2 = _mm_load_si128((__m128i *)pw_thre_shift);
                x3 = _mm_cmpgt_epi16(x3, x0);
                x2 = _mm_cmpgt_epi16(x2, x0);
                x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
                x3 = _mm_or_si128(x3, x2);
                _mm_store_si128((__m128i *)(tmp8pix + jw*16), x3);
            }
            afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
            _mm_storel_epi64((__m128i*)(buf2_ptr), x0);
        }
    } else {
        p0 += step * (h_start - 1);
        p1 += step * (h_start - 1);
        dst += si_pitch * h_start;
    }

    __m64 m0 = _mm_setzero_si64();

    int h_loop_fin = std::min(height, h_fin + 4);
    for (ih = h_start + ((h_start == 0) ? 1 : 0); ih < h_loop_fin; ih++, p0 += step, p1 += step) {
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 8, buf2_ptr += 8) {
            _mm_prefetch((char *)ptr_p0 + (step6 << 1), _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + (step6 << 1), _MM_HINT_T0);
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += ((COMPRESS_BUF) ? 32 : 64)) {
                ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
                ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
                x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step6));
                x1 = x0;
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6)));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x0);
#else
                x2 = _mm_setzero_si128();
                x2 = _mm_cmpgt_epi16(x2, x0);
                x0 = _mm_xor_si128(x0, x2);
                x0 = _mm_subs_epi16(x0, x2);
#endif
                x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
                x2 = _mm_load_si128((__m128i *)pw_thre_shift);
                x3 = _mm_cmpgt_epi16(x3, x0);
                x2 = _mm_cmpgt_epi16(x2, x0);
                x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
                x3 = _mm_or_si128(x3, x2);
            
                x2 = _mm_loadu_si128((__m128i *)ptr_p0);
                x2 = _mm_subs_epi16(x2, x1);
#if USE_SSSE3
                x0 = _mm_abs_epi16(x2);
                x2 = _mm_cmpeq_epi16(x2, x0);
#else
                x0 = x2;
                x2 = _mm_setzero_si128();
                x2 = _mm_cmpgt_epi16(x2, x0);
                x0 = _mm_xor_si128(x0, x2);
                x0 = _mm_subs_epi16(x0, x2);
#endif
                x1 = x0;
                x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
                x1 = _mm_packs_epi16(x1, x1);
                x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
                x0 = _mm_packs_epi16(x0, x0);
                x1 = _mm_unpacklo_epi8(x1, x0);
#if COMPRESS_BUF
                x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
#if USE_SSSE3
                x6 = _mm_abs_epi16(x7); //絶対値がcountの値
#else
                x6 = _mm_srli_epi16(_mm_slli_epi16(x7, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
                x7 = _mm_srai_epi16(x7, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
                x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
                x6 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#endif
                x6 = _mm_and_si128(_mm_and_si128(x6, _mm_xor_si128(x2, x7)), x1);
                x6 = _mm_subs_epi8(x6, x1);
#if COMPRESS_BUF
#if USE_SSSE3
                //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
                //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
                x0 = _mm_andnot_si128(x2, x6);
                x2 = _mm_sign_epi16(x6, x2);
                x2 = _mm_or_si128(x2, x0);
#else
                x2 = _mm_or_si128(x6, _mm_slli_epi16(x2, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
                _mm_store_si128((__m128i *)(buf_ptr +  0), x2);
#else
                _mm_store_si128((__m128i *)(buf_ptr +  0), x2);
                _mm_store_si128((__m128i *)(buf_ptr + 16), x6);
#endif

                x0 = x6;
                x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
                x0 = _mm_srli_epi16(x0, 4);
                x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_0));
                x3 = _mm_or_si128(x3, x0);
            
                x2 = _mm_loadu_si128((__m128i *)(ptr[0]));
                x2 = _mm_subs_epi16(x2, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x2);
                x2 = _mm_cmpeq_epi16(x2, x0);
#else
                x0 = x2;
                x2 = _mm_setzero_si128();
                x2 = _mm_cmpgt_epi16(x2, x0);
                x0 = _mm_xor_si128(x0, x2);
                x0 = _mm_subs_epi16(x0, x2);
#endif
                x1 = x0;
                x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
                x1 = _mm_packs_epi16(x1, x1);
                x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
                x0 = _mm_packs_epi16(x0, x0);
                x1 = _mm_unpacklo_epi8(x1, x0);
#if COMPRESS_BUF
                x5 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#if USE_SSSE3
                x4 = _mm_abs_epi16(x5); //絶対値がcountの値
#else
                x4 = _mm_srli_epi16(_mm_slli_epi16(x5, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
                x5 = _mm_srai_epi16(x5, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
                x5 = _mm_load_si128((__m128i *)(buf_ptr + 32));
                x4 = _mm_load_si128((__m128i *)(buf_ptr + 48));
#endif
                x4 = _mm_and_si128(_mm_and_si128(x4, _mm_xor_si128(x2, x5)), x1);
                x4 = _mm_subs_epi8(x4, x1);
#if COMPRESS_BUF
#if USE_SSSE3
                //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
                //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
                x0 = _mm_andnot_si128(x2, x4);
                x2 = _mm_sign_epi16(x4, x2);
                x2 = _mm_or_si128(x2, x0);
#else
                x2 = _mm_or_si128(x4, _mm_slli_epi16(x2, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
                _mm_store_si128((__m128i *)(buf_ptr + 16), x2);
#else
                _mm_store_si128((__m128i *)(buf_ptr + 32), x2);
                _mm_store_si128((__m128i *)(buf_ptr + 48), x4);
#endif
                x0 = x4;
                x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
                x0 = _mm_srli_epi16(x0, 4);
                x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_1));
                x3 = _mm_or_si128(x3, x0);
            
                _mm_store_si128((__m128i *)(tmp8pix + jw*16), x3);
            }
            afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
            _mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
            _mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
        }

        if (ih >= h_start + 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
                x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                x2 = x6;
                x2 = _mm_or_si128(x2, x5);
                x2 = _mm_or_si128(x2, x4);
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
                x2 = _mm_or_si128(x2, x1);
                x2 = _mm_or_si128(x2, x7);
                _mm_storeu_si128((__m128i*)ptr_dst, x2);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    h_loop_fin = std::min(height + 4, h_fin + 4);
    for (; ih < h_loop_fin; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
            x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            x2 = x6;
            x2 = _mm_or_si128(x2, x5);
            x2 = _mm_or_si128(x2, x4);
            x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
            x2 = _mm_or_si128(x2, x7);
            _mm_storeu_si128((__m128i*)ptr_dst, x2);
            _mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
        }
        dst += si_pitch;
    }
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}


static void __forceinline __stdcall afs_analyze_1_simd_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    const int step6 = step * 6;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * ((COMPRESS_BUF) ? 2 : 4);
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (height - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    int motion_count_tmp[2] = { 0, 0 };
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    BYTE *buf_ptr, *buf2_ptr;
    BYTE *ptr[2];
    int ih;
    
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    BYTE __declspec(align(16)) tmp8pix[48];
    BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;
    
    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;
    
    x3 = _mm_load_si128((__m128i *)pw_thre_shift);
    if (h_start == 0) {
        for (int kw = 0; kw < width; kw += 8, buf2_ptr += 8) {
            _mm_prefetch((char *)ptr_p0 + step6, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + step6, _MM_HINT_T0);
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
                //afs_analyze_1_mmx_sub
                x0 = _mm_loadu_si128((__m128i *)ptr_p0);
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
#if USE_SSSE3
                x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
                x1 = _mm_setzero_si128();
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
                x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
                x2 = x3;
                x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h

                _mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
            }
            afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
            _mm_storel_epi64((__m128i*)(buf2_ptr), x0);
        }
    } else {
        p0 += step * (h_start - 1);
        p1 += step * (h_start - 1);
        dst += si_pitch * h_start;
    }

    __m64 m0 = _mm_setzero_si64();
        
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;

    int h_loop_fin = std::min(height, h_fin + 4);
    for (ih = h_start + ((h_start == 0) ? 1 : 0); ih < h_loop_fin; ih++, p0 += step, p1 += step) {
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 8, buf2_ptr += 8) {
            _mm_prefetch((char *)ptr_p0 + (step6 << 1), _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + (step6 << 1), _MM_HINT_T0);
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += ((COMPRESS_BUF) ? 32 : 64)) {
                ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
                ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
                //afs_analyze_1_mmx_loop
                //former field line
                //analyze motion
                x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
#if USE_SSSE3
                x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
                x2 = x3;
                x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
                
                //analyze non-shift
                x1 = _mm_loadu_si128((__m128i *)ptr_p0);
                x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0 + step6)));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x1);
                x1 = _mm_cmpeq_epi16(x1, x0);
#else
                x0 = x1;
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0);
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1);
#endif
                x0 = _mm_cmpgt_epi16(x0, x3);
#if COMPRESS_BUF
                x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
#if USE_SSSE3
                x6 = _mm_abs_epi16(x7); //絶対値がcountの値
#else
                x6 = _mm_srli_epi16(_mm_slli_epi16(x7, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
                x7 = _mm_srai_epi16(x7, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
                x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
                x6 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#endif
                x6 = _mm_and_si128(_mm_and_si128(x6, _mm_xor_si128(x1, x7)), x0);
                x6 = _mm_subs_epi16(x6, x0);
#if COMPRESS_BUF
#if USE_SSSE3
                //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
                //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
                x0 = _mm_andnot_si128(x1, x6);
                x1 = _mm_sign_epi16(x6, x1);
                x1 = _mm_or_si128(x1, x0);
#else
                x1 = _mm_or_si128(x6, _mm_slli_epi16(x1, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
                _mm_store_si128((__m128i *)(buf_ptr +  0), x1);
#else
                _mm_store_si128((__m128i *)(buf_ptr +  0), x1);
                _mm_store_si128((__m128i *)(buf_ptr + 16), x6);
#endif

                x1 = x6;
                x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_0));
                x2 = _mm_or_si128(x2, x1);

                
                //analyze shift
                x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
                x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1] + step6)));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x1);
                x1 = _mm_cmpeq_epi16(x1, x0);
#else
                x0 = x1;
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0);
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1);
#endif
                x0 = _mm_cmpgt_epi16(x0, x3);
#if COMPRESS_BUF
                x5 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#if USE_SSSE3
                x4 = _mm_abs_epi16(x5); //絶対値がcountの値
#else
                x4 = _mm_srli_epi16(_mm_slli_epi16(x5, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
                x5 = _mm_srai_epi16(x5, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
                x5 = _mm_load_si128((__m128i *)(buf_ptr + 32));
                x4 = _mm_load_si128((__m128i *)(buf_ptr + 48));
#endif
                x4 = _mm_and_si128(_mm_and_si128(x4, _mm_xor_si128(x1, x5)), x0);
                x4 = _mm_subs_epi16(x4, x0);
#if COMPRESS_BUF
#if USE_SSSE3
                //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
                //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
                x0 = _mm_andnot_si128(x1, x4);
                x1 = _mm_sign_epi16(x4, x1);
                x1 = _mm_or_si128(x1, x0);
#else
                x1 = _mm_or_si128(x4, _mm_slli_epi16(x1, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
                _mm_store_si128((__m128i *)(buf_ptr + 16), x1);
#else
                _mm_store_si128((__m128i *)(buf_ptr + 32), x1);
                _mm_store_si128((__m128i *)(buf_ptr + 48), x4);
#endif
                
                x1 = x4;
                x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_1));
                x2 = _mm_or_si128(x2, x1);
                
                _mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
            }
            afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
            _mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
            _mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
        }

        if (ih >= 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
                x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                x2 = x6;
                x2 = _mm_or_si128(x2, x5);
                x2 = _mm_or_si128(x2, x4);
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
                x2 = _mm_or_si128(x2, x1);
                x2 = _mm_or_si128(x2, x7);
                _mm_storeu_si128((__m128i*)ptr_dst, x2);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    h_loop_fin = std::min(height + 4, h_fin + 4);
    for (; ih < h_loop_fin; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
            x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            x2 = x6;
            x2 = _mm_or_si128(x2, x5);
            x2 = _mm_or_si128(x2, x4);
            x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
            x2 = _mm_or_si128(x2, x7);
            _mm_storeu_si128((__m128i*)ptr_dst, x2);
            _mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
        }
        dst += si_pitch;
    }
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}

static void __forceinline __stdcall afs_analyze_2_simd_plus2(BYTE *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    const int step6 = step * 6;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * ((COMPRESS_BUF) ? 2 : 4);
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (height - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    BYTE *buf_ptr, *buf2_ptr;
    BYTE *ptr[2];
    int ih;
    
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    BYTE __declspec(align(16)) tmp8pix[48];
    BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;
    
    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;

    if (h_start == 0) {
        for (int kw = 0; kw < width; kw += 8, buf2_ptr += 8) {
            _mm_prefetch((char *)ptr_p0 + step6, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + step6, _MM_HINT_T0);
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
                x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
                //afs_analyze_2_mmx_sub
                x0 = _mm_loadu_si128((__m128i *)ptr_p0);
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
#if USE_SSSE3
                x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
                x2 = x3;
                x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h

                _mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
            }
            afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
            _mm_storel_epi64((__m128i*)(buf2_ptr), x0);
        }
    } else {
        p0 += step * (h_start - 1);
        p1 += step * (h_start - 1);
        dst += si_pitch * h_start;
    }

    __m64 m0 = _mm_setzero_si64();
        
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;

    int h_loop_fin = std::min(height, h_fin + 4);
    for (ih = h_start + ((h_start == 0) ? 1 : 0); ih < h_loop_fin; ih++, p0 += step, p1 += step) {
        ptr_dst = (BYTE *)dst;
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 8, buf2_ptr += 8) {
            _mm_prefetch((char *)ptr_p0 + (step6 << 1), _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + (step6 << 1), _MM_HINT_T0);
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += ((COMPRESS_BUF) ? 32 : 64)) {
                x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
                ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
                ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
                //afs_analyze_1_mmx_loop
                //former field line
                //analyze motion
                x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
                x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
#if USE_SSSE3
                x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
#else
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
#endif
                x2 = x3;
                x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
                
                //analyze non-shift
                x1 = _mm_loadu_si128((__m128i *)ptr_p0);
                x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0 + step6)));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x1);
                x1 = _mm_cmpeq_epi16(x1, x0);
#else
                x0 = x1;
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0);
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1);
#endif
                x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
#if COMPRESS_BUF
                x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
#if USE_SSSE3
                x6 = _mm_abs_epi16(x7); //絶対値がcountの値
#else
                x6 = _mm_srli_epi16(_mm_slli_epi16(x7, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
                x7 = _mm_srai_epi16(x7, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
                x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
                x6 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#endif
                x6 = _mm_and_si128(_mm_and_si128(x6, _mm_xor_si128(x1, x7)), x0);
                x6 = _mm_subs_epi16(x6, x0);
#if COMPRESS_BUF
#if USE_SSSE3
                //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
                //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
                x0 = _mm_andnot_si128(x1, x6);
                x1 = _mm_sign_epi16(x6, x1);
                x1 = _mm_or_si128(x1, x0);
#else
                x1 = _mm_or_si128(x6, _mm_slli_epi16(x1, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
                _mm_store_si128((__m128i *)(buf_ptr +  0), x1);
#else
                _mm_store_si128((__m128i *)(buf_ptr +  0), x1);
                _mm_store_si128((__m128i *)(buf_ptr + 16), x6);
#endif
                x1 = x6;
                x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_0));
                x2 = _mm_or_si128(x2, x1);
                
                //analyze shift
                x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
                x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1] + step6)));
#if USE_SSSE3
                x0 = _mm_abs_epi16(x1);
                x1 = _mm_cmpeq_epi16(x1, x0);
#else
                x0 = x1;
                x1 = _mm_setzero_si128();
                x1 = _mm_cmpgt_epi16(x1, x0);
                x0 = _mm_xor_si128(x0, x1);
                x0 = _mm_subs_epi16(x0, x1);
#endif
                x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
#if COMPRESS_BUF
                x5 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#if USE_SSSE3
                x4 = _mm_abs_epi16(x5); //絶対値がcountの値
#else
                x4 = _mm_srli_epi16(_mm_slli_epi16(x5, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
                x5 = _mm_srai_epi16(x5, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
                x5 = _mm_load_si128((__m128i *)(buf_ptr + 32));
                x4 = _mm_load_si128((__m128i *)(buf_ptr + 48));
#endif
                x4 = _mm_and_si128(_mm_and_si128(x4, _mm_xor_si128(x1, x5)), x0);
                x4 = _mm_subs_epi16(x4, x0);
#if COMPRESS_BUF
#if USE_SSSE3
                //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
                //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
                x0 = _mm_andnot_si128(x1, x4);
                x1 = _mm_sign_epi16(x4, x1);
                x1 = _mm_or_si128(x1, x0);
#else
                x1 = _mm_or_si128(x4, _mm_slli_epi16(x1, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
                _mm_store_si128((__m128i *)(buf_ptr + 16), x1);
#else
                _mm_store_si128((__m128i *)(buf_ptr + 32), x1);
                _mm_store_si128((__m128i *)(buf_ptr + 48), x4);
#endif
                x1 = x4;
                x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_1));
                x2 = _mm_or_si128(x2, x1);
                
                _mm_store_si128((__m128i *)(tmp8pix + jw*16), x2);
            }
            afs_analyze_shrink_info_sub<TRUE>(tmp8pix, x0, x1);
            _mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
            _mm_storel_epi64((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
        }

        if (ih >= h_start + 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
                x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                x2 = x6;
                x2 = _mm_or_si128(x2, x5);
                x2 = _mm_or_si128(x2, x4);
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
                x2 = _mm_or_si128(x2, x1);
                x2 = _mm_or_si128(x2, x7);
                _mm_storeu_si128((__m128i*)ptr_dst, x2);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    h_loop_fin = std::min(height + 4, h_fin + 4);
    for (; ih < h_loop_fin; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
            x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            x2 = x6;
            x2 = _mm_or_si128(x2, x5);
            x2 = _mm_or_si128(x2, x4);
            x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
            x2 = _mm_or_si128(x2, x7);
            _mm_storeu_si128((__m128i*)ptr_dst, x2);
            _mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
        }
        dst += si_pitch;
    }
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}

static __m128i __forceinline afs_abs_epi8(__m128i x0) {
#if USE_SSSE3
    return _mm_abs_epi8(x0);
#else
    __m128i x1;
    x1 = _mm_setzero_si128();
    x1 = _mm_cmpgt_epi8(x1, x0);
    x0 = _mm_xor_si128(x0, x1);
    return _mm_sub_epi8(x0, x1);
#endif
}

//出力 16pixel分の8bitマスク
static __m128i __forceinline afs_analyze_motion(__m128i x0, __m128i x1, int i_thre_motion) {
    __m128i x2, x3;
    x0 = _mm_sub_epi8(_mm_max_epu8(x0, x1), _mm_min_epu8(x0, x1));
    x0 = _mm_min_epu8(x0, _mm_set1_epi8(127));
    x3 = _mm_load_si128((__m128i *)pb_thre_motion[i_thre_motion]);
    x2 = _mm_load_si128((__m128i *)pb_thre_shift);
    x3 = _mm_cmpgt_epi8(x3, x0);
    x2 = _mm_cmpgt_epi8(x2, x0);
    x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pb_mask_2motion_0));
    x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_1motion_0));
    x3 = _mm_or_si128(x3, x2);
    return x3;
}

static __m128i __forceinline afs_analyze_element_stripe_nv16(__m128i x0, __m128i x2, BYTE *buf_ptr, const BYTE *pw_mask_12stripe) {
    __m128i x6, x7;
#if COMPRESS_BUF
    x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
#if USE_SSSE3
    x6 = _mm_abs_epi16(x7); //絶対値がcountの値
#else
    x6 = _mm_srli_epi16(_mm_slli_epi16(x7, 1), 1); //最上位ビットを取り除いたものがcountの値
#endif
    x7 = _mm_srai_epi16(x7, 15); //符号付きとしてシフトし、0xffffか0x0000を作る
#else
    x7 = _mm_load_si128((__m128i *)(buf_ptr +  0));
    x6 = _mm_load_si128((__m128i *)(buf_ptr + 16));
#endif
    x6 = _mm_and_si128(_mm_and_si128(x6, _mm_xor_si128(x2, x7)), x0);
    x6 = _mm_subs_epi8(x6, x0);
#if COMPRESS_BUF
#if USE_SSSE3
    //_mm_sign_epi16を使用して、符号の状況を示すフラグを0xffffならカウントを負の値にして格納、0x0000ならそのまま正の値で格納する
    //ただ、_mm_sign_epi16はフラグが0だと戻り値が0になってしまうので、そこはあとから加算する
    x0 = _mm_andnot_si128(x2, x6);
    x2 = _mm_sign_epi16(x6, x2);
    x2 = _mm_or_si128(x2, x0);
#else
    x2 = _mm_or_si128(x6, _mm_slli_epi16(x2, 15));  //符号の状況を示すフラグを最上位ビットに格納する
#endif
    _mm_store_si128((__m128i *)(buf_ptr +  0), x2);
#else
    _mm_store_si128((__m128i *)(buf_ptr +  0), x2);
    _mm_store_si128((__m128i *)(buf_ptr + 16), x6);
#endif

    x0 = x6;
    x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
    x0 = _mm_srli_epi16(x0, 4);
    x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe));
    return x0;
}

//出力 x0, x1 ... 16bit幅で8pixelずつ(計16pixel)のマスクデータ
static void __forceinline afs_analyze_stripe_nv16(__m128i& x0, __m128i& x1, BYTE *buf_ptr, const BYTE *pw_mask_12stripe) {
    __m128i x2, x4, x5;
    x2 = x0;
    x0 = _mm_sub_epi8(_mm_max_epu8(x2, x1), _mm_min_epu8(x2, x1));
    x2 = _mm_cmpeq_epi8(_mm_max_epu8(x2, x1), x2);
    x0 = _mm_min_epu8(x0, _mm_set1_epi8(127));
    x1 = x0;
    x1 = _mm_cmpgt_epi8(x1, _mm_load_si128((__m128i *)pb_thre_deint));
    x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_shift));
    x4 = _mm_unpacklo_epi8(x1, x0);
    x5 = _mm_unpackhi_epi8(x1, x0);
    //
    x0 = afs_analyze_element_stripe_nv16(x4, _mm_unpacklo_epi8(x2, x2), buf_ptr +  0, pw_mask_12stripe);
    x1 = afs_analyze_element_stripe_nv16(x5, _mm_unpackhi_epi8(x2, x2), buf_ptr + ((COMPRESS_BUF) ? 16 : 32), pw_mask_12stripe);
}

//出力 x0, x1 ... 16bit幅で8pixelずつ(計16pixel)のマスクデータ
static void __forceinline afs_analyze_count_stripe_nv16(__m128i& x0, __m128i& x1, BYTE *ptr_p0, BYTE *ptr_p1, BYTE *buf_ptr, int i_thre_motion, int step, int tb_order, int ih) {
    BYTE *ptr[2];
    ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
    ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
    _mm_prefetch((char *)ptr_p0 + (step << 1), _MM_HINT_T0);
    _mm_prefetch((char *)ptr_p1 + (step << 1), _MM_HINT_T0);

    __m128i x3, x4, x6;

    //analyze motion
    x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step));
    x1 = _mm_loadu_si128((__m128i *)(ptr_p1 + step));
    x6 = afs_analyze_motion(x0, x1, i_thre_motion);

    //analyze non-shift
    x1 = _mm_loadu_si128((__m128i *)ptr_p0);
    afs_analyze_stripe_nv16(x0, x1, buf_ptr, pw_mask_12stripe_0);

    //analyze shift
    x3 = _mm_loadu_si128((__m128i *)(ptr[0]));
    x4 = _mm_loadu_si128((__m128i *)(ptr[1]+step));
    afs_analyze_stripe_nv16(x3, x4, buf_ptr + ((COMPRESS_BUF) ? 32 : 64), pw_mask_12stripe_1);

    //gather flags
    x0 = _mm_or_si128(x0, _mm_unpacklo_epi8(_mm_setzero_si128(), x6));
    x1 = _mm_or_si128(x1, _mm_unpackhi_epi8(_mm_setzero_si128(), x6));
    x0 = _mm_or_si128(x0, x3);
    x1 = _mm_or_si128(x1, x4);
}

static void __forceinline __stdcall afs_analyze_12_nv16_simd_plus2(BYTE *dst, BYTE *p0, BYTE *p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    const int frame_size = step * h_max;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 4 * ((COMPRESS_BUF) ? 2 : 4);
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (height - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m128i x0, x1, x2, x4, x5, x6, x7;
    BYTE *buf_ptr, *buf2_ptr;
    int ih;

    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    __m128i tmpY0, tmpY1, tmpC0, tmpC1;
    BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;

    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;

    if (h_start == 0) {
        for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16, ptr_p0 += 16, ptr_p1 += 16) {
            //Y
            _mm_prefetch((char *)ptr_p0 + step, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + step, _MM_HINT_T0);
            x0 = _mm_loadu_si128((__m128i *)ptr_p0);
            x1 = _mm_loadu_si128((__m128i *)ptr_p1);
            tmpY0 = afs_analyze_motion(x0, x1, 0);

            //C
            _mm_prefetch((char *)ptr_p0 + frame_size + step, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + frame_size + step, _MM_HINT_T0);
            x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + frame_size));
            x1 = _mm_loadu_si128((__m128i *)(ptr_p1 + frame_size));
            tmpC0 = afs_analyze_motion(x0, x1, 1);

            tmpY1 = _mm_unpackhi_epi8(_mm_setzero_si128(), tmpY0);
            tmpY0 = _mm_unpacklo_epi8(_mm_setzero_si128(), tmpY0);
            tmpC1 = _mm_unpackhi_epi8(_mm_setzero_si128(), tmpC0);
            tmpC0 = _mm_unpacklo_epi8(_mm_setzero_si128(), tmpC0);

            afs_analyze_shrink_info_sub_nv16(tmpY0, tmpY1, tmpC0, tmpC1, x0, x1);
            _mm_store_si128((__m128i*)(buf2_ptr), x0);
        }
    } else {
        p0 += step * (h_start - 1);
        p1 += step * (h_start - 1);
        dst += si_pitch * h_start;
    }

    __m64 m0 = _mm_setzero_si64();

    int h_loop_fin = std::min(height, h_fin + 4);
    for (ih = h_start + ((h_start == 0) ? 1 : 0); ih < h_loop_fin; ih++, p0 += step, p1 += step) {
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += ((COMPRESS_BUF) ? 128 : 256)) {
            //Y
            afs_analyze_count_stripe_nv16(tmpY0, tmpY1, ptr_p0,              ptr_p1,              buf_ptr +   0, 0, step, tb_order, ih);
            //C
            afs_analyze_count_stripe_nv16(tmpC0, tmpC1, ptr_p0 + frame_size, ptr_p1 + frame_size, buf_ptr + ((COMPRESS_BUF) ? 64 : 128), 1, step, tb_order, ih);

            afs_analyze_shrink_info_sub_nv16(tmpY0, tmpY1, tmpC0, tmpC1, x0, x1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), x0);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), x1);
        }

        if (ih >= h_start + 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
                x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                x1 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                x2 = x6;
                x2 = _mm_or_si128(x2, x5);
                x2 = _mm_or_si128(x2, x4);
                x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
                x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pb_mask_1stripe_01));
                x2 = _mm_or_si128(x2, x1);
                x2 = _mm_or_si128(x2, x7);
                _mm_storeu_si128((__m128i*)ptr_dst, x2);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3, 1, 3, 0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    h_loop_fin = std::min(height + 4, h_fin + 4);
    for (; ih < h_loop_fin; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, ptr_dst += 16, buf2_ptr += 16) {
            x7 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            x6 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            x5 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            x4 = _mm_load_si128((__m128i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            x2 = x6;
            x2 = _mm_or_si128(x2, x5);
            x2 = _mm_or_si128(x2, x4);
            x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pb_mask_12stripe_01));
            x2 = _mm_or_si128(x2, x7);
            _mm_storeu_si128((__m128i*)ptr_dst, x2);
            _mm_store_si128((__m128i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm_setzero_si128());
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(x2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3, 1, 3, 0)));
        }
        dst += si_pitch;
    }
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}
