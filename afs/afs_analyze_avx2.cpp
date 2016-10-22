#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include <Windows.h>
#include "afs.h"
#include "afs_convert_const.h"
#include "filter.h"
#include "simd_util.h"

#if _MSC_VER >= 1800 && !defined(__AVX__) && !defined(_DEBUG)
static_assert(false, "do not forget to set /arch:AVX or /arch:AVX2 for this file.");
#endif

static const _declspec(align(32)) BYTE pb_thre_count[32]       = { 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03 };
static const _declspec(align(32)) BYTE pw_thre_count2[32]      = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(32)) BYTE pw_thre_count1[32]      = { 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_2stripe_0[32]   = { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_2stripe_1[32]   = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_1stripe_0[32]   = { 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_1stripe_1[32]   = { 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_12stripe_0[32]  = { 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_12stripe_1[32]  = { 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00, 0x22, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_2motion_0[32]   = { 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04 };
static const _declspec(align(32)) BYTE pw_mask_1motion_0[32]   = { 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40 };

static const _declspec(align(32)) BYTE pb_mask_1stripe_01[32]  = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };
static const _declspec(align(32)) BYTE pb_mask_12stripe_01[32] = { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 };
static const _declspec(align(32)) BYTE pb_mask_12motion_01[32] = { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc };
static const _declspec(align(32)) BYTE pb_mask_12motion_stripe_01[32] = { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
static const _declspec(align(32)) BYTE pw_mask_12stripe_01[32] = { 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_12motion_01[32] = { 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_lowbyte[32]     = { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00 };
static const _declspec(align(32)) BYTE pw_mask_highbyte[32]    = { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff };
static const _declspec(align(32)) BYTE pw_mask_lowshort[32]    = { 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };

static const _declspec(align(32)) BYTE pb_mask_2motion_0[32]   = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
static const _declspec(align(32)) BYTE pb_mask_1motion_0[32]   = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

static _declspec(align(32)) USHORT pw_thre_shift[16]       = { 0 };
static _declspec(align(32)) USHORT pw_thre_deint[16]       = { 0 };
static _declspec(align(32)) USHORT pw_thre_motion[3][16]   = { 0 };

static _declspec(align(32)) USHORT pb_thre_shift[16]       = { 0 };
static _declspec(align(32)) USHORT pb_thre_deint[16]       = { 0 };
static _declspec(align(32)) USHORT pb_thre_motion[3][16]   = { 0 };

static __forceinline int count_motion(__m256i y0, BYTE mc_mask[BLOCK_SIZE_YCP], int x, int y, int y_limit, int top) {
    DWORD heightMask = 0 - ((DWORD)(y - top) < (DWORD)y_limit);
    
    static const _declspec(align(32)) BYTE MOTION_COUNT_CHECK[32] = {
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
        0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    };
    
    const __m256i yMotion = _mm256_load_si256((__m256i *)MOTION_COUNT_CHECK);
    y0 = _mm256_andnot_si256(y0, yMotion);
    y0 = _mm256_cmpeq_epi8(y0, yMotion);
    y0 = _mm256_and_si256(y0, _mm256_load_si256((__m256i *)(mc_mask + x)));
    return _mm_popcnt_u32(heightMask & _mm256_movemask_epi8(y0));
}

void __stdcall afs_analyze_set_threshold_avx2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    __m256i y0, y1;
    y0 = _mm256_set1_epi16((short)thre_shift);
    _mm256_store_si256((__m256i *)pw_thre_shift, y0);
    
    y1 = _mm256_set1_epi16((short)thre_deint);
    _mm256_store_si256((__m256i *)pw_thre_deint, y1);
    
    _mm256_zeroupper();
    
    pw_thre_motion[0][ 0] = thre_Ymotion;
    pw_thre_motion[0][ 1] = thre_Cmotion;
    pw_thre_motion[0][ 2] = thre_Cmotion;
    pw_thre_motion[0][ 3] = thre_Ymotion;
    pw_thre_motion[0][ 4] = thre_Cmotion;
    pw_thre_motion[0][ 5] = thre_Cmotion;
    pw_thre_motion[0][ 6] = thre_Ymotion;
    pw_thre_motion[0][ 7] = thre_Cmotion;
    
    pw_thre_motion[0][ 8] = thre_Cmotion;
    pw_thre_motion[0][ 9] = thre_Ymotion;
    pw_thre_motion[0][10] = thre_Cmotion;
    pw_thre_motion[0][11] = thre_Cmotion;
    pw_thre_motion[0][12] = thre_Ymotion;
    pw_thre_motion[0][13] = thre_Cmotion;
    pw_thre_motion[0][14] = thre_Cmotion;
    pw_thre_motion[0][15] = thre_Ymotion;
    
    pw_thre_motion[1][ 0] = thre_Cmotion;
    pw_thre_motion[1][ 1] = thre_Cmotion;
    pw_thre_motion[1][ 2] = thre_Ymotion;
    pw_thre_motion[1][ 3] = thre_Cmotion;
    pw_thre_motion[1][ 4] = thre_Cmotion;
    pw_thre_motion[1][ 5] = thre_Ymotion;
    pw_thre_motion[1][ 6] = thre_Cmotion;
    pw_thre_motion[1][ 7] = thre_Cmotion;
    
    __m128i x0 = _mm_load_si128((__m128i*)&pw_thre_motion[0][0]);
    __m128i x1 = _mm_load_si128((__m128i*)&pw_thre_motion[0][8]);
    __m128i x2 = _mm_load_si128((__m128i*)&pw_thre_motion[1][0]);
    _mm_store_si128((__m128i*)&pw_thre_motion[1][8], x0);
    _mm_store_si128((__m128i*)&pw_thre_motion[2][0], x1);
    _mm_store_si128((__m128i*)&pw_thre_motion[2][8], x2);
}

void __stdcall afs_analyze_set_threshold_nv16_avx2(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    __m256i y0, y1;
    thre_shift   = CLAMP((thre_shift  *219 + 383)>>12, -128, 127);
    thre_deint   = CLAMP((thre_deint  *219 + 383)>>12, -128, 127);
    thre_Ymotion = CLAMP((thre_Ymotion*219 + 383)>>12, -128, 127);
    thre_Cmotion = CLAMP((thre_Cmotion*  7 +  66)>> 7, -128, 127);

    y0 = _mm256_set1_epi8((char)thre_shift);
    _mm256_stream_si256((__m256i *)pb_thre_shift, y0);

    y1 = _mm256_set1_epi8((char)thre_deint);
    _mm256_stream_si256((__m256i *)pb_thre_deint, y1);

    y0 = _mm256_set1_epi8((char)thre_Ymotion);
    _mm256_stream_si256((__m256i *)pb_thre_motion[0], y0);

    y1 = _mm256_set1_epi8((char)thre_Cmotion);
    _mm256_stream_si256((__m256i *)pb_thre_motion[1], y1);

    _mm256_zeroupper();
}

template <BOOL aligned>
static __m256i __forceinline afs_analyze_shrink_info_sub_avx2(BYTE *src, __m128i& xOut0, __m128i& xOut1) {
#define USE_LOAD256_AND_BLEND_EPI32 1 //256bit loadとblend_epi32を使用する
#define PREFER_BLEND_EPI32          0 //YC48の並べ替えにblend_epi32を使用する
#define UNORDERED_CLAC_OPTIMIZATION 1 //PREFER_BLEND_EPI32が0のときに、YUVはそろえるが、並べ替えはせずに計算し、pshufbの使用回数を減らす
    __m256i y0, y1, y2, y3, y5, y6, y7;
#if PREFER_BLEND_EPI32 || !UNORDERED_CLAC_OPTIMIZATION
    __m256i yShuffleArray = _mm256_load_si256((__m256i*)Array_SUFFLE_YCP_Y);
#endif
    const int MASK_INT = 0x40 + 0x08 + 0x01;
#if USE_LOAD256_AND_BLEND_EPI32
    __m256i yA0 = (aligned) ? _mm256_load_si256((__m256i *)(src +  0)) : _mm256_loadu_si256((__m256i *)(src +  0));
    __m256i yA1 = (aligned) ? _mm256_load_si256((__m256i *)(src + 32)) : _mm256_loadu_si256((__m256i *)(src + 32));
    __m256i yA2 = (aligned) ? _mm256_load_si256((__m256i *)(src + 64)) : _mm256_loadu_si256((__m256i *)(src + 64));

    // yA0 = 128, 0
    // yA1 = 384, 256
    // yA2 = 640, 512

    //_mm256_permute2x128_si256よりなるべく_mm256_blend_epi32を使う
    y1 = _mm256_blend_epi32(yA0, yA1, 0xf0);                    // 384, 0
    y2 = _mm256_permute2x128_si256(yA0, yA2, (0x02<<4) + 0x01); // 512, 128
    y3 = _mm256_blend_epi32(yA1, yA2, 0xf0);                    // 640, 256
#else
    y1 = _mm256_set_m128i(_mm_load_si128((__m128i*)(src + 48)), _mm_load_si128((__m128i*)(src +  0)));
    y2 = _mm256_set_m128i(_mm_load_si128((__m128i*)(src + 64)), _mm_load_si128((__m128i*)(src + 16)));
    y3 = _mm256_set_m128i(_mm_load_si128((__m128i*)(src + 80)), _mm_load_si128((__m128i*)(src + 32)));
#endif


#if PREFER_BLEND_EPI32
    y0 = _mm256_blend_epi16(y3, y1, MASK_INT);
    y0 = _mm256_blend_epi16(y0, y2, MASK_INT<<1);

    y7 = _mm256_blend_epi32(y1, y2, 0x22);
    y7 = _mm256_blend_epi32(y7, y3, 0x99);

    y1 = _mm256_alignr_epi8(y2, y1, 2);
    y2 = _mm256_bsrli_epi128(y2, 2);
    y3 = _mm256_bsrli_epi128(y3, 2);
    //y2 = _mm256_alignr_epi8(y3, y2, 2);
    //y3 = _mm256_alignr_epi8(y3, y3, 2);

    y6 = _mm256_blend_epi32(y1, y2, 0x44);
    y6 = _mm256_blend_epi32(y6, y3, 0x22);

    y6 = _mm256_shuffle_epi32(y6, _MM_SHUFFLE(1,2,3,0));
    y7 = _mm256_shuffle_epi32(y7, _MM_SHUFFLE(3,0,1,2));

    y0 = _mm256_shuffle_epi8(y0, yShuffleArray); //Y
    y5 = _mm256_or_si256(_mm256_and_si256(y6, *(__m256i*)pw_mask_lowshort), _mm256_slli_epi32(y7, 16));
    //y7 = _mm256_or_si256(_mm256_srli_epi32(y6, 16), _mm256_andnot_si256(*(__m256i*)pw_mask_lowshort, y7));
    //y5 = _mm256_blend_epi16(y6, _mm256_slli_epi32(y7, 16), 0x80+0x20+0x08+0x02);
    y7 = _mm256_blend_epi16(_mm256_srli_epi32(y6, 16), y7, 0x80+0x20+0x08+0x02);
#else
    y0 = _mm256_blend_epi16(y1, y3, 0x20+0x04);
    y6 = _mm256_blend_epi16(y1, y3, 0x40+0x08+0x01);
    y7 = _mm256_blend_epi16(y1, y3, 0x80+0x10+0x02);

    y0 = _mm256_blend_epi16(y0, y2, 0x80+0x10+0x02);
    y6 = _mm256_blend_epi16(y6, y2, 0x20+0x04);
    y7 = _mm256_blend_epi16(y7, y2, 0x40+0x08+0x01);
#if UNORDERED_CLAC_OPTIMIZATION
    y5 = _mm256_alignr_epi8(y6, y6, 2);
    y7 = _mm256_alignr_epi8(y7, y7, 4);
#else
    y0 = _mm256_shuffle_epi8(y0, yShuffleArray); //Y
    y5 = _mm256_shuffle_epi8(y6, _mm256_alignr_epi8(yShuffleArray, yShuffleArray, 6)); //Cb
    y7 = _mm256_shuffle_epi8(y7, _mm256_alignr_epi8(yShuffleArray, yShuffleArray, 12)); //Cr
#endif //UNORDERED_CLAC_OPTIMIZATION
#endif /PREFER_BLEND_EPI32

    y1 = _mm256_or_si256(y0, y5);
    y0 = _mm256_and_si256(y0, y5);
    y1 = _mm256_or_si256(y1, y7);
    y0 = _mm256_and_si256(y0, y7);
    
    y0 = _mm256_srai_epi16(y0, 8);
#if 0
    y1 = _mm256_slli_epi16(y1, 8);
    y1 = _mm256_srai_epi16(y1, 8);
#else
    y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_lowbyte));
#endif

#if 0
    y0 = _mm256_packs_epi16(y0, y0); //128bitx2のパックであることに注意する
    y1 = _mm256_packs_epi16(y1, y1);

    y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3,1,2,0));
    y1 = _mm256_permute4x64_epi64(y1, _MM_SHUFFLE(3,1,2,0));

    y0 = _mm256_and_si256(y0, _mm256_load_si256((__m256i*)pb_mask_12motion_01));
    y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i*)pb_mask_12stripe_01));

    y0 = _mm256_or_si256(y0, y1);
    y0 = _mm256_permute2x128_si256(y0, y1, (0x02<<4) + 0x00);
#else
    y0 = _mm256_packs_epi16(y0, y1);
#if UNORDERED_CLAC_OPTIMIZATION
    static const _declspec(align(32)) BYTE Array_SUFFLE_YCP_COMPRESSED[32] = {
        0, 3, 6, 1, 4, 7, 2, 5, 8, 11, 14, 9, 12, 15, 10, 13,
        0, 3, 6, 1, 4, 7, 2, 5, 8, 11, 14, 9, 12, 15, 10, 13,
    };
    y0 = _mm256_shuffle_epi8(y0, _mm256_load_si256((__m256i*)Array_SUFFLE_YCP_COMPRESSED));
#endif
    y0 = _mm256_and_si256(y0, _mm256_load_si256((__m256i*)pb_mask_12motion_stripe_01));
    y1 = _mm256_bsrli_epi128(y0, 8);
    y0 = _mm256_or_si256(y0, y1);
#if 0
    y0 = _mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3,1,2,0));

    xOut0 = _mm256_castsi128_si256(y0);
    xOut1 = _mm256_extractf128_si256(y0, 1);
#else
    xOut0 = _mm256_castsi256_si128(_mm256_permute4x64_epi64(y0, _MM_SHUFFLE(3,1,2,0)));
    xOut1 = _mm256_castsi256_si128(_mm256_permute4x64_epi64(y0, _MM_SHUFFLE(2,0,3,1)));
#endif
#endif
    return y0;
}

//各16pixel分の16bit幅のマスクデータを2つ受け取り、x0とx1に32pixel分の8bit幅の出力を返す
static void __forceinline afs_analyze_shrink_info_sub_nv16(const __m256i& yY0, const __m256i& yY1, const __m256i& yC0, const __m256i& yC1, __m256i &x0, __m256i &x1) {
    const __m256i yLowMask = _mm256_set1_epi32(0x0000ffff);
    __m256i y0, y1, y2, y3, y6, y7;
    y6 = _mm256_blend_epi16(yC0, _mm256_slli_epi32(yC0, 16), 0x40+0x10+0x04+0x01);
    y7 = _mm256_blend_epi16(yC0, _mm256_srli_epi32(yC0, 16), 0x80+0x20+0x08+0x02);
    y0 = yY0;

    y1 = _mm256_or_si256(x0, y6);
    y0 = _mm256_and_si256(x0, y6);
    y1 = _mm256_or_si256(x1, y7);
    y0 = _mm256_and_si256(x0, y7);
    y2 = _mm256_srai_epi16(x0, 8);
#if 0
    y3 = _mm256_slli_epi16(y1, 8);
    y3 = _mm256_srai_epi16(y3, 8);
#else
    y3 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_lowbyte));
#endif

    //
    y6 = _mm256_blend_epi16(yC1, _mm256_slli_epi32(yC1, 16), 0x40+0x10+0x04+0x01);
    y7 = _mm256_blend_epi16(yC1, _mm256_srli_epi32(yC1, 16), 0x80+0x20+0x08+0x02);
    y0 = yY1;

    y1 = _mm256_or_si256(x0, y6);
    y0 = _mm256_and_si256(x0, y6);
    y1 = _mm256_or_si256(x1, y7);
    y0 = _mm256_and_si256(x0, y7);
    y0 = _mm256_srai_epi16(x0, 8);
#if 0
    y1 = _mm256_slli_epi16(x1, 8);
    y1 = _mm256_srai_epi16(x1, 8);
#else
    y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_lowbyte));
#endif

    y0 = _mm256_packs_epi16(y2, y0);
    y1 = _mm256_packs_epi16(y3, y1);

    y0 = _mm256_and_si256(x0, _mm256_load_si256((__m256i*)pb_mask_12motion_01));
    y1 = _mm256_and_si256(x1, _mm256_load_si256((__m256i*)pb_mask_12stripe_01));

    y0 = _mm256_or_si256(x0, y1);
}

void __stdcall afs_analyze_12_avx2_plus2(BYTE *dst, void *_p0, void *_p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    PIXEL_YC *p0 = (PIXEL_YC *)_p0;
    PIXEL_YC *p1 = (PIXEL_YC *)_p1;
    const int step6 = step * 6;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (h - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m128i xShrinked0, xShrinked1;
    __m256i y0, y1, y2, y3, y4, y5, y6, y7;
    BYTE *buf_ptr, *buf2_ptr;
    //BYTE *ptr[2];
    int ih;
    
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p[2] = { (BYTE *)p0, (BYTE *)p1 };
    BYTE __declspec(align(32)) tmp16pix[96];
    BYTE __declspec(align(32)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;
    
    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    //for (int i = 0; i < BUFFER_SIZE; i++)
        //buffer[i] = 0x00;

    for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16) {
        for (int jw = 0; jw < 3; jw++, ptr_p[0] += 32, ptr_p[1] += 32) {
            y0 = _mm256_loadu_si256((__m256i *)ptr_p[0]);
            y0 = _mm256_sub_epi16(y0, _mm256_loadu_si256((__m256i *)ptr_p[1]));
            _mm_prefetch((char *)ptr_p[0] + step6, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p[1] + step6, _MM_HINT_T0);
            y0 = _mm256_abs_epi16(y0);
            y3 = _mm256_load_si256((__m256i *)pw_thre_motion[jw]);
            y2 = _mm256_load_si256((__m256i *)pw_thre_shift);
            y3 = _mm256_cmpgt_epi16(y3, y0);
            y2 = _mm256_cmpgt_epi16(y2, y0);
            y3 = _mm256_and_si256(y3, _mm256_load_si256((__m256i *)pw_mask_2motion_0));
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pw_mask_1motion_0));
            y3 = _mm256_or_si256(y3, y2);
            _mm256_store_si256((__m256i *)(tmp16pix + jw*32), y3);
        }
        y0 = afs_analyze_shrink_info_sub_avx2<TRUE>(tmp16pix, xShrinked0, xShrinked1);
        _mm_store_si128((__m128i*)(buf2_ptr), xShrinked0);
    }

    __m64 m0 = _mm_setzero_si64();
    
    for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
        ptr_p[0] = (BYTE *)p0;
        ptr_p[1] = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16) {
            for (int jw = 0; jw < 3; jw++, ptr_p[0] += 32, ptr_p[1] += 32, buf_ptr += 128) {
                //ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
                //ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
                y0 = _mm256_loadu_si256((__m256i *)(ptr_p[0] + step6));
                y1 = y0;
                y0 = _mm256_sub_epi16(y0, _mm256_loadu_si256((__m256i *)(ptr_p[1]+step6)));
                y0 = _mm256_abs_epi16(y0);
                y3 = _mm256_load_si256((__m256i *)pw_thre_motion[jw]);
                y2 = _mm256_load_si256((__m256i *)pw_thre_shift);
                y3 = _mm256_cmpgt_epi16(y3, y0);
                y2 = _mm256_cmpgt_epi16(y2, y0);
                y3 = _mm256_and_si256(y3, _mm256_load_si256((__m256i *)pw_mask_2motion_0));
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pw_mask_1motion_0));
                y3 = _mm256_or_si256(y3, y2);

                _mm_prefetch((char *)ptr_p[0] + (step6<<1), _MM_HINT_T0);
                _mm_prefetch((char *)ptr_p[1] + (step6<<1), _MM_HINT_T0);
            
                y2 = _mm256_loadu_si256((__m256i *)ptr_p[0]);
                y2 = _mm256_sub_epi16(y2, y1);
                y0 = _mm256_abs_epi16(y2);
                y2 = _mm256_cmpeq_epi16(y2, y0);
                y1 = y0;
                y1 = _mm256_cmpgt_epi16(y1, _mm256_load_si256((__m256i *)pw_thre_deint));
                y0 = _mm256_cmpgt_epi16(y0, _mm256_load_si256((__m256i *)pw_thre_shift));
