#pragma once

void __stdcall afs_analyzemap_filter_sse2(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_sse2_plus(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_ssse3(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_ssse3_plus(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx_plus(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx2(unsigned char* sip, int si_w, int w, int h);
void __stdcall afs_analyzemap_filter_avx2_plus(unsigned char* sip, int si_w, int w, int h);

void __stdcall afs_merge_scan_sse2(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);
void __stdcall afs_merge_scan_sse2_plus(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);
void __stdcall afs_merge_scan_avx(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);
void __stdcall afs_merge_scan_avx_plus(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);
void __stdcall afs_merge_scan_avx2(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);
void __stdcall afs_merge_scan_avx2_plus(unsigned char* dst, unsigned char* src0, unsigned char* src1, int si_w, int h);

#ifdef ENABLE_FUNC_BASE
#include <tmmintrin.h> //SSSE3
#include "simd_util.h"

static const _declspec(align(16)) BYTE pqb_mask_a[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static const _declspec(align(16)) BYTE pqb_mask_s[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff,
};
static const _declspec(align(16)) BYTE pb_mask_03[] = {
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
};
static const _declspec(align(16)) BYTE pb_mask_04[] = {
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
};
//static const _declspec(align(16)) BYTE pb_mask_f8[] = {
//    0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 
//};
static const _declspec(align(16)) BYTE pb_mask_33[] = {
    0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 
};
static const _declspec(align(16)) BYTE pb_mask_f3[] = {
    0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 0xf3, 
};
static const _declspec(align(16)) BYTE pb_mask_44[] = {
    0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 
};

 static void __forceinline __stdcall afs_merge_scan_simd(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	int step = 0;
	__m128i x0, x1, x2, x3, x4, x5;
	const __m128i xPbMask33 = _mm_load_si128((__m128i*)(pb_mask_33));
	const __m128i xPbMaskf3 = _mm_load_si128((__m128i*)(pb_mask_f3));
	for (int jw = si_w >> 4; jw; jw--) {
		BYTE *ptr_dst = dst  + step;
		BYTE *ptr_src0 = src0 + step;
		BYTE *ptr_src1 = src1 + step;
		x0 = _mm_loadu_si128((__m128i*)(ptr_src0 + 0));
		x1 = _mm_loadu_si128((__m128i*)(ptr_src0 + si_w));
		x2 = _mm_loadu_si128((__m128i*)(ptr_src1 + 0));
		x3 = _mm_loadu_si128((__m128i*)(ptr_src1 + si_w));
		ptr_src0 += si_w;
		ptr_src1 += si_w;
		_mm_prefetch((char *)(ptr_src0 + si_w), _MM_HINT_NTA);
		x4 = x0;
		x4 = _mm_or_si128(x4, x1);
		x4 = _mm_or_si128(x4, xPbMaskf3);
		x4 = _mm_and_si128(x4, x0);
		x5 = x2;
		x5 = _mm_or_si128(x5, x3);
		x5 = _mm_or_si128(x5, xPbMaskf3);
		x5 = _mm_and_si128(x5, x2);
		x4 = _mm_and_si128(x4, x5);
		x4 = _mm_and_si128(x4, _mm_load_si128((__m128i*)pb_mask_44));
		_mm_prefetch((char *)(ptr_src1 + si_w), _MM_HINT_NTA);
		x5 = x0;
		x5 = _mm_andnot_si128(x5, xPbMask33);
		x4 = _mm_or_si128(x4, x5);
		_mm_storeu_si128((__m128i*)ptr_dst, x4);
		ptr_dst += si_w;


		for (int ih = h - 2; ih; ih--) {
			x4 = x0;
			x0 = x1;
			x1 = _mm_loadu_si128((__m128i*)(ptr_src0 + si_w));
			ptr_src0 += si_w;
			_mm_prefetch((char *)(ptr_src0 + si_w), _MM_HINT_NTA);
			x4 = _mm_or_si128(x4, x1);
			x4 = _mm_or_si128(x4, xPbMaskf3);
			x4 = _mm_and_si128(x4, x0);
			x5 = x2;
			x2 = x3;
			
			x3 = _mm_loadu_si128((__m128i*)(ptr_src1 + si_w));
			ptr_src1 += si_w;
			_mm_prefetch((char *)(ptr_src1 + si_w), _MM_HINT_NTA);
			x5 = _mm_or_si128(x5, x3);
			x5 = _mm_or_si128(x5, xPbMaskf3);
			x5 = _mm_and_si128(x5, x2);
			x4 = _mm_and_si128(x4, x5);
			x4 = _mm_and_si128(x4, _mm_load_si128((__m128i*)pb_mask_44));
			x5 = x0;
			x5 = _mm_andnot_si128(x5, xPbMask33);
			x4 = _mm_or_si128(x4, x5);
			_mm_storeu_si128((__m128i*)ptr_dst, x4);
			ptr_dst += si_w;
		}
		
		x4 = x0;
		x4 = _mm_or_si128(x4, x1);
		x4 = _mm_or_si128(x4, xPbMaskf3);
		x4 = _mm_and_si128(x4, x1);
		x5 = x2;
		x5 = _mm_or_si128(x5, x3);
		x5 = _mm_or_si128(x5, xPbMaskf3);
		x5 = _mm_and_si128(x5, x3);
		x4 = _mm_and_si128(x4, x5);
		x4 = _mm_and_si128(x4, _mm_load_si128((__m128i*)pb_mask_44));
		x5 = x1;
		
		x5 = _mm_andnot_si128(x5, xPbMask33);
		x4 = _mm_or_si128(x4, x5);
		_mm_storeu_si128((__m128i*)ptr_dst, x4);
		
		step += 16;
	}
 }
 
 static void __forceinline __stdcall afs_merge_scan_simd_plus(BYTE* dst, BYTE* src0, BYTE* src1, int si_w, int h) {
	int step = 0;
	__m128i x0, x1, x2, x3, x4, x5;
	const __m128i xPbMask33 = _mm_load_si128((__m128i*)(pb_mask_33));
	const __m128i xPbMaskf3 = _mm_load_si128((__m128i*)(pb_mask_f3));
	const __m128i xPbMask44 = _mm_load_si128((__m128i*)(pb_mask_44));
	const int MAX_BLOCK_SIZE = 1920;
	const int block_size = (si_w / (((si_w + MAX_BLOCK_SIZE-1) / MAX_BLOCK_SIZE)) + 15) & ~15;
	BYTE __declspec(align(16)) buffer[MAX_BLOCK_SIZE * 4];
	BYTE *bufptr_0 = buffer;
	BYTE *bufptr_1 = buffer + (block_size<<1);
	for (int jw = 0, step; jw < si_w; jw += block_size) {
		step = si_w - jw;
		BOOL over_block_size = (step > block_size);
		step = over_block_size * block_size + !over_block_size * step;
		BYTE *ptr_dst = dst  + jw;
		BYTE *ptr_src0 = src0 + jw;
		BYTE *ptr_src1 = src1 + jw;
		for (int iw = 0; iw < step; iw += 16) {
			x0 = _mm_load_si128((__m128i*)(ptr_src0+iw+ 0));
			x1 = _mm_load_si128((__m128i*)(ptr_src0+iw+ si_w));
			x2 = _mm_load_si128((__m128i*)(ptr_src1+iw+ 0));
			x3 = _mm_load_si128((__m128i*)(ptr_src1+iw+ si_w));
			x4 = x0;
			x4 = _mm_or_si128(x4, x1);
			x4 = _mm_or_si128(x4, xPbMaskf3);
			x4 = _mm_and_si128(x4, x0);
			x5 = x2;
			x5 = _mm_or_si128(x5, x3);
			x5 = _mm_or_si128(x5, xPbMaskf3);
			x5 = _mm_and_si128(x5, x2);
			x4 = _mm_and_si128(x4, x5);
			x4 = _mm_and_si128(x4, xPbMask44);
			x5 = x0;
			x5 = _mm_andnot_si128(x5, xPbMask33);
			x4 = _mm_or_si128(x4, x5);
			_mm_store_si128((__m128i*)(ptr_dst+iw), x4);

			_mm_store_si128((__m128i*)(bufptr_0           +iw), x0);
			_mm_store_si128((__m128i*)(bufptr_1           +iw), x1);
			_mm_store_si128((__m128i*)(bufptr_0+block_size+iw), x2);
			_mm_store_si128((__m128i*)(bufptr_1+block_size+iw), x3);
		}
		ptr_dst += si_w;
		ptr_src0 += si_w;
		ptr_src1 += si_w;

		for (int ih = h - 2; ih; ih--) {
			for (int iw = 0; iw < step; iw += 16) {
				x4 = _mm_load_si128((__m128i*)(bufptr_0+           iw));
				x0 = _mm_load_si128((__m128i*)(bufptr_1+           iw));
				x5 = _mm_load_si128((__m128i*)(bufptr_0+block_size+iw));
				x2 = _mm_load_si128((__m128i*)(bufptr_1+block_size+iw));
				x1 = _mm_load_si128((__m128i*)(ptr_src0 + si_w + iw));
				x3 = _mm_load_si128((__m128i*)(ptr_src1 + si_w + iw));
				_mm_store_si128((__m128i*)(bufptr_0           +iw), x1);
				_mm_store_si128((__m128i*)(bufptr_0+block_size+iw), x3);
				x4 = _mm_or_si128(x4, x1);
				x4 = _mm_or_si128(x4, xPbMaskf3);
				x4 = _mm_and_si128(x4, x0);
				x5 = _mm_or_si128(x5, x3);
				x5 = _mm_or_si128(x5, xPbMaskf3);
				x5 = _mm_and_si128(x5, x2);
				x4 = _mm_and_si128(x4, x5);
				x4 = _mm_and_si128(x4, xPbMask44);
				x5 = x0;
				x5 = _mm_andnot_si128(x5, xPbMask33);
				x4 = _mm_or_si128(x4, x5);
				_mm_store_si128((__m128i*)(ptr_dst+iw), x4);
			}
			SWAP(BYTE *, bufptr_0, bufptr_1);
			ptr_dst += si_w;
			ptr_src0 += si_w;
			ptr_src1 += si_w;
		}
		for (int iw = 0; iw < step; iw += 16) {
			x4 = _mm_load_si128((__m128i*)(bufptr_0+           iw));
			x1 = _mm_load_si128((__m128i*)(bufptr_1+           iw));
			x5 = _mm_load_si128((__m128i*)(bufptr_0+block_size+iw));
			x3 = _mm_load_si128((__m128i*)(bufptr_1+block_size+iw));
			x4 = _mm_or_si128(x4, x1);
			x4 = _mm_or_si128(x4, xPbMaskf3);
			x4 = _mm_and_si128(x4, x1);
			x5 = _mm_or_si128(x5, x3);
			x5 = _mm_or_si128(x5, xPbMaskf3);
			x5 = _mm_and_si128(x5, x3);
			x4 = _mm_and_si128(x4, x5);
			x4 = _mm_and_si128(x4, xPbMask44);
			x5 = x1;
		
			x5 = _mm_andnot_si128(x5, xPbMask33);
			x4 = _mm_or_si128(x4, x5);
			_mm_store_si128((__m128i*)(ptr_dst+iw), x4);
		}
	}
 }

static void __forceinline __stdcall afs_analyzemap_filter_simd(BYTE* sip, int si_w, int w, int h, DWORD simd) {
	__m128i x0, x1, x2, x3, x4, x5;
	__m128i x6 = _mm_load_si128((__m128i*)(pb_mask_03));
	__m128i x7 = _mm_load_si128((__m128i*)(pb_mask_04));
	BYTE *ptr_sip_out = sip;
	BYTE *ptr_sip_in = 0;

	//loop horizontal_1
	for (int jh = h; jh; jh--) {
		int iw = w;
		x1 = _mm_loadu_si128((__m128i*)(ptr_sip_out));
		x2 = _mm_loadu_si128((__m128i*)(ptr_sip_out+16));
		iw -= 16;
		
		x3 = x1;
		x3 = _mm_slli_si128(x3, 1);
		x5 = x1;
		x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + 1*16)));
		x3 = _mm_or_si128(x3, x5);
		x4 = _mm_alignr_epi8_simd(x2, x1, 1);
		
		x5 = x3;
		x3 = _mm_or_si128(x3, x4);
		x5 = _mm_and_si128(x5, x4);
		x3 = _mm_and_si128(x3, x6);
		x5 = _mm_and_si128(x5, x7);
		x3 = _mm_or_si128(x3, x1);
		x3 = _mm_or_si128(x3, x5);
		
		_mm_storeu_si128((__m128i*)ptr_sip_out, x3);
		ptr_sip_in = ptr_sip_out + 16;
		
		for (; iw > 16; iw -= 16) {
			x0 = x1;
			x1 = x2;
			x2 = _mm_loadu_si128((__m128i*)(ptr_sip_in+1*16));
			
			x3 = _mm_alignr_epi8_simd(x1, x0, 15);
			x4 = _mm_alignr_epi8_simd(x2, x1, 1);
			
			x5 = x3;
			x3 = _mm_or_si128(x3, x4);
			x5 = _mm_and_si128(x5, x4);
			x3 = _mm_and_si128(x3, x6);
			x5 = _mm_and_si128(x5, x7);
			x3 = _mm_or_si128(x3, x1);
			x3 = _mm_or_si128(x3, x5);
			_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
			
			ptr_sip_in += 16;
		}
		
		x0 = _mm_load_si128((__m128i*)(pqb_mask_a + iw*16));
		x4 = x2;
		x4 = _mm_and_si128(x4, x0);
		x4 = _mm_srli_si128(x4, 1);
		x3 = _mm_alignr_epi8_simd(x2, x1, 15);
		x5 = x2;
		x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + iw*16)));
		x4 = _mm_or_si128(x4, x5);
		
		x5 = x3;
		x3 = _mm_or_si128(x3, x4);
		x5 = _mm_and_si128(x5, x4);
		x3 = _mm_and_si128(x3, x6);
		x5 = _mm_and_si128(x5, x7);
		x3 = _mm_or_si128(x3, x2);
		x3 = _mm_or_si128(x3, x5);
		x3 = _mm_and_si128(x3, x0);
		
		_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
		ptr_sip_out += si_w;
	}
	
	//loop vertical_1
	ptr_sip_out = sip;
	x7 = _mm_or_si128(x7, x6);
	for (int jw = si_w >> 4; jw; jw--) {
		int ih = h;
		x1 = _mm_loadu_si128((__m128i*)(ptr_sip_out));
		x2 = _mm_loadu_si128((__m128i*)(ptr_sip_out+si_w));
		ih -= 2;
		x3 = x2;
		x3 = _mm_and_si128(x3, x1);
		x3 = _mm_and_si128(x3, x7);
		x3 = _mm_or_si128(x3, x1);
		_mm_storeu_si128((__m128i*)ptr_sip_out, x3);
		ptr_sip_in = ptr_sip_out + si_w;
		for ( ; ih; ih--) {
			x0 = x1;
			x1 = x2;
			x2 = _mm_loadu_si128((__m128i*)(ptr_sip_in+si_w));
			
			x3 = x2;
			x3 = _mm_and_si128(x3, x0);
			x3 = _mm_and_si128(x3, x7);
			x3 = _mm_or_si128(x3, x1);
			_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
			ptr_sip_in += si_w;
		}
		x3 = x2;
		x3 = _mm_and_si128(x3, x1);
		x3 = _mm_and_si128(x3, x7);
		x3 = _mm_or_si128(x3, x2);
		_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
		ptr_sip_out += 16;
	}
	
	//loop horizontal_2
	ptr_sip_out = sip;
	x6 = _mm_cmpeq_epi8(x6, x6);
	x6 = _mm_xor_si128(x6, x7);
	for (int jh = h; jh; jh--) {
		int iw = w;
		x1 = _mm_loadu_si128((__m128i*)(ptr_sip_out));
		x2 = _mm_loadu_si128((__m128i*)(ptr_sip_out+16));
		iw -= 16;

		x3 = x1;
		x3 = _mm_slli_si128(x3, 1);
		x5 = x1;
		x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + 1*16)));
		x3 = _mm_or_si128(x3, x5);
		x4 = _mm_alignr_epi8_simd(x2, x1, 1);
		
		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x1);
		_mm_storeu_si128((__m128i*)ptr_sip_out, x3);
		ptr_sip_in = ptr_sip_out + 16;
		
		for ( ; iw > 16; iw -= 16) {
			x0 = x1;
			x1 = x2;
			x2 = _mm_loadu_si128((__m128i*)(ptr_sip_in+16));
			
			x3 = _mm_alignr_epi8_simd(x1, x0, 15);
			x4 = _mm_alignr_epi8_simd(x2, x1, 1);
			
			x3 = _mm_and_si128(x3, x4);
			x3 = _mm_or_si128(x3, x6);
			x3 = _mm_and_si128(x3, x1);
			_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
			
			ptr_sip_in += 16;
		}
		
		x0 = _mm_load_si128((__m128i*)(pqb_mask_a + iw*16));
		
		x4 = x2;
		x4 = _mm_and_si128(x4, x0);
		x4 = _mm_srli_si128(x4, 1);
		x3 = _mm_alignr_epi8_simd(x2, x1, 15);
		x5 = x2;
		x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + iw*16)));
		x4 = _mm_or_si128(x4, x5);
		
		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x2);
		x3 = _mm_and_si128(x3, x0);
		
		_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
		ptr_sip_out += si_w;
	}
	
	//loop vertical_2
	ptr_sip_out = sip;
	for (int jw = si_w >> 4; jw; jw--) {
		int ih = h;
		x1 = _mm_loadu_si128((__m128i*)(ptr_sip_out));
		x2 = _mm_loadu_si128((__m128i*)(ptr_sip_out+si_w));
		ih -= 2;
		x3 = x2;
		x3 = _mm_and_si128(x3, x1);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x1);
		_mm_storeu_si128((__m128i*)ptr_sip_out, x3);
		ptr_sip_in = ptr_sip_out + si_w;
		for ( ; ih; ih--) {
			x0 = x1;
			x1 = x2;
			x2 = _mm_loadu_si128((__m128i*)(ptr_sip_in+si_w));
			
			x3 = x2;
			x3 = _mm_and_si128(x3, x0);
			x3 = _mm_or_si128(x3, x6);
			x3 = _mm_and_si128(x3, x1);
			_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
			ptr_sip_in += si_w;
		}
		x3 = x2;
		x3 = _mm_and_si128(x3, x1);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x2);
		_mm_storeu_si128((__m128i*)ptr_sip_in, x3);
		ptr_sip_out += 16;
	}
}

