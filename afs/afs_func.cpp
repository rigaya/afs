﻿#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <intrin.h>
#include "filter.h"
#include "afs_func.h"
#include "simd_util.h"

#include "afs.h"
#include "afs_mmx.h"
#include "afs_filter_mmx.h"
#include "afs_analyze_mmx.h"
#include "afs_yuy2up_mmx.h"

void __stdcall afs_get_stripe_count(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
    const int y_fin = scan_h - sp0->clip.bottom - ((scan_h - sp0->clip.top - sp0->clip.bottom) & 1);
    for (int pos_y = sp0->clip.top; pos_y < y_fin; pos_y++) {
        BYTE *sip = sp->map + pos_y * si_w + sp0->clip.left;
        if (is_latter_field(pos_y, sp0->tb_order)) {
            for (int pos_x = sp0->clip.left; pos_x < scan_w - sp0->clip.right; pos_x++) {
                if(!(*sip & 0x50)) count[0]++;
                sip++;
            }
        } else {
            for (int pos_x = sp0->clip.left; pos_x < scan_w - sp0->clip.right; pos_x++) {
                if(!(*sip & 0x60)) count[1]++;
                sip++;
            }
        }
    }
}

void __stdcall afs_get_motion_count(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
    const int y_fin = scan_h - sp->clip.bottom - ((scan_h - sp->clip.top - sp->clip.bottom) & 1);
    for (int pos_y = sp->clip.top; pos_y < y_fin; pos_y++) {
        BYTE *sip = sp->map + pos_y * si_w + sp->clip.left;
        if (is_latter_field(pos_y, sp->tb_order)) {
            for (int pos_x = sp->clip.left; pos_x < scan_w - sp->clip.right; pos_x++) {
                motion_count[1] += ~*sip & 0x40;
                sip++;
            }
        } else {
            for (int pos_x = sp->clip.left; pos_x < scan_w - sp->clip.right; pos_x++) {
                motion_count[0] += ~*sip & 0x40;
                sip++;
            }
        }
    }
    motion_count[0] >>= 6;
    motion_count[1] >>= 6;
}

static DWORD get_availableSIMD() {
    int CPUInfo[4];
    __cpuid(CPUInfo, 1);
    DWORD simd = NONE;
    if (CPUInfo[3] & 0x04000000) simd |= SSE2;
    if (CPUInfo[2] & 0x00000001) simd |= SSE3;
    if (CPUInfo[2] & 0x00000200) simd |= SSSE3;
    if (CPUInfo[2] & 0x00080000) simd |= SSE41;
    if (CPUInfo[2] & 0x00100000) simd |= SSE42;
    if (CPUInfo[2] & 0x00800000) simd |= POPCNT;
#if (_MSC_VER >= 1600)
    UINT64 xgetbv = 0;
    if ((CPUInfo[2] & 0x18000000) == 0x18000000) {
        xgetbv = _xgetbv(0);
        if ((xgetbv & 0x06) == 0x06)
            simd |= AVX;
    }
#endif
#if (_MSC_VER >= 1700)
    __cpuid(CPUInfo, 7);
    if ((simd & AVX) && (CPUInfo[1] & 0x00000020))
        simd |= AVX2;
    if ((simd & AVX) && ((xgetbv >> 5) & 7) == 7) {
        if (CPUInfo[1] & (1u << 16)) simd |= AVX512F;
        if (simd & AVX512F) {
            if (CPUInfo[1] & (1u << 17)) simd |= AVX512DQ;
            if (CPUInfo[1] & (1u << 21)) simd |= AVX512IFMA;
            if (CPUInfo[1] & (1u << 26)) simd |= AVX512PF;
            if (CPUInfo[1] & (1u << 27)) simd |= AVX512ER;
            if (CPUInfo[1] & (1u << 28)) simd |= AVX512CD;
            if (CPUInfo[1] & (1u << 30)) simd |= AVX512BW;
            if (CPUInfo[1] & (1u << 31)) simd |= AVX512VL;
            if (CPUInfo[2] & (1u <<  1)) simd |= AVX512VBMI;
        }
    }
    if (simd & AVX2) {
        __cpuid(CPUInfo, 0);
        char vendor[16] = { 0 };
        memcpy(vendor + 0, &CPUInfo[1], sizeof(CPUInfo[1]));
        memcpy(vendor + 4, &CPUInfo[3], sizeof(CPUInfo[3]));
        memcpy(vendor + 8, &CPUInfo[2], sizeof(CPUInfo[2]));
        //if (strcmp(vendor, "GenuineIntel") == 0) {
        if (strcmp(vendor, "AuthenticAMD") != 0) {
            simd |= AVX2FAST;
        }
    }
#endif
    return simd;
}