#if 0
                y1 = _mm256_packs_epi16(y1, y1);
                y0 = _mm256_packs_epi16(y0, y0);
                y1 = _mm256_unpacklo_epi8(y1, y0);
#elif 0
                y0 = _mm256_slli_epi16(y0, 8);
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i*)pw_mask_lowbyte));
                y1 = _mm256_or_si256(y1, y0);
#else
                y1 = _mm256_blendv_epi8(y0, y1, _mm256_load_si256((__m256i*)pw_mask_lowbyte));
#endif
                //y7 = _mm256_load_si256((__m256i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
                //y6 = _mm256_load_si256((__m256i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
                y7 = _mm256_load_si256((__m256i *)(buf_ptr +  0));
                y6 = _mm256_load_si256((__m256i *)(buf_ptr + 32));
                y2 = _mm256_xor_si256(y2, y7);
                y7 = _mm256_xor_si256(y7, y2);
                y6 = _mm256_and_si256(y6, _mm256_and_si256(y2, y1));
                y6 = _mm256_subs_epi8(y6, y1);
                _mm256_store_si256((__m256i *)(buf_ptr +  0), y7);
                _mm256_store_si256((__m256i *)(buf_ptr + 32), y6);

                y0 = y6;
                y0 = _mm256_cmpgt_epi8(y0, _mm256_load_si256((__m256i *)pb_thre_count));
                y0 = _mm256_srli_epi16(y0, 4);
                y0 = _mm256_and_si256(y0, _mm256_load_si256((__m256i *)pw_mask_12stripe_0));
                y3 = _mm256_or_si256(y3, y0);
            
                y2 = _mm256_loadu_si256((__m256i *)(ptr_p[tb_order != (ih & 1)]));
                y2 = _mm256_sub_epi16(y2, _mm256_loadu_si256((__m256i *)(ptr_p[tb_order == (ih & 1)]+step6)));
                y0 = _mm256_abs_epi16(y2);
                y2 = _mm256_cmpeq_epi16(y2, y0);
                y1 = y0;
                y1 = _mm256_cmpgt_epi16(y1, _mm256_load_si256((__m256i *)pw_thre_deint));
                y0 = _mm256_cmpgt_epi16(y0, _mm256_load_si256((__m256i *)pw_thre_shift));
#if 0
                y1 = _mm256_packs_epi16(y1, y1);
                y0 = _mm256_packs_epi16(y0, y0);
                y1 = _mm256_unpacklo_epi8(y1, y0);
#elif 0
                y0 = _mm256_slli_epi16(y0, 8);
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i*)pw_mask_lowbyte));
                y1 = _mm256_or_si256(y1, y0);
#else
                y1 = _mm256_blendv_epi8(y0, y1, _mm256_load_si256((__m256i*)pw_mask_lowbyte));
#endif
                y5 = _mm256_load_si256((__m256i *)(buf_ptr + 64));
                y4 = _mm256_load_si256((__m256i *)(buf_ptr + 96));
                y2 = _mm256_xor_si256(y2, y5);
                y5 = _mm256_xor_si256(y5, y2);
                y4 = _mm256_and_si256(y4, _mm256_and_si256(y2, y1));
                y4 = _mm256_subs_epi8(y4, y1);
                _mm256_store_si256((__m256i *)(buf_ptr + 64), y5);
                _mm256_store_si256((__m256i *)(buf_ptr + 96), y4);
                y0 = y4;
                y0 = _mm256_cmpgt_epi8(y0, _mm256_load_si256((__m256i *)pb_thre_count));
                y0 = _mm256_srli_epi16(y0, 4);
                y0 = _mm256_and_si256(y0, _mm256_load_si256((__m256i *)pw_mask_12stripe_1));
                y3 = _mm256_or_si256(y3, y0);
            
                _mm256_store_si256((__m256i *)(tmp16pix + jw*32), y3);
            }
            y0 = afs_analyze_shrink_info_sub_avx2<TRUE>(tmp16pix, xShrinked0, xShrinked1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), xShrinked1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), xShrinked0);
        }

        if (ih >= 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
                y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                y1 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                y2 = y6;
                y2 = _mm256_or_si256(y2, y5);
                y2 = _mm256_or_si256(y2, y4);
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pb_mask_1stripe_01));
                y2 = _mm256_or_si256(y2, y1);
                y2 = _mm256_or_si256(y2, y7);
                _mm256_storeu_si256((__m256i*)ptr_dst, y2);
                //const int is_latter_feild = is_latter_field(ih - 4, tb_order);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    for ( ; ih < h + 4; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
            y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            y2 = y6;
            y2 = _mm256_or_si256(y2, y5);
            y2 = _mm256_or_si256(y2, y4);
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
            y2 = _mm256_or_si256(y2, y7);
            _mm256_storeu_si256((__m256i*)ptr_dst, y2);
            _mm256_store_si256((__m256i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm256_setzero_si256());
            //const int is_latter_feild = is_latter_field(ih - 4, tb_order);
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
        }
        dst += si_pitch;
    }
    _mm256_zeroupper();
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}

