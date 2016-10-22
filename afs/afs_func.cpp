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
#endif
    return simd;
}

#define analyze_avx2 

static const struct {
    DWORD simd;
    AFS_FUNC_ANALYZE analyze[2];
} FUNC_ANALYZE_LIST[] = {
    //set_thresholdで変数を渡しているため、これら5つの関数は必ずセットで
    { AVX2|AVX|POPCNT, {
        { 31, 32, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_avx2,     NULL, { afs_analyze_12_avx2_plus2,     afs_analyze_1_avx2_plus2, afs_analyze_2_avx2_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_avx, NULL, { afs_analyze_12_nv16_avx_plus2, NULL,                     NULL                     } }
    } },
    { AVX|POPCNT|SSE41|SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_avx,      NULL, { afs_analyze_12_avx_plus2,      afs_analyze_1_avx_plus2, afs_analyze_2_avx_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_avx, NULL, { afs_analyze_12_nv16_avx_plus2, NULL,                    NULL                    } },
    } },
    { POPCNT|SSE41|SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_sse4_1_popcnt,      NULL, { afs_analyze_12_sse4_1_popcnt_plus2,      afs_analyze_1_sse4_1_popcnt_plus2, afs_analyze_2_sse4_1_popcnt_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_sse4_1_popcnt, NULL, { afs_analyze_12_nv16_sse4_1_popcnt_plus2, NULL,                              NULL                              } }
    } },
    { SSE41|SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_sse4_1,      NULL, { afs_analyze_12_sse4_1_plus2,      afs_analyze_1_sse4_1_plus2, afs_analyze_2_sse4_1_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_sse4_1, NULL, { afs_analyze_12_nv16_sse4_1_plus2, NULL,                       NULL                       } }
    } },
    { SSSE3|SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_ssse3,      NULL, { afs_analyze_12_ssse3_plus2,      afs_analyze_1_ssse3_plus2, afs_analyze_2_ssse3_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_ssse3, NULL, { afs_analyze_12_nv16_ssse3_plus2, NULL,                      NULL                      } },
    } },
    { SSE2, {
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_sse2,      NULL, { afs_analyze_12_sse2_plus2,      afs_analyze_1_sse2_plus2, afs_analyze_2_sse2_plus2 } },
        { 15, 16, BLOCK_SIZE_YCP, TRUE,  afs_analyze_set_threshold_nv16_sse2, NULL, { afs_analyze_12_nv16_sse2_plus2, NULL,                     NULL                     } }
    } },
    { NONE, {
        {  7,  4,  4, FALSE, afs_analyze_set_threshold_mmx, afs_analyze_shrink_info_mmx_plus, { afs_analyze_12_mmx_plus2, afs_analyze_1_mmx_plus2, afs_analyze_2_mmx_plus2 } },
        {  7,  4,  4, FALSE, NULL, NULL, { NULL, NULL, NULL } }
    } },
};       

static const struct {
    DWORD simd;
    func_blend blend[4];
} FUNC_BLEND_LIST[] = {
    { AVX2|AVX,             { afs_blend_avx2,   afs_blend_avx2,   afs_blend_nv16up_avx,    afs_blend_nv16_avx    } },
    { AVX|SSE41|SSSE3|SSE2, { afs_blend_avx,    afs_blend_avx,    afs_blend_nv16up_avx,    afs_blend_nv16_avx    } },
    { SSE41|SSSE3|SSE2,     { afs_blend_sse4_1, afs_blend_sse4_1, afs_blend_nv16up_sse4_1, afs_blend_nv16_sse4_1 } },
    { SSE2,                 { afs_blend_sse2,   afs_blend_sse2,   afs_blend_nv16up_ssse3,  afs_blend_nv16_ssse3  } },
    { NONE,                 { afs_blend_mmx,    afs_blend_mmx,    afs_blend_nv16up_sse2,   afs_blend_nv16_sse2   } },
};

static const struct {
    DWORD simd;
    func_mie_spot mie_spot[4];
} FUNC_MIE_SPOT_LIST[] = {
    { AVX2|AVX,         { afs_mie_spot_avx2,  afs_mie_spot_avx2,  afs_mie_spot_nv16_avx,     afs_mie_spot_nv16_avx    } },
    { AVX|SSE2,         { afs_mie_spot_avx,   afs_mie_spot_avx,   afs_mie_spot_nv16_avx,     afs_mie_spot_nv16_avx    } },
    { SSE41|SSSE3|SSE2, { afs_mie_spot_sse2,  afs_mie_spot_sse2,  afs_mie_spot_nv16_sse4_1,  afs_mie_spot_nv16_sse4_1 } },
    { SSSE3|SSE2,       { afs_mie_spot_sse2,  afs_mie_spot_sse2,  afs_mie_spot_nv16_ssse3,   afs_mie_spot_nv16_ssse3  } },
    { SSE2,             { afs_mie_spot_sse2,  afs_mie_spot_sse2,  afs_mie_spot_nv16_sse2,    afs_mie_spot_nv16_sse2   } },
    { NONE,             { afs_mie_spot_mmx,   afs_mie_spot_mmx,   afs_mie_spot_nv16_sse2,    afs_mie_spot_nv16_sse2   } },
};