#define analyze_avx2 

static const struct {
    DWORD simd;
    AFS_FUNC_ANALYZE analyze[2];
} FUNC_ANALYZE_LIST[] = {
    //set_thresholdで変数を渡しているため、これら5つの関数は必ずセットで
#if AFS_USE_XBYAK
    { AVX512F|AVX512BW|AVX2|AVX|POPCNT, {
        { 63, 64, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_avx512,    NULL, { NULL,                           NULL,                           NULL                     } },
        { 63, 64, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_avx2, NULL, { afs_analyze_12_nv16_avx2_plus2, afs_analyze_12_nv16_avx2_plus2, NULL                     } }
    } },
    { AVX2|AVX|POPCNT, {
        { 31, 32, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_avx2,      NULL, { NULL,                           afs_analyze_1_avx2_plus2,       afs_analyze_2_avx2_plus2 } },
        { 31, 32, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_avx2, NULL, { afs_analyze_12_nv16_avx2_plus2, afs_analyze_12_nv16_avx2_plus2, NULL                     } }
    } },
#endif
    { AVX2|AVX|POPCNT, {
        { 31, 32, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_avx2,      NULL, { afs_analyze_12_avx2_plus2,      afs_analyze_1_avx2_plus2,       afs_analyze_2_avx2_plus2 } },
        { 31, 32, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_avx2, NULL, { afs_analyze_12_nv16_avx2_plus2, afs_analyze_12_nv16_avx2_plus2, NULL                     } }
    } },
    { AVX|POPCNT|SSE41|SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_avx,      NULL, { afs_analyze_12_avx_plus2,      afs_analyze_1_avx_plus2,       afs_analyze_2_avx_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_avx, NULL, { afs_analyze_12_nv16_avx_plus2, afs_analyze_12_nv16_avx_plus2, NULL                    } },
    } },
    { POPCNT|SSE41|SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_sse4_1_popcnt,      NULL, { afs_analyze_12_sse4_1_popcnt_plus2,      afs_analyze_1_sse4_1_popcnt_plus2,       afs_analyze_2_sse4_1_popcnt_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_sse4_1_popcnt, NULL, { afs_analyze_12_nv16_sse4_1_popcnt_plus2, afs_analyze_12_nv16_sse4_1_popcnt_plus2, NULL                              } }
    } },
    { SSE41|SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_sse4_1,      NULL, { afs_analyze_12_sse4_1_plus2,      afs_analyze_1_sse4_1_plus2,       afs_analyze_2_sse4_1_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_sse4_1, NULL, { afs_analyze_12_nv16_sse4_1_plus2, afs_analyze_12_nv16_sse4_1_plus2, NULL                       } }
    } },
    { SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_ssse3,      NULL, { afs_analyze_12_ssse3_plus2,      afs_analyze_1_ssse3_plus2,       afs_analyze_2_ssse3_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_ssse3, NULL, { afs_analyze_12_nv16_ssse3_plus2, afs_analyze_12_nv16_ssse3_plus2, NULL                      } },
    } },
    { SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_sse2,      NULL, { afs_analyze_12_sse2_plus2,      afs_analyze_1_sse2_plus2,       afs_analyze_2_sse2_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_sse2, NULL, { afs_analyze_12_nv16_sse2_plus2, afs_analyze_12_nv16_sse2_plus2, NULL                     } }
    } },
    { NONE, {
        {  7,  4,  4, FALSE, afs_analyze_set_threshold_mmx, afs_analyze_shrink_info_mmx_plus, { afs_analyze_12_mmx_plus2, afs_analyze_1_mmx_plus2, afs_analyze_2_mmx_plus2 } },
        {  7,  4,  4, FALSE, NULL, NULL, { NULL, NULL, NULL } }
    } },
};