void __stdcall afs_analyze_1_avx2_plus2(BYTE *dst, void *_p0, void *_p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    PIXEL_YC *p0 = (PIXEL_YC *)_p0;
    PIXEL_YC *p1 = (PIXEL_YC *)_p1;
    const int step6 = step * 6;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (h - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m128i xShrinked0, xShrinked1;
    __m256i y0, y1, y2, y3, y4, y5, y6, y7;
    BYTE *buf_ptr, *buf2_ptr;
    BYTE *ptr[2];
    int ih;
    
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    BYTE __declspec(align(32)) tmp16pix[96];
    BYTE __declspec(align(32)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;
    
    y3 = _mm256_load_si256((__m256i *)pw_thre_shift);
    
    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;

    for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16) {
        for (int jw = 0; jw < 3; jw++, ptr_p0 += 32, ptr_p1 += 32) {
            //afs_analyze_1_mmx_sub
            y0 = _mm256_loadu_si256((__m256i *)ptr_p0);
            y0 = _mm256_subs_epi16(y0, _mm256_loadu_si256((__m256i *)ptr_p1)); //y0 = *p0 - *p1
            _mm_prefetch((char *)ptr_p0 + step6, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + step6, _MM_HINT_T0);
            y0 = _mm256_abs_epi16(y0); //y0 = abs(*p0 - *p1)
            y2 = y3;
            y2 = _mm256_cmpgt_epi16(y2, y0); //y2 = (thre_motion > abs(*p0 - *p1))
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pw_mask_1motion_0)); //y2 &= 4000h
            
            _mm256_store_si256((__m256i *)(tmp16pix + jw*32), y2);
        }
        y0 = afs_analyze_shrink_info_sub_avx2<TRUE>(tmp16pix, xShrinked0, xShrinked1);
        _mm_store_si128((__m128i*)(buf2_ptr), xShrinked0);
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
        
    for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16) {
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 32, ptr_p1 += 32, buf_ptr += 32) {
                ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
                ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
                //afs_analyze_1_mmx_loop
                //former field line
                //analyze motion
                y0 = _mm256_loadu_si256((__m256i *)(ptr_p0+step6));
                y4 = y0;
                y0 = _mm256_subs_epi16(y0, _mm256_loadu_si256((__m256i *)(ptr_p1+step6))); //y0 = *p0 - *p1
                y0 = _mm256_abs_epi16(y0); //y0 = abs(*p0 - *p1)
                y2 = y3;
                y2 = _mm256_cmpgt_epi16(y2, y0); //y2 = (thre_shift > abs(*p0 - *p1))
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pw_mask_1motion_0)); //y2 &= 4000h

                _mm_prefetch((char *)ptr_p0 + (step6<<1), _MM_HINT_T0);
                _mm_prefetch((char *)ptr_p1 + (step6<<1), _MM_HINT_T0);
                
                //analyze non-shift
                y1 = _mm256_loadu_si256((__m256i *)ptr_p0);
                y1 = _mm256_subs_epi16(y1, y4);
                y0 = _mm256_abs_epi16(y1);
                y1 = _mm256_cmpeq_epi16(y1, y0);
                y0 = _mm256_cmpgt_epi16(y0, y3);
                y7 = _mm256_load_si256((__m256i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
                y6 = _mm256_load_si256((__m256i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
                y1 = _mm256_xor_si256(y1, y7);
                y7 = _mm256_xor_si256(y7, y1);
                y6 = _mm256_and_si256(y6, y1);
                y6 = _mm256_and_si256(y6, y0);
                y6 = _mm256_subs_epi16(y6, y0);
                _mm256_store_si256((__m256i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), y7);
                _mm256_store_si256((__m256i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), y6);
                
                y1 = y6;
                y1 = _mm256_cmpgt_epi16(y1, _mm256_load_si256((__m256i *)pw_thre_count1));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_1stripe_0));
                y2 = _mm256_or_si256(y2, y1);
                
                //analyze shift
                y1 = _mm256_loadu_si256((__m256i *)(ptr[0]));
                y1 = _mm256_subs_epi16(y1, _mm256_loadu_si256((__m256i *)(ptr[1]+step6)));
                y0 = _mm256_abs_epi16(y1);
                y1 = _mm256_cmpeq_epi16(y1, y0);
                y0 = _mm256_cmpgt_epi16(y0, y3);
                y5 = _mm256_load_si256((__m256i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
                y4 = _mm256_load_si256((__m256i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
                y1 = _mm256_xor_si256(y1, y5);
                y5 = _mm256_xor_si256(y5, y1);
                y4 = _mm256_and_si256(y4, y1);
                y4 = _mm256_and_si256(y4, y0);
                y4 = _mm256_subs_epi16(y4, y0);
                _mm256_store_si256((__m256i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), y5);
                _mm256_store_si256((__m256i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), y4);
                y1 = y4;
                y1 = _mm256_cmpgt_epi16(y1, _mm256_load_si256((__m256i *)pw_thre_count1));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_1stripe_1));
                y2 = _mm256_or_si256(y2, y1);
            
                _mm256_store_si256((__m256i *)(tmp16pix + jw*32), y2);
            }
            y0 = afs_analyze_shrink_info_sub_avx2<TRUE>(tmp16pix, xShrinked0, xShrinked1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), xShrinked1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), xShrinked0);
        }

        if (ih >= 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
                y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                y1 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                y2 = y6;
                y2 = _mm256_or_si256(y2, y5);
                y2 = _mm256_or_si256(y2, y4);
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pb_mask_1stripe_01));
                y2 = _mm256_or_si256(y2, y1);
                y2 = _mm256_or_si256(y2, y7);
                _mm256_storeu_si256((__m256i*)ptr_dst, y2);
                //const int is_latter_feild = is_latter_field(ih - 4, tb_order);
                const int is_latter_feild = is_latter_field(ih, tb_order);
                int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    for ( ; ih < h + 4; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
            y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            y2 = y6;
            y2 = _mm256_or_si256(y2, y5);
            y2 = _mm256_or_si256(y2, y4);
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
            y2 = _mm256_or_si256(y2, y7);
            _mm256_storeu_si256((__m256i*)ptr_dst, y2);
            _mm256_store_si256((__m256i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm256_setzero_si256());
            //const int is_latter_feild = is_latter_field(ih - 4, tb_order);
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
        }
        dst += si_pitch;
    }

    _mm256_zeroupper();
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}

