#include <Windows.h>
#include "filter.h"
#define ENABLE_FUNC_BASE
#define USE_POPCNT 1

#include "afs_simd.h"

void __stdcall afs_get_stripe_count_sse2_popcnt(int *count, AFS_SCAN_DATA* sp0, AFS_SCAN_DATA* sp1, AFS_STRIPE_DATA *sp, int si_w, int scan_w, int scan_h) {
	afs_get_stripe_count_simd(count, sp0, sp1, sp, si_w, scan_w, scan_h);
}

void __stdcall afs_get_motion_count_sse2_popcnt(int *motion_count, AFS_SCAN_DATA *sp, int si_w, int scan_w, int scan_h) {
	afs_get_motion_count_simd(motion_count, sp, si_w, scan_w, scan_h);
}