static const struct {
    DWORD simd;
    func_blend blend[8];
} FUNC_BLEND_LIST[] = {
#if AFS_AVX512_INTRINSIC
    { AVX512BW|AVX2|AVX,    { afs_blend_avx512, afs_blend_avx512, afs_blend_nv16up_avx2,   afs_blend_nv16_avx2,   afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2 } },
#endif
    { AVX2|AVX,             { afs_blend_avx2,   afs_blend_avx2,   afs_blend_nv16up_avx2,   afs_blend_nv16_avx2,   afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2 } },
    { AVX|SSE41|SSSE3|SSE2, { afs_blend_avx,    afs_blend_avx,    afs_blend_nv16up_avx,    afs_blend_nv16_avx,    afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2 } },
    { SSE41|SSSE3|SSE2,     { afs_blend_sse4_1, afs_blend_sse4_1, afs_blend_nv16up_sse4_1, afs_blend_nv16_sse4_1, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2 } },
    { SSE2,                 { afs_blend_sse2,   afs_blend_sse2,   afs_blend_nv16up_ssse3,  afs_blend_nv16_ssse3,  afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2 } },
    { NONE,                 { afs_blend_mmx,    afs_blend_mmx,    afs_blend_nv16up_sse2,   afs_blend_nv16_sse2,   afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2, afs_blend_nv16_yuy2_sse2 } },
};

static const struct {
    DWORD simd;
    func_mie_spot mie_spot[8];
} FUNC_MIE_SPOT_LIST[] = {
#if AFS_AVX512_INTRINSIC
    { AVX512BW|AVX2|AVX, { afs_mie_spot_avx512, afs_mie_spot_avx512, afs_mie_spot_nv16_avx2,    afs_mie_spot_nv16_avx2,   afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2 } },
#endif
    { AVX|SSE2,          { afs_mie_spot_avx,    afs_mie_spot_avx,    afs_mie_spot_nv16_avx,     afs_mie_spot_nv16_avx,    afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2 } },
    { SSE41|SSSE3|SSE2,  { afs_mie_spot_sse2,   afs_mie_spot_sse2,   afs_mie_spot_nv16_sse4_1,  afs_mie_spot_nv16_sse4_1, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2 } },
    { SSSE3|SSE2,        { afs_mie_spot_sse2,   afs_mie_spot_sse2,   afs_mie_spot_nv16_ssse3,   afs_mie_spot_nv16_ssse3,  afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2 } },
    { SSE2,              { afs_mie_spot_sse2,   afs_mie_spot_sse2,   afs_mie_spot_nv16_sse2,    afs_mie_spot_nv16_sse2,   afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2 } },
    { NONE,              { afs_mie_spot_mmx,    afs_mie_spot_mmx,    afs_mie_spot_nv16_sse2,    afs_mie_spot_nv16_sse2,   afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2, afs_mie_spot_nv16_yuy2_sse2 } },
};