void __stdcall afs_analyze_2_avx2_plus2(BYTE *dst, void *_p0, void *_p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    PIXEL_YC *p0 = (PIXEL_YC *)_p0;
    PIXEL_YC *p1 = (PIXEL_YC *)_p1;
    const int step6 = step * 6;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (h - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m128i xShrinked0, xShrinked1;
    __m256i y0, y1, y2, y3, y4, y5, y6, y7;
    BYTE *buf_ptr, *buf2_ptr;
    BYTE *ptr[2];
    int ih;
    
    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    BYTE __declspec(align(32)) tmp16pix[96];
    BYTE __declspec(align(32)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;
    
    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;

    for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16) {
        for (int jw = 0; jw < 3; jw++, ptr_p0 += 32, ptr_p1 += 32) {
            y3 = _mm256_load_si256((__m256i *)(pw_thre_motion[jw]));
            //afs_analyze_2_mmx_sub
            y0 = _mm256_loadu_si256((__m256i *)ptr_p0);
            y0 = _mm256_subs_epi16(y0, _mm256_loadu_si256((__m256i *)ptr_p1)); //y0 = *p0 - *p1
            _mm_prefetch((char *)ptr_p0 + step6, _MM_HINT_T0);
            _mm_prefetch((char *)ptr_p1 + step6, _MM_HINT_T0);
            y0 = _mm256_abs_epi16(y0); //y0 = abs(*p0 - *p1)
            y2 = y3;
            y2 = _mm256_cmpgt_epi16(y2, y0); //y2 = (thre_motion > abs(*p0 - *p1))
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pw_mask_2motion_0)); //y2 &= 4000h
            
            _mm256_store_si256((__m256i *)(tmp16pix + jw*32), y2);
        }
        y0 = afs_analyze_shrink_info_sub_avx2<TRUE>(tmp16pix, xShrinked0, xShrinked1);
        _mm_store_si128((__m128i*)(buf2_ptr), xShrinked0);
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
        
    for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 16, buf2_ptr += 16) {
            for (int jw = 0; jw < 3; jw++, ptr_p0 += 32, ptr_p1 += 32, buf_ptr += 32) {
                y3 = _mm256_load_si256((__m256i *)(pw_thre_motion[jw]));
                ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
                ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
                //afs_analyze_1_mmx_loop
                //former field line
                //analyze motion
                y0 = _mm256_loadu_si256((__m256i *)(ptr_p0+step6));
                y4 = y0;
                y0 = _mm256_subs_epi16(y0, _mm256_loadu_si256((__m256i *)(ptr_p1+step6))); //y0 = *p0 - *p1
                y0 = _mm256_abs_epi16(y0); //y0 = abs(*p0 - *p1)
                y2 = y3;
                y2 = _mm256_cmpgt_epi16(y2, y0); //y2 = (thre_shift > abs(*p0 - *p1))
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pw_mask_2motion_0)); //y2 &= 4000h

                _mm_prefetch((char *)ptr_p0 + (step6<<1), _MM_HINT_T0);
                _mm_prefetch((char *)ptr_p1 + (step6<<1), _MM_HINT_T0);
                
                //analyze non-shift
                y1 = _mm256_loadu_si256((__m256i *)ptr_p0);
                y1 = _mm256_subs_epi16(y1, y4);
                y0 = _mm256_abs_epi16(y1);
                y1 = _mm256_cmpeq_epi16(y1, y0);
                y0 = _mm256_cmpgt_epi16(y0, _mm256_load_si256((__m256i *)pw_thre_deint));
                y7 = _mm256_load_si256((__m256i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
                y6 = _mm256_load_si256((__m256i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
                y1 = _mm256_xor_si256(y1, y7);
                y7 = _mm256_xor_si256(y7, y1);
                y6 = _mm256_and_si256(y6, y1);
                y6 = _mm256_and_si256(y6, y0);
                y6 = _mm256_subs_epi16(y6, y0);
                _mm256_store_si256((__m256i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), y7);
                _mm256_store_si256((__m256i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), y6);
                
                y1 = y6;
                y1 = _mm256_cmpgt_epi16(y1, _mm256_load_si256((__m256i *)pw_thre_count2));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_2stripe_0));
                y2 = _mm256_or_si256(y2, y1);

                //analyze shift
                y1 = _mm256_loadu_si256((__m256i *)(ptr[0]));
                y1 = _mm256_subs_epi16(y1, _mm256_loadu_si256((__m256i *)(ptr[1]+step6)));
                y0 = _mm256_abs_epi16(y1);
                y1 = _mm256_cmpeq_epi16(y1, y0);
                y0 = _mm256_cmpgt_epi16(y0, _mm256_load_si256((__m256i *)pw_thre_deint));
                y5 = _mm256_load_si256((__m256i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
                y4 = _mm256_load_si256((__m256i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
                y1 = _mm256_xor_si256(y1, y5);
                y5 = _mm256_xor_si256(y5, y1);
                y4 = _mm256_and_si256(y4, y1);
                y4 = _mm256_and_si256(y4, y0);
                y4 = _mm256_subs_epi16(y4, y0);
                _mm256_store_si256((__m256i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), y5);
                _mm256_store_si256((__m256i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), y4);
                y1 = y4;
                y1 = _mm256_cmpgt_epi16(y1, _mm256_load_si256((__m256i *)pw_thre_count2));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pw_mask_2stripe_1));
                y2 = _mm256_or_si256(y2, y1);
            
                _mm256_store_si256((__m256i *)(tmp16pix + jw*32), y2);
            }
            y0 = afs_analyze_shrink_info_sub_avx2<TRUE>(tmp16pix, xShrinked0, xShrinked1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), xShrinked1);
            _mm_store_si128((__m128i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), xShrinked0);
        }

        if (ih >= 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
                y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                y1 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                y2 = y6;
                y2 = _mm256_or_si256(y2, y5);
                y2 = _mm256_or_si256(y2, y4);
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pb_mask_1stripe_01));
                y2 = _mm256_or_si256(y2, y1);
                y2 = _mm256_or_si256(y2, y7);
                _mm256_storeu_si256((__m256i*)ptr_dst, y2);
                //const int is_latter_feild = is_latter_field(ih - 4, tb_order);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    for ( ; ih < h + 4; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
            y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            y2 = y6;
            y2 = _mm256_or_si256(y2, y5);
            y2 = _mm256_or_si256(y2, y4);
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
            y2 = _mm256_or_si256(y2, y7);
            _mm256_storeu_si256((__m256i*)ptr_dst, y2);
            _mm256_store_si256((__m256i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm256_setzero_si256());
            //const int is_latter_feild = is_latter_field(ih - 4, tb_order);
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3,1,3,0)));
        }
        dst += si_pitch;
    }
    _mm256_zeroupper();
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
}

