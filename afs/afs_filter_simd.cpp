#include <emmintrin.h>
#include <Windows.h>
#include "filter.h"

#define ENABLE_FUNC_BASE
#include "afs_filter_simd.h"

 void __stdcall afs_merge_scan_sse2(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	 afs_merge_scan_simd(dst, src0, src1, si_w, h);
 }
 
 void __stdcall afs_merge_scan_sse2_plus(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	 afs_merge_scan_simd_plus(dst, src0, src1, si_w, h);
 }

void __stdcall afs_analyzemap_filter_sse2(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd(sip, si_w, w, h, SSE2);
}
void __stdcall afs_analyzemap_filter_ssse3(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd(sip, si_w, w, h, SSSE3|SSE2);
}

void __stdcall afs_analyzemap_filter_sse2_plus(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd_plus(sip, si_w, w, h, SSE2);
}
void __stdcall afs_analyzemap_filter_ssse3_plus(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd_plus(sip, si_w, w, h, SSSE3|SSE2);
}