static void __forceinline __stdcall afs_analyzemap_filter_simd_plus(BYTE* sip, int si_w, int w, int h, DWORD simd) {
	__m128i x0, x1, x2, x3, x4, x5;
	__m128i x6 = _mm_load_si128((__m128i*)(pb_mask_03));
	__m128i x7 = _mm_load_si128((__m128i*)(pb_mask_04));
	BYTE *ptr_sip, *ptr_sip_line;
	BYTE *buf_ptr;
	int iw, jh;
	const int BLOCK_SIZE = 4096;
	BYTE __declspec(align(16)) buffer[BLOCK_SIZE];
	//この関数ではsi_wがBLOCK_SIZEまでしか処理できないので、それ以上なら旧関数を使う
	if (si_w > BLOCK_SIZE)
		return (simd & SSSE3) ? afs_analyzemap_filter_ssse3(sip, si_w, w, h) : afs_analyzemap_filter_sse2(sip, si_w, w, h);

	////// loop_1 ////////////////////////////////////////////////////////////////////
	ptr_sip_line = sip;
	ptr_sip = ptr_sip_line;
	buf_ptr = buffer;
	//---- loop_1 - prepare for first line ---------------------------------------------
	x2 = _mm_load_si128((__m128i*)(ptr_sip+ 0));
	x1 = _mm_slli_si128(x2, 15);
	for (iw = w; iw > 16; iw -= 16, ptr_sip += 16, buf_ptr += 16) {
		//horizontal filtering
		x0 = x1;
		x1 = x2;
		x2 = _mm_load_si128((__m128i*)(ptr_sip+16));
			
		x3 = _mm_alignr_epi8_simd(x1, x0, 15);
		x4 = _mm_alignr_epi8_simd(x2, x1, 1);
			
		x5 = x3;
		x3 = _mm_or_si128(x3, x4);
		x5 = _mm_and_si128(x5, x4);
		x3 = _mm_and_si128(x3, x6);
		x5 = _mm_and_si128(x5, x7);
		x3 = _mm_or_si128(x3, x1);
		x3 = _mm_or_si128(x3, x5);
		_mm_store_si128((__m128i*)(ptr_sip), x3);
		_mm_store_si128((__m128i*)buf_ptr, x3);
	}
	//---- loop_1 - first line - last edge
	x0 = _mm_load_si128((__m128i*)(pqb_mask_a + iw*16));
	x4 = x2;
	x4 = _mm_and_si128(x4, x0);
	x4 = _mm_srli_si128(x4, 1);
	x3 = _mm_alignr_epi8_simd(x2, x1, 15);
	x5 = x2;
	x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + iw*16)));
	x4 = _mm_or_si128(x4, x5);
		
	x5 = x3;
	x3 = _mm_or_si128(x3, x4);
	x5 = _mm_and_si128(x5, x4);
	x3 = _mm_and_si128(x3, x6);
	x5 = _mm_and_si128(x5, x7);
	x3 = _mm_or_si128(x3, x2);
	x3 = _mm_or_si128(x3, x5);
	x3 = _mm_and_si128(x3, x0);
		
	_mm_store_si128((__m128i*)(ptr_sip), x3);
	_mm_store_si128((__m128i*)buf_ptr, x3);
	//---- loop_1 - prepare for first line - end ---------------------------------------------

	//---- loop_1 - main loop  ---------------------------------------------
	for (jh = 1; jh < h; jh++, ptr_sip_line += si_w) {
		buf_ptr = buffer;
		ptr_sip = ptr_sip_line;
		x2 = _mm_load_si128((__m128i*)(ptr_sip+si_w+ 0));
		x1 = _mm_slli_si128(x2, 15);
		for (iw = w; iw > 16; iw -= 16, ptr_sip += 16, buf_ptr += 16) {
			//horizontal filtering
			x0 = x1;
			x1 = x2;
			x2 = _mm_load_si128((__m128i*)(ptr_sip+si_w+16));
			
			x3 = _mm_alignr_epi8_simd(x1, x0, 15);
			x4 = _mm_alignr_epi8_simd(x2, x1, 1);
			
			x5 = x3;
			x3 = _mm_or_si128(x3, x4);
			x5 = _mm_and_si128(x5, x4);
			x3 = _mm_and_si128(x3, x6);
			x5 = _mm_and_si128(x5, x7);
			x3 = _mm_or_si128(x3, x1);
			x3 = _mm_or_si128(x3, x5);
			_mm_store_si128((__m128i*)(ptr_sip+si_w), x3);

			//vertical filtering
			x4 = _mm_load_si128((__m128i *)buf_ptr);
			x5 = _mm_load_si128((__m128i *)ptr_sip);

			x3 = _mm_and_si128(x3, x4);
			x3 = _mm_and_si128(x3, _mm_or_si128(x6, x7));
			x3 = _mm_or_si128(x3, x5);
			_mm_store_si128((__m128i*)ptr_sip, x3);
			_mm_store_si128((__m128i*)buf_ptr, x5);
		}
		//---- loop_1 - main loop - last edge ---------------------------
		//horizontal filtering
		x0 = _mm_load_si128((__m128i*)(pqb_mask_a + iw*16));
		x4 = x2;
		x4 = _mm_and_si128(x4, x0);
		x4 = _mm_srli_si128(x4, 1);
		x3 = _mm_alignr_epi8_simd(x2, x1, 15);
		x5 = x2;
		x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + iw*16)));
		x4 = _mm_or_si128(x4, x5);
		
		x5 = x3;
		x3 = _mm_or_si128(x3, x4);
		x5 = _mm_and_si128(x5, x4);
		x3 = _mm_and_si128(x3, x6);
		x5 = _mm_and_si128(x5, x7);
		x3 = _mm_or_si128(x3, x2);
		x3 = _mm_or_si128(x3, x5);
		x3 = _mm_and_si128(x3, x0);
		
		_mm_store_si128((__m128i*)(ptr_sip+si_w), x3);
		
		//vertical filtering
		x4 = _mm_load_si128((__m128i *)buf_ptr);
		x5 = _mm_load_si128((__m128i *)ptr_sip);

		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_and_si128(x3, _mm_or_si128(x6, x7));
		x3 = _mm_or_si128(x3, x5);
		_mm_store_si128((__m128i*)ptr_sip, x3);
		_mm_store_si128((__m128i*)buf_ptr, x5);
		//---- loop_1 - main loop - last edge end ------------------------
	}
	//---- loop_1 - main loop  - end  ------------------------------------------

	//---- loop_1 - last line --------------------------------------------
	buf_ptr = buffer;
	ptr_sip = ptr_sip_line;
	for (iw = 0; iw < si_w; iw += 16, ptr_sip += 16, buf_ptr += 16) {
		//vertical filtering
		x4 = _mm_load_si128((__m128i *)buf_ptr);
		x5 = _mm_load_si128((__m128i *)ptr_sip);
		x3 = x5;

		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_and_si128(x3, _mm_or_si128(x6, x7));
		x3 = _mm_or_si128(x3, x5);
		_mm_store_si128((__m128i*)ptr_sip, x3);
	}
	//---- loop_1 - last line - end ---------------------------------------------
	////// loop_1 - end  ////////////////////////////////////////////////////////////////////

	////// loop_2 ////////////////////////////////////////////////////////////////////
	ptr_sip_line = sip;
	ptr_sip = ptr_sip_line;
	buf_ptr = buffer;
	x6 = _mm_xor_si128(_mm_cmpeq_epi8(x6, x6), _mm_or_si128(x6, x7));
	//---- loop_2 - prepare for first line --------------------------------------------
	x2 = _mm_load_si128((__m128i*)(ptr_sip+ 0));
	x1 = _mm_slli_si128(x2, 15);
	for (iw = w; iw > 16; iw -= 16, ptr_sip += 16, buf_ptr += 16) {
		//horizontal filtering
		x0 = x1;
		x1 = x2;
		x2 = _mm_load_si128((__m128i*)(ptr_sip+16));
			
		x3 = _mm_alignr_epi8_simd(x1, x0, 15);
		x4 = _mm_alignr_epi8_simd(x2, x1, 1);
			
		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x1);
		_mm_store_si128((__m128i*)(ptr_sip), x3);
		_mm_store_si128((__m128i*)buf_ptr, x3);
	}
	//first line - last edge
	x0 = _mm_load_si128((__m128i*)(pqb_mask_a + iw*16));
		
	x4 = x2;
	x4 = _mm_and_si128(x4, x0);
	x4 = _mm_srli_si128(x4, 1);
	x3 = _mm_alignr_epi8_simd(x2, x1, 15);
	x5 = x2;
	x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + iw*16)));
	x4 = _mm_or_si128(x4, x5);
		
	x3 = _mm_and_si128(x3, x4);
	x3 = _mm_or_si128(x3, x6);
	x3 = _mm_and_si128(x3, x2);
	x3 = _mm_and_si128(x3, x0);
		
	_mm_store_si128((__m128i*)(ptr_sip), x3);
	_mm_store_si128((__m128i*)buf_ptr, x3);
	//---- loop_2 - prepare for first line - end ---------------------------------------------

	//---- loop_2 - main loop  ---------------------------------------------
	for (jh = 1; jh < h; jh++, ptr_sip_line += si_w) {
		buf_ptr = buffer;
		ptr_sip = ptr_sip_line;
		x2 = _mm_load_si128((__m128i*)(ptr_sip+si_w+ 0));
		x1 = _mm_slli_si128(x2, 15);
		for (iw = w; iw > 16; iw -= 16, ptr_sip += 16, buf_ptr += 16) {
			//horizontal filtering
			x0 = x1;
			x1 = x2;
			x2 = _mm_load_si128((__m128i*)(ptr_sip+si_w+16));
			
			x3 = _mm_alignr_epi8_simd(x1, x0, 15);
			x4 = _mm_alignr_epi8_simd(x2, x1, 1);
			
			x3 = _mm_and_si128(x3, x4);
			x3 = _mm_or_si128(x3, x6);
			x3 = _mm_and_si128(x3, x1);
			_mm_store_si128((__m128i*)(ptr_sip+si_w), x3);

			//vertical filtering
			x4 = _mm_load_si128((__m128i *)buf_ptr);
			x5 = _mm_load_si128((__m128i *)ptr_sip);

			x3 = _mm_and_si128(x3, x4);
			x3 = _mm_or_si128(x3, x6);
			x3 = _mm_and_si128(x3, x5);
			_mm_store_si128((__m128i*)ptr_sip, x3);
			_mm_store_si128((__m128i*)buf_ptr, x5);
		}
		//---- loop_2 - main loop - last edge --------------------------
		//horizontal filtering
		x0 = _mm_load_si128((__m128i*)(pqb_mask_a + iw*16));
		
		x4 = x2;
		x4 = _mm_and_si128(x4, x0);
		x4 = _mm_srli_si128(x4, 1);
		x3 = _mm_alignr_epi8_simd(x2, x1, 15);
		x5 = x2;
		x5 = _mm_and_si128(x5, _mm_load_si128((__m128i*)(pqb_mask_s + iw*16)));
		x4 = _mm_or_si128(x4, x5);
		
		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x2);
		x3 = _mm_and_si128(x3, x0);
		
		_mm_store_si128((__m128i*)(ptr_sip+si_w), x3);
		
		//vertical filtering
		x4 = _mm_load_si128((__m128i *)buf_ptr);
		x5 = _mm_load_si128((__m128i *)ptr_sip);

		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x5);
		_mm_store_si128((__m128i*)ptr_sip, x3);
		_mm_store_si128((__m128i*)buf_ptr, x5);
		//---- loop_2 - main loop - last edge end ------------------------
	}
	//---- loop_2 - main loop  - end  ------------------------------------------

	//---- loop_2 - last line ---------------------------------------------
	buf_ptr = buffer;
	ptr_sip = ptr_sip_line;
	for (iw = 0; iw < si_w; iw += 16, ptr_sip += 16, buf_ptr += 16) {
		//vertical filtering
		x4 = _mm_load_si128((__m128i *)buf_ptr);
		x5 = _mm_load_si128((__m128i *)ptr_sip);
		x3 = x5;
		
		x3 = _mm_and_si128(x3, x4);
		x3 = _mm_or_si128(x3, x6);
		x3 = _mm_and_si128(x3, x5);
		_mm_store_si128((__m128i*)ptr_sip, x3);
	}
	//---- loop_2 - last line - end ---------------------------------------------
	////// loop_2 - end  ////////////////////////////////////////////////////////////////////
}


#endif //ENABLE_FUNC_BASE