static __m256i __forceinline afs_subs_epi8(__m256i y0, __m256i y1) {
    __m256i y2, y3;
    y2 = _mm256_unpackhi_epi8(y0, _mm256_setzero_si256());
    y0 = _mm256_unpacklo_epi8(y0, _mm256_setzero_si256());
    y3 = _mm256_unpackhi_epi8(y1, _mm256_setzero_si256());
    y1 = _mm256_unpacklo_epi8(y1, _mm256_setzero_si256());
    y0 = _mm256_sub_epi16(y0, y1);
    y2 = _mm256_sub_epi16(y2, y3);
    return _mm256_packs_epi16(y0, y2);
}

static __m256i __forceinline afs_abs_epi8(__m256i y0) {
    return _mm256_abs_epi8(y0);
}

//出力 16pixel分の8bitマスク
static __m256i __forceinline afs_analyze_motion(__m256i y0, __m256i y1, int i_thre_motion) {
    __m256i y2, y3;
    y0 = afs_subs_epi8(y0, y1);
    y0 = _mm256_abs_epi8(y0);

    y3 = _mm256_load_si256((__m256i *)pb_thre_motion[i_thre_motion]);
    y2 = _mm256_load_si256((__m256i *)pb_thre_shift);
    y3 = _mm256_cmpgt_epi8(y3, y0);
    y2 = _mm256_cmpgt_epi8(y2, y0);
    y3 = _mm256_and_si256(y3, _mm256_load_si256((__m256i *)pb_mask_2motion_0));
    y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_1motion_0));
    y3 = _mm256_or_si256(y3, y2);
    return y3;
}

static __m256i __forceinline afs_analyze_element_stripe_nv16(__m256i y0, __m256i y2, const BYTE *buf_ptr, const BYTE *pw_mask_12stripe) {
    __m256i y6, y7;
    y7 = _mm256_load_si256((__m256i *)(buf_ptr +  0));
    y6 = _mm256_load_si256((__m256i *)(buf_ptr + 32));
    y2 = _mm256_xor_si256(y2, y7);
    y7 = _mm256_xor_si256(y7, y2);
    y6 = _mm256_and_si256(y6, y2);
    y6 = _mm256_and_si256(y6, y0);
    y6 = _mm256_subs_epi8(y6, y0);
    _mm256_store_si256((__m256i *)(buf_ptr +  0), y7);
    _mm256_store_si256((__m256i *)(buf_ptr + 32), y6);

    y0 = y6;
    y0 = _mm256_cmpgt_epi8(y0, _mm256_load_si256((__m256i *)pb_thre_count));
    y0 = _mm256_srli_epi16(y0, 4);
    y0 = _mm256_and_si256(y0, _mm256_load_si256((__m256i *)pw_mask_12stripe));
    return y0;
}

//出力 y0, y1 ... 16bit幅で8pixelずつ(計16pixel)のマスクデータ
static void __forceinline afs_analyze_stripe_nv16(__m256i& y0, __m256i& y1, const BYTE *buf_ptr) {
    __m256i y2, y4, y5;
    y2 = afs_subs_epi8(y0, y1);

    y0 = _mm256_abs_epi8(y2);
    y2 = _mm256_cmpeq_epi8(y2, y0);

    y1 = y0;
    y1 = _mm256_cmpgt_epi8(y1, _mm256_load_si256((__m256i *)pb_thre_deint));
    y0 = _mm256_cmpgt_epi8(y0, _mm256_load_si256((__m256i *)pb_thre_shift));
    y4 = _mm256_unpacklo_epi8(y1, y0);
    y5 = _mm256_unpackhi_epi8(y1, y0);
    //
    y0 = afs_analyze_element_stripe_nv16(y4, y2, buf_ptr +  0, pw_mask_12stripe_0);
    y1 = afs_analyze_element_stripe_nv16(y5, y2, buf_ptr + 64, pw_mask_12stripe_1);
}

//出力 y0, y1 ... 16bit幅で8pixelずつ(計16pixel)のマスクデータ
static void __forceinline afs_analyze_count_stripe_nv16(__m256i& y0, __m256i& y1, BYTE *ptr_p0, BYTE *ptr_p1, const BYTE *buf_ptr, int i_thre_motion, int step, int tb_order, int ih) {
    BYTE *ptr[2];
    ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
    ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
    _mm_prefetch((char *)ptr_p0 + (step << 1), _MM_HINT_T0);
    _mm_prefetch((char *)ptr_p1 + (step << 1), _MM_HINT_T0);

    __m256i y3, y4, y6;

    //analyze motion
    y0 = _mm256_loadu_si256((__m256i *)(ptr_p0 + step));
    y1 = _mm256_loadu_si256((__m256i *)(ptr_p1 + step));
    y6 = afs_analyze_motion(y0, y1, i_thre_motion);

    //analyze non-shift
    y0 = _mm256_loadu_si256((__m256i *)ptr_p0);
    afs_analyze_stripe_nv16(y0, y1, buf_ptr);

    //analyze shift
    y3 = _mm256_loadu_si256((__m256i *)(ptr[0]));
    y4 = _mm256_loadu_si256((__m256i *)(ptr[1]+step));
    afs_analyze_stripe_nv16(y3, y4, buf_ptr + 2 * BLOCK_SIZE_YCP * 4);

    //gather flags
    y0 = _mm256_or_si256(y0, _mm256_unpacklo_epi8(_mm256_setzero_si256(), y6));
    y1 = _mm256_or_si256(y1, _mm256_unpackhi_epi8(_mm256_setzero_si256(), y6));
    y0 = _mm256_or_si256(y0, y3);
    y1 = _mm256_or_si256(y1, y4);
}