static const struct {
    DWORD simd;
    func_mie_inter mie_inter[8];
} FUNC_MIE_INTER_LIST[] = {
#if AFS_AVX512_INTRINSIC
    { AVX512BW|AVX2|AVX, { afs_mie_inter_avx512, afs_mie_inter_avx512, afs_mie_inter_nv16_avx2,    afs_mie_inter_nv16_avx2,   afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
#endif
    { AVX2|AVX,          { afs_mie_inter_avx2,   afs_mie_inter_avx2,   afs_mie_inter_nv16_avx2,    afs_mie_inter_nv16_avx2,   afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
    { AVX|SSE2,          { afs_mie_inter_avx,    afs_mie_inter_avx,    afs_mie_inter_nv16_avx,     afs_mie_inter_nv16_avx,    afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
    { SSE41|SSSE3|SSE2,  { afs_mie_inter_sse2,   afs_mie_inter_sse2,   afs_mie_inter_nv16_sse4_1,  afs_mie_inter_nv16_sse4_1, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
    { SSSE3|SSE2,        { afs_mie_inter_sse2,   afs_mie_inter_sse2,   afs_mie_inter_nv16_ssse3,   afs_mie_inter_nv16_ssse3,  afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
    { SSE2,              { afs_mie_inter_sse2,   afs_mie_inter_sse2,   afs_mie_inter_nv16_sse2,    afs_mie_inter_nv16_sse2,   afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
    { NONE,              { afs_mie_inter_mmx,    afs_mie_inter_mmx,    afs_mie_inter_nv16_sse2,    afs_mie_inter_nv16_sse2,   afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2, afs_mie_inter_nv16_yuy2_sse2 } },
};

static const struct {
    DWORD simd;
    func_deint4 deint4[8];
} FUNC_DEINT4_LIST[] = {
#if AFS_AVX512_INTRINSIC
    { AVX512BW|AVX2|AVX, { afs_deint4_avx512, afs_deint4_avx512, afs_deint4_nv16_avx2,    afs_deint4_nv16_avx2,   afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
#endif
    { AVX2|AVX,          { afs_deint4_avx2,   afs_deint4_avx2,   afs_deint4_nv16_avx2,    afs_deint4_nv16_avx2,   afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
    { AVX|SSE2,          { afs_deint4_avx,    afs_deint4_avx,    afs_deint4_nv16_avx,     afs_deint4_nv16_avx,    afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
    { SSE41|SSSE3|SSE2,  { afs_deint4_sse2,   afs_deint4_sse2,   afs_deint4_nv16_sse4_1,  afs_deint4_nv16_sse4_1, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
    { SSSE3|SSE2,        { afs_deint4_sse2,   afs_deint4_sse2,   afs_deint4_nv16_ssse3,   afs_deint4_nv16_ssse3,  afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
    { SSE2,              { afs_deint4_sse2,   afs_deint4_sse2,   afs_deint4_nv16_sse2,    afs_deint4_nv16_sse2,   afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
    { NONE,              { afs_deint4_mmx,    afs_deint4_mmx,    afs_deint4_nv16_sse2,    afs_deint4_nv16_sse2,   afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2, afs_deint4_nv16_yuy2_sse2 } },
};

static const struct {
    DWORD simd;
    func_copy_line copy_line[8];
} FUNC_COPY_LINE_LIST[] = {
#if AFS_AVX512_INTRINSIC
    { AVX512BW|AVX2|AVX,    { afs_copy_yc48_line_avx512, afs_copy_yc48_line_avx512, afs_convert_nv16_yc48_avx2,   afs_convert_nv16_yc48up_avx2,   afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2 } },
#endif
    { AVX2|AVX,             { afs_copy_yc48_line_avx2,   afs_copy_yc48_line_avx2,   afs_convert_nv16_yc48_avx2,   afs_convert_nv16_yc48up_avx2,   afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2 } },
    { AVX|SSE41|SSSE3|SSE2, { afs_copy_yc48_line_sse,    afs_copy_yc48_line_sse,    afs_convert_nv16_yc48_avx,    afs_convert_nv16_yc48up_avx,    afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2 } },
    { SSE41|SSSE3|SSE2,     { afs_copy_yc48_line_sse,    afs_copy_yc48_line_sse,    afs_convert_nv16_yc48_sse4_1, afs_convert_nv16_yc48up_sse4_1, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2 } },
    { SSSE3|SSE2,           { afs_copy_yc48_line_sse,    afs_copy_yc48_line_sse,    afs_convert_nv16_yc48_ssse3,  afs_convert_nv16_yc48up_ssse3,  afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2 } },
    { SSE2,                 { afs_copy_yc48_line_sse,    afs_copy_yc48_line_sse,    afs_convert_nv16_yc48_sse2,   afs_convert_nv16_yc48up_sse2,   afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2, afs_convert_nv16_yuy2_sse2 } },
};

static const struct {
    DWORD simd;
    func_analyzemap_filter analyzemap_filter;
} FUNC_ANALYZEMAP_LIST[] = {
    { AVX2|AVX,       afs_analyzemap_filter_avx2_plus  },
    { AVX|SSSE3|SSE2, afs_analyzemap_filter_avx_plus   },
    { SSSE3|SSE2,     afs_analyzemap_filter_ssse3_plus },
    { SSE2,           afs_analyzemap_filter_sse2_plus  },
    { NONE,           afs_analyzemap_filter_mmx        },
};

static const struct {
    DWORD simd;
    func_merge_scan merge_scan;
} FUNC_MERGE_SCAN_LIST[] = {
#if AFS_USE_XBYAK
    { AVX512BW|AVX2|AVX, NULL                     },
#endif //#if AFS_USE_XBYAK
    { AVX2|AVX,          afs_merge_scan_avx2_plus },
    { AVX|SSE2,          afs_merge_scan_avx_plus  },
    { SSE2,              afs_merge_scan_sse2_plus },
    { NONE,              afs_merge_scan_mmx       },
};

static const struct {
    DWORD simd;
    func_yuy2up yuy2up[8];
} FUNC_YUY2UP_LIST[] = {
    { AVX2|AVX,             { afs_copy_yc48_frame_sse2, afs_yuy2up_frame_avx2,   afs_convert_yc48_to_nv16_avx2,   afs_convert_yc48_to_nv16_avx2,   afs_copy_yuy2_nv16_avx2, afs_copy_yuy2_nv16_avx2, afs_copy_yuy2_nv16_avx2, afs_copy_yuy2_nv16_avx2 } },
    { AVX|SSE41|SSSE3|SSE2, { afs_copy_yc48_frame_sse2, afs_yuy2up_frame_avx,    afs_convert_yc48_to_nv16_avx,    afs_convert_yc48_to_nv16_avx,    afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2 } },
    { SSE41|SSSE3|SSE2,     { afs_copy_yc48_frame_sse2, afs_yuy2up_frame_sse4_1, afs_convert_yc48_to_nv16_sse4_1, afs_convert_yc48_to_nv16_sse4_1, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2 } },
    { SSSE3|SSE2,           { afs_copy_yc48_frame_sse2, afs_yuy2up_frame_sse2,   afs_convert_yc48_to_nv16_ssse3,  afs_convert_yc48_to_nv16_ssse3,  afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2 } },
    { SSE2,                 { afs_copy_yc48_frame_sse2, afs_yuy2up_frame_sse2,   afs_convert_yc48_to_nv16_sse2,   afs_convert_yc48_to_nv16_sse2,   afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2 } },
    { NONE,                 { afs_copy_yc48_frame_sse2, afs_yuy2up_frame_mmx,    afs_convert_yc48_to_nv16_sse2,   afs_convert_yc48_to_nv16_sse2,   afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2, afs_copy_yuy2_nv16_sse2 } }
};

static const struct {
    DWORD simd;
    AFS_FUNC_GET_COUNT get_count;
} FUNC_GET_COUNT_LIST[] = {
    { AVX2|AVX|POPCNT, { afs_get_stripe_count_avx2,        afs_get_motion_count_avx2        } },
    { AVX|POPCNT|SSE2, { afs_get_stripe_count_avx,         afs_get_motion_count_avx         } },
    { POPCNT|SSE2,     { afs_get_stripe_count_sse2_popcnt, afs_get_motion_count_sse2_popcnt } },
    { SSE2,            { afs_get_stripe_count_sse2,        afs_get_motion_count_sse2        } },
    { NONE,            { afs_get_stripe_count,             afs_get_motion_count             } },
};

void get_afs_func_list(AFS_FUNC *func_list, char *simd_select) {
    DWORD simd_mask = 0xffffffff;
    bool use_xbyak = (AFS_USE_XBYAK) ? true : false;
    if (strncmp(simd_select, "auto", strlen("auto"))) {
        if (strncmp(simd_select, "avx512_xbyak", strlen("avx512_xbyak")) == 0
            || strncmp(simd_select, "avx512", strlen("avx512")) == 0) {
            simd_mask = AVX512F|AVX512BW|AVX2|AVX|POPCNT|SSE41|SSSE3|SSE2;
        } else if (strncmp(simd_select, "avx2_xbyak", strlen("avx2_xbyak")) == 0) {
            simd_mask = AVX2FAST|AVX2|AVX|POPCNT|SSE41|SSSE3|SSE2;
        } else if (strncmp(simd_select, "avx2_xbyak_slow", strlen("avx2_xbyak_slow")) == 0) {
            simd_mask = AVX2|AVX|POPCNT|SSE41|SSSE3|SSE2;
        } else if (strncmp(simd_select, "avx2", strlen("avx2")) == 0) {
            simd_mask = AVX2FAST|AVX2|AVX|POPCNT|SSE41|SSSE3|SSE2;
            use_xbyak = false;
        } else if (strncmp(simd_select, "avx", strlen("avx")) == 0) {
            simd_mask = AVX|POPCNT|SSE41|SSSE3|SSE2;
            use_xbyak = false;
        } else if (strncmp(simd_select, "sse4.1", strlen("sse4.1")) == 0) {
            simd_mask = SSE41|SSSE3|SSE2;
            use_xbyak = false;
        } else if (strncmp(simd_select, "ssse3", strlen("ssse3")) == 0) {
            simd_mask = SSSE3|SSE2;
            use_xbyak = false;
        } else {
            simd_mask = SSE2;
            use_xbyak = false;
        }
    }
    const DWORD simd_avail = get_availableSIMD() & simd_mask;
    ZeroMemory(func_list, sizeof(func_list[0]));
    func_list->simd_avail = simd_avail;
    func_list->use_xbyak = FALSE;
    for (int i = (AFS_USE_XBYAK && !use_xbyak) ? 2 : 0; i < _countof(FUNC_ANALYZE_LIST); i++) {
        if ((FUNC_ANALYZE_LIST[i].simd & simd_avail) == FUNC_ANALYZE_LIST[i].simd) {
            memcpy(func_list->analyze, FUNC_ANALYZE_LIST[i].analyze, sizeof(func_list->analyze));
            func_list->simd_used = FUNC_ANALYZE_LIST[i].simd;
            func_list->use_xbyak = func_list->analyze[0].analyze_main[0] == nullptr;
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_BLEND_LIST); i++) {
        if ((FUNC_BLEND_LIST[i].simd & simd_avail) == FUNC_BLEND_LIST[i].simd) {
            memcpy(func_list->blend, FUNC_BLEND_LIST[i].blend, sizeof(func_list->blend));
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_MIE_SPOT_LIST); i++) {
        if ((FUNC_MIE_SPOT_LIST[i].simd & simd_avail) == FUNC_MIE_SPOT_LIST[i].simd) {
            memcpy(func_list->mie_spot, FUNC_MIE_SPOT_LIST[i].mie_spot, sizeof(func_list->mie_spot));
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_MIE_INTER_LIST); i++) {
        if ((FUNC_MIE_INTER_LIST[i].simd & simd_avail) == FUNC_MIE_INTER_LIST[i].simd) {
            memcpy(func_list->mie_inter, FUNC_MIE_INTER_LIST[i].mie_inter, sizeof(func_list->mie_inter));
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_DEINT4_LIST); i++) {
        if ((FUNC_DEINT4_LIST[i].simd & simd_avail) == FUNC_DEINT4_LIST[i].simd) {
            memcpy(func_list->deint4, FUNC_DEINT4_LIST[i].deint4, sizeof(func_list->deint4));
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_COPY_LINE_LIST); i++) {
        if ((FUNC_COPY_LINE_LIST[i].simd & simd_avail) == FUNC_COPY_LINE_LIST[i].simd) {
            memcpy(func_list->copy_line, &FUNC_COPY_LINE_LIST[i].copy_line, sizeof(func_list->copy_line));
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_ANALYZEMAP_LIST); i++) {
        if ((FUNC_ANALYZEMAP_LIST[i].simd & simd_avail) == FUNC_ANALYZEMAP_LIST[i].simd) {
            func_list->analyzemap_filter = FUNC_ANALYZEMAP_LIST[i].analyzemap_filter;
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_MERGE_SCAN_LIST); i++) {
        if ((FUNC_MERGE_SCAN_LIST[i].simd & simd_avail) == FUNC_MERGE_SCAN_LIST[i].simd) {
            func_list->merge_scan = FUNC_MERGE_SCAN_LIST[i].merge_scan;
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_YUY2UP_LIST); i++) {
        if ((FUNC_YUY2UP_LIST[i].simd & simd_avail) == FUNC_YUY2UP_LIST[i].simd) {
            memcpy(func_list->yuy2up, FUNC_YUY2UP_LIST[i].yuy2up, sizeof(func_list->yuy2up));
            break;
        }
    }
    for (int i = 0; i < _countof(FUNC_GET_COUNT_LIST); i++) {
        if ((FUNC_GET_COUNT_LIST[i].simd & simd_avail) == FUNC_GET_COUNT_LIST[i].simd) {
            memcpy(&func_list->get_count, &FUNC_GET_COUNT_LIST[i].get_count, sizeof(func_list->get_count));
            break;
        }
    }
}

const char *simd_str(DWORD simd, BOOL use_xbyak) {
    if (use_xbyak) {
        if (simd & AVX512VBMI) return "avx512vbmi/avx2(x)";
        if (simd & AVX512BW)   return "avx512bw/avx2(x)";
        if (simd & AVX512F)    return "avx512f/avx2(x)";
        if (simd & AVX2FAST)   return "avx2(x)";
        if (simd & AVX2)       return "avx2(xs)";
    }
    if (simd & AVX2)       return "avx2";
    if (simd & AVX)        return "avx";
    if (simd & SSE41)      return "sse4.1";
    if (simd & SSSE3)      return "ssse3";
    return "sse2";
}