static const struct {
    DWORD simd;
    func_mie_inter mie_inter[4];
} FUNC_MIE_INTER_LIST[] = {
    { AVX2|AVX,         { afs_mie_inter_avx2,  afs_mie_inter_avx2,  afs_mie_inter_nv16_avx,     afs_mie_inter_nv16_avx    } },
    { AVX|SSE2,         { afs_mie_inter_avx,   afs_mie_inter_avx,   afs_mie_inter_nv16_avx,     afs_mie_inter_nv16_avx    } },
    { SSE41|SSSE3|SSE2, { afs_mie_inter_sse2,  afs_mie_inter_sse2,  afs_mie_inter_nv16_sse4_1,  afs_mie_inter_nv16_sse4_1 } },
    { SSSE3|SSE2,       { afs_mie_inter_sse2,  afs_mie_inter_sse2,  afs_mie_inter_nv16_ssse3,   afs_mie_inter_nv16_ssse3  } },
    { SSE2,             { afs_mie_inter_sse2,  afs_mie_inter_sse2,  afs_mie_inter_nv16_sse2,    afs_mie_inter_nv16_sse2   } },
    { NONE,             { afs_mie_inter_mmx,   afs_mie_inter_mmx,   afs_mie_inter_nv16_sse2,    afs_mie_inter_nv16_sse2   } },
};

static const struct {
    DWORD simd;
    func_deint4 deint4[4];
} FUNC_DEINT4_LIST[] = {
    { AVX2|AVX,         { afs_deint4_avx2,  afs_deint4_avx2,  afs_deint4_nv16_avx,     afs_deint4_nv16_avx    } },
    { AVX|SSE2,         { afs_deint4_avx,   afs_deint4_avx,   afs_deint4_nv16_avx,     afs_deint4_nv16_avx    } },
    { SSE41|SSSE3|SSE2, { afs_deint4_sse2,  afs_deint4_sse2,  afs_deint4_nv16_sse4_1,  afs_deint4_nv16_sse4_1 } },
    { SSSE3|SSE2,       { afs_deint4_sse2,  afs_deint4_sse2,  afs_deint4_nv16_ssse3,   afs_deint4_nv16_ssse3  } },
    { SSE2,             { afs_deint4_sse2,  afs_deint4_sse2,  afs_deint4_nv16_sse2,    afs_deint4_nv16_sse2   } },
    { NONE,             { afs_deint4_mmx,   afs_deint4_mmx,   afs_deint4_nv16_sse2,    afs_deint4_nv16_sse2   } },
};

static const struct {
    DWORD simd;
    func_copy_line copy_line[4];
} FUNC_COPY_LINE_LIST[] = {
    { AVX2|AVX,             { afs_copy_yc48_line_sse, afs_copy_yc48_line_avx2, afs_convert_nv16_yc48_avx,    afs_convert_nv16_yc48up_avx    } },
    { AVX|SSE41|SSSE3|SSE2, { afs_copy_yc48_line_sse, afs_copy_yc48_line_sse,  afs_convert_nv16_yc48_avx,    afs_convert_nv16_yc48up_avx    } },
    { SSE41|SSSE3|SSE2,     { afs_copy_yc48_line_sse, afs_copy_yc48_line_sse,  afs_convert_nv16_yc48_sse4_1, afs_convert_nv16_yc48up_sse4_1 } },
    { SSSE3|SSE2,           { afs_copy_yc48_line_sse, afs_copy_yc48_line_sse,  afs_convert_nv16_yc48_ssse3,  afs_convert_nv16_yc48up_ssse3  } },
    { SSE2,                 { afs_copy_yc48_line_sse, afs_copy_yc48_line_sse,  afs_convert_nv16_yc48_sse2,   afs_convert_nv16_yc48up_sse2   } },
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
    { AVX2|AVX, afs_merge_scan_avx2_plus },
    { AVX|SSE2, afs_merge_scan_avx_plus  },
    { SSE2,     afs_merge_scan_sse2_plus },
    { NONE,     afs_merge_scan_mmx       },
};

static const struct {
    DWORD simd;
    func_yuy2up yuy2up[4];
} FUNC_YUY2UP_LIST[] = {
    { AVX2|AVX,             { afs_yuy2up_frame_avx2,   afs_yuy2up_frame_avx2,   afs_convert_yc48_to_nv16_avx2,   afs_convert_yc48_to_nv16_avx2   } },
    { AVX|SSE41|SSSE3|SSE2, { afs_yuy2up_frame_avx,    afs_yuy2up_frame_avx,    afs_convert_yc48_to_nv16_avx,    afs_convert_yc48_to_nv16_avx    } },
    { SSE41|SSSE3|SSE2,     { afs_yuy2up_frame_sse4_1, afs_yuy2up_frame_sse4_1, afs_convert_yc48_to_nv16_sse4_1, afs_convert_yc48_to_nv16_sse4_1 } },
    { SSSE3|SSE2,           { afs_yuy2up_frame_sse2,   afs_yuy2up_frame_sse2,   afs_convert_yc48_to_nv16_ssse3,  afs_convert_yc48_to_nv16_ssse3  } },
    { SSE2,                 { afs_yuy2up_frame_sse2,   afs_yuy2up_frame_sse2,   afs_convert_yc48_to_nv16_sse2,   afs_convert_yc48_to_nv16_sse2   } },
    { NONE,                 { afs_yuy2up_frame_mmx,    afs_yuy2up_frame_mmx,    afs_convert_yc48_to_nv16_sse2,   afs_convert_yc48_to_nv16_sse2   } }
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

void get_afs_func_list(AFS_FUNC *func_list) {
    const DWORD simd_avail = get_availableSIMD();
    ZeroMemory(func_list, sizeof(func_list[0]));
    func_list->simd_avail = simd_avail;
    for (int i = 0; i < _countof(FUNC_ANALYZE_LIST); i++) {
        if ((FUNC_ANALYZE_LIST[i].simd & simd_avail) == FUNC_ANALYZE_LIST[i].simd) {
            memcpy(func_list->analyze, FUNC_ANALYZE_LIST[i].analyze, sizeof(func_list->analyze));
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