void __stdcall afs_analyze_nv16_12_simd_plus2(BYTE *dst, BYTE *p0, BYTE *p1, int tb_order, int width, int step, int si_pitch, int h, int max_h, int *motion_count, AFS_SCAN_CLIP *mc_clip) {
    const int frame_size = step * max_h;
    const int BUFFER_SIZE = BLOCK_SIZE_YCP * 4 * 4;
    const int scan_t = mc_clip->top;
    const int mc_scan_y_limit = (h - mc_clip->bottom - scan_t) & ~1;
    BYTE __declspec(align(32)) mc_mask[BLOCK_SIZE_YCP];
    __m256i y0, y1, y2, y4, y5, y6, y7;
    BYTE *buf_ptr, *buf2_ptr;
    int ih;

    BYTE *ptr_dst = (BYTE *)dst;
    BYTE *ptr_p0 = (BYTE *)p0;
    BYTE *ptr_p1 = (BYTE *)p1;
    __m256i tmpY0, tmpY1, tmpC0, tmpC1;
    BYTE __declspec(align(16)) buffer[BUFFER_SIZE + BLOCK_SIZE_YCP * 8];
    buf_ptr = buffer;
    buf2_ptr = buffer + BUFFER_SIZE;

    __stosb(mc_mask, 0, 256);
    const int mc_scan_x_limit = width - mc_clip->right;
    for (int i = mc_clip->left; i < mc_scan_x_limit; i++)
        mc_mask[i] = 0xff;
    for (int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = 0x00;

    for (int kw = 0; kw < width; kw += 32, buf2_ptr += 32) {
        //Y
        _mm_prefetch((char *)ptr_p0 + step, _MM_HINT_T0);
        _mm_prefetch((char *)ptr_p1 + step, _MM_HINT_T0);
        y0 = _mm256_loadu_si256((__m256i *)ptr_p0);
        y1 = _mm256_loadu_si256((__m256i *)ptr_p1);
        tmpY0 = afs_analyze_motion(y0, y1, 0);

        //C
        _mm_prefetch((char *)ptr_p0 + frame_size + step, _MM_HINT_T0);
        _mm_prefetch((char *)ptr_p1 + frame_size + step, _MM_HINT_T0);
        y0 = _mm256_loadu_si256((__m256i *)(ptr_p0 + frame_size));
        y1 = _mm256_loadu_si256((__m256i *)(ptr_p1 + frame_size));
        tmpC0 = afs_analyze_motion(y0, y1, 0);

        tmpY1 = _mm256_unpackhi_epi8(_mm256_setzero_si256(), tmpY0);
        tmpY0 = _mm256_unpacklo_epi8(_mm256_setzero_si256(), tmpY0);
        tmpC1 = _mm256_unpackhi_epi8(_mm256_setzero_si256(), tmpC0);
        tmpC0 = _mm256_unpacklo_epi8(_mm256_setzero_si256(), tmpC0);

        afs_analyze_shrink_info_sub_nv16(tmpY0, tmpY1, tmpC0, tmpC1, y0, y1);
        _mm256_store_si256((__m256i*)(buf2_ptr), y0);
    }

    __m64 m0 = _mm_setzero_si64();

    for (ih = 1; ih < h; ih++, p0 += step, p1 += step) {
        ptr_p0 = (BYTE *)p0;
        ptr_p1 = (BYTE *)p1;
        buf_ptr = buffer;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 32, buf2_ptr += 32, ptr_p0 += 32, ptr_p1 += 32, buf_ptr += 512) {
            //Y
            afs_analyze_count_stripe_nv16(tmpY0, tmpY1, ptr_p0,              ptr_p1,              buf_ptr +   0, 0, step, tb_order, ih);
            //C
            afs_analyze_count_stripe_nv16(tmpC0, tmpC1, ptr_p0 + frame_size, ptr_p1 + frame_size, buf_ptr + 256, 1, step, tb_order, ih);

            afs_analyze_shrink_info_sub_nv16(tmpY0, tmpY1, tmpC0, tmpC1, y0, y1);
            _mm256_store_si256((__m256i*)(buf2_ptr + (((ih+0) & 7)) * BLOCK_SIZE_YCP), y0);
            _mm256_store_si256((__m256i*)(buf2_ptr + (((ih+1) & 7)) * BLOCK_SIZE_YCP), y1);
        }

        if (ih >= 4) {
            buf2_ptr = buffer + BUFFER_SIZE;
            ptr_dst = (BYTE *)dst;
            for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
                y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
                y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
                y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
                y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
                y1 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih+1)&7) * BLOCK_SIZE_YCP));
                y2 = y6;
                y2 = _mm256_or_si256(y2, y5);
                y2 = _mm256_or_si256(y2, y4);
                y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
                y1 = _mm256_and_si256(y1, _mm256_load_si256((__m256i *)pb_mask_1stripe_01));
                y2 = _mm256_or_si256(y2, y1);
                y2 = _mm256_or_si256(y2, y7);
                _mm256_storeu_si256((__m256i*)ptr_dst, y2);
                const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
                int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
                __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
                m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3, 1, 3, 0)));
            }
            dst += si_pitch;
        }
    }
    //残りの4ライン
    for (; ih < h + 4; ih++) {
        ptr_dst = (BYTE *)dst;
        buf2_ptr = buffer + BUFFER_SIZE;
        for (int kw = 0; kw < width; kw += 32, ptr_dst += 32, buf2_ptr += 32) {
            y7 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-4)&7) * BLOCK_SIZE_YCP));
            y6 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-3)&7) * BLOCK_SIZE_YCP));
            y5 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-2)&7) * BLOCK_SIZE_YCP));
            y4 = _mm256_load_si256((__m256i*)(buf2_ptr + ((ih-1)&7) * BLOCK_SIZE_YCP));
            y2 = y6;
            y2 = _mm256_or_si256(y2, y5);
            y2 = _mm256_or_si256(y2, y4);
            y2 = _mm256_and_si256(y2, _mm256_load_si256((__m256i *)pb_mask_12stripe_01));
            y2 = _mm256_or_si256(y2, y7);
            _mm256_storeu_si256((__m256i*)ptr_dst, y2);
            _mm256_store_si256((__m256i*)(buf2_ptr + ((ih+0)&7) * BLOCK_SIZE_YCP), _mm256_setzero_si256());
            const int is_latter_feild = is_latter_field(ih, tb_order); //ih-4でもihでも答えは同じ
            int count = count_motion(y2, mc_mask, kw, ih-4, mc_scan_y_limit, scan_t);
            __m64 m1 = _m_from_int(count << (is_latter_feild * 16));
            m0 = _mm_add_pi32(m0, _mm_shuffle_pi16(m1, _MM_SHUFFLE(3, 1, 3, 0)));
        }
        dst += si_pitch;
    }
    __m64 m2 = *(__m64 *)motion_count;
    *(__m64 *)motion_count = _mm_add_pi32(m2, m0);
    _mm_empty();
    _mm256_zeroupper();
}
