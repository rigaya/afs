#include <emmintrin.h>
#include <smmintrin.h>
#include <Windows.h>
#include "filter.h"

#define ENABLE_FUNC_BASE
#include "afs_filter_simd.h"

 void __stdcall afs_merge_scan_avx(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	 afs_merge_scan_simd(dst, src0, src1, si_w, h);
 }
 
 void __stdcall afs_merge_scan_avx_plus(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	 afs_merge_scan_simd_plus(dst, src0, src1, si_w, h);
 }

void __stdcall afs_analyzemap_filter_avx(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd(sip, si_w, w, h, AVX|SSE41|SSSE3|SSE2);
}

void __stdcall afs_analyzemap_filter_avx_plus(BYTE* sip, int si_w, int w, int h) {
	afs_analyzemap_filter_simd_plus(sip, si_w, w, h, AVX|SSE41|SSSE3|SSE2);
}