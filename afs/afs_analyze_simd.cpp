#include <emmintrin.h> //SSE2
#include <Windows.h>
#include "filter.h"
#include "simd_util.h"
#define ENABLE_FUNC_BASE
#include "afs_analyze_simd.h"

//以下 高速化版+6までの旧コードと+7での高速化第1弾
/*
void __stdcall afs_analyze_12_sse2(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	for (int jw = 0; jw < 3; jw++) {
		BYTE *ptr_dst = (BYTE *)dst + jw*16;
		BYTE *ptr_p0 = (BYTE *)p0  + jw*16;
		BYTE *ptr_p1 = (BYTE *)p1  + jw*16;
		__m128i x0, x1, x2, x3, x4, x5, x6, x7;
		
		x1 = _mm_setzero_si128();
		x0 = _mm_loadu_si128((__m128i *)ptr_p0);
		x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1));
		x1 = _mm_cmpgt_epi16(x1, x0);
		x0 = _mm_xor_si128(x0, x1);
		x0 = _mm_subs_epi16(x0, x1);
		x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
		x2 = _mm_load_si128((__m128i *)pw_thre_shift);
		_mm_prefetch((char *)(ptr_p0 + step6), _MM_HINT_T0);
		_mm_prefetch((char *)(ptr_p1 + step6), _MM_HINT_T0);
		x3 = _mm_cmpgt_epi16(x3, x0);
		x2 = _mm_cmpgt_epi16(x2, x0);
		x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
		x3 = _mm_or_si128(x3, x2);
		_mm_storeu_si128((__m128i *)ptr_dst, x3);
		
		x4 = _mm_setzero_si128();
		x5 = _mm_setzero_si128();
		x6 = _mm_setzero_si128();
		x7 = _mm_setzero_si128();
		
		BYTE *ptr[2];
		for (int ih = 1; ih < h; ih++) {
			ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
			ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
			ptr_dst += 48;
			x1 = _mm_setzero_si128();
			x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step6));
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6)));
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
			x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
			x2 = _mm_load_si128((__m128i *)pw_thre_shift);
			x3 = _mm_cmpgt_epi16(x3, x0);
			x2 = _mm_cmpgt_epi16(x2, x0);
			x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
			x3 = _mm_or_si128(x3, x2);
			
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x2 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
			x2 = _mm_cmpgt_epi16(x2, x0);
			x0 = _mm_xor_si128(x0, x2);
			x0 = _mm_subs_epi16(x0, x2);
			x1 = x0;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_packs_epi16(x1, x2);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
			x0 = _mm_packs_epi16(x0, x2);
			x1 = _mm_unpacklo_epi8(x1, x0);
			x2 = _mm_xor_si128(x2, x7);
			x7 = _mm_xor_si128(x7, x2);
			x6 = _mm_and_si128(x6, x2);
			x6 = _mm_and_si128(x6, x1);
			x6 = _mm_subs_epi8(x6, x1);

			_mm_prefetch((char *)(ptr_p0 + (step6<<1)), _MM_HINT_T0);

			x0 = x6;
			x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
			x0 = _mm_srli_epi16(x0, 4);
			x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_0));
			x3 = _mm_or_si128(x3, x0);
			
			_mm_prefetch((char *)(ptr_p1 + (step6<<1)), _MM_HINT_T0);
			
			x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
			x2 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
			x2 = _mm_cmpgt_epi16(x2, x0);
			x0 = _mm_xor_si128(x0, x2);
			x0 = _mm_subs_epi16(x0, x2);
			x1 = x0;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_packs_epi16(x1, x2);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
			x0 = _mm_packs_epi16(x0, x2);
			x1 = _mm_unpacklo_epi8(x1, x0);
			x2 = _mm_xor_si128(x2, x5);
			x5 = _mm_xor_si128(x5, x2);
			x4 = _mm_and_si128(x4, x2);
			x4 = _mm_and_si128(x4, x1);
			x4 = _mm_subs_epi8(x4, x1);
			x0 = x4;
			x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
			x0 = _mm_srli_epi16(x0, 4);
			x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_1));
			x3 = _mm_or_si128(x3, x0);
			
			_mm_storeu_si128((__m128i *)ptr_dst, x3);
			
			ptr_p0 += step6;
			ptr_p1 += step6;
		}
	}
}

void __stdcall afs_analyze_1_sse2(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7; 
	x3 = _mm_load_si128((__m128i *)pw_thre_shift);
	for (int jw = 0; jw < 3; jw++) {
		BYTE *ptr_dst = (BYTE *)dst + 16 * jw;
		BYTE *ptr_p0 = (BYTE *)p0  + 16 * jw;
		BYTE *ptr_p1 = (BYTE *)p1  + 16 * jw;
		//afs_analyze_1_mmx_sub
		x1 = _mm_setzero_si128();
		x0 = _mm_loadu_si128((__m128i *)ptr_p0);
		x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
		_mm_prefetch((char *)(ptr_p0 + step6), _MM_HINT_T0);
		_mm_prefetch((char *)(ptr_p1 + step6), _MM_HINT_T0);
		x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
		x0 = _mm_xor_si128(x0, x1);
		x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
		x2 = x3;
		x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
		
		_mm_storeu_si128((__m128i *)ptr_dst, x2);
		
		x4 = _mm_setzero_si128();
		x5 = _mm_setzero_si128();
		x6 = _mm_setzero_si128();
		x7 = _mm_setzero_si128();
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
		BYTE *ptr[2];
		for (int ih = 1; ih < h; ih++) {
			ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
			ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
			//afs_analyze_1_mmx_loop
			//former field line
			ptr_dst += 48;
			//analyze motion
			x1 = _mm_setzero_si128();
			x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
			x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
			
			//analyze non-shift
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x1 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
			x0 = _mm_cmpgt_epi16(x0, x3);
			x1 = _mm_xor_si128(x1, x7);
			x7 = _mm_xor_si128(x7, x1);
			x6 = _mm_and_si128(x6, x1);
			x6 = _mm_and_si128(x6, x0);
			x6 = _mm_subs_epi16(x6, x0);

			_mm_prefetch((char *)(ptr_p0 + (step6<<1)), _MM_HINT_T0);

			x1 = x6;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_0));
			x2 = _mm_or_si128(x2, x1);

			_mm_prefetch((char *)(ptr_p1 + (step6<<1)), _MM_HINT_T0);
			
			//analyze shift
			x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
			x1 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
			x0 = _mm_cmpgt_epi16(x0, x3);
			x1 = _mm_xor_si128(x1, x5);
			x5 = _mm_xor_si128(x5, x1);
			x4 = _mm_and_si128(x4, x1);
			x4 = _mm_and_si128(x4, x0);
			x4 = _mm_subs_epi16(x4, x0);
			x1 = x4;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_1));
			x2 = _mm_or_si128(x2, x1);
			
			_mm_storeu_si128((__m128i *)ptr_dst, x2);
			
			ptr_p1 += step6;
			ptr_p0 += step6;
		}
	}	
}

void __stdcall afs_analyze_2_sse2(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7; 
	for (int jw = 0; jw < 3; jw++) {
		BYTE *ptr_dst = (BYTE *)dst + 16 * jw;
		BYTE *ptr_p0 = (BYTE *)p0  + 16 * jw;
		BYTE *ptr_p1 = (BYTE *)p1  + 16 * jw;
		x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
		//afs_analyze_2_mmx_sub
		x1 = _mm_setzero_si128();
		x0 = _mm_loadu_si128((__m128i *)ptr_p0);
		x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
		_mm_prefetch((char *)(ptr_p0 + step6), _MM_HINT_T0);
		_mm_prefetch((char *)(ptr_p1 + step6), _MM_HINT_T0);
		x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
		x0 = _mm_xor_si128(x0, x1);
		x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
		x2 = x3;
		x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
		
		_mm_storeu_si128((__m128i *)ptr_dst, x2);
		
		x4 = _mm_setzero_si128();
		x5 = _mm_setzero_si128();
		x6 = _mm_setzero_si128();
		x7 = _mm_setzero_si128();
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
		BYTE *ptr[2];
		for (int ih = 1; ih < h; ih++) {
			ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
			ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
			//afs_analyze_1_mmx_loop
			//former field line
			ptr_dst += 48;
			//analyze motion
			x1 = _mm_setzero_si128();
			x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
			x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
			
			//analyze non-shift
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x1 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_xor_si128(x1, x7);
			x7 = _mm_xor_si128(x7, x1);
			x6 = _mm_and_si128(x6, x1);
			x6 = _mm_and_si128(x6, x0);
			x6 = _mm_subs_epi16(x6, x0);

			_mm_prefetch((char *)(ptr_p0 + (step6<<1)), _MM_HINT_T0);

			x1 = x6;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_0));
			x2 = _mm_or_si128(x2, x1);

			_mm_prefetch((char *)(ptr_p1 + (step6<<1)), _MM_HINT_T0);
			
			//analyze shift
			x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
			x1 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_xor_si128(x1, x5);
			x5 = _mm_xor_si128(x5, x1);
			x4 = _mm_and_si128(x4, x1);
			x4 = _mm_and_si128(x4, x0);
			x4 = _mm_subs_epi16(x4, x0);
			x1 = x4;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_1));
			x2 = _mm_or_si128(x2, x1);
			
			_mm_storeu_si128((__m128i *)ptr_dst, x2);
			
			ptr_p1 += step6;
			ptr_p0 += step6;
		}
	}	
}

void __stdcall afs_analyze_12_sse2_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr;
	BYTE *ptr[2];
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE];
	buf_ptr = buffer;
	
	for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			x1 = _mm_setzero_si128();
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1));
			x1 = _mm_cmpgt_epi16(x1, x0);
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1);
			x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
			x2 = _mm_load_si128((__m128i *)pw_thre_shift);
			x3 = _mm_cmpgt_epi16(x3, x0);
			x2 = _mm_cmpgt_epi16(x2, x0);
			x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
			x3 = _mm_or_si128(x3, x2);
			_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x3);
		}
	}
	dst += 8;

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
	for (int ih = 1; ih < h; ih++, dst += 8, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				x1 = _mm_setzero_si128();
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6)));
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
				x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
				x2 = _mm_load_si128((__m128i *)pw_thre_shift);
				x3 = _mm_cmpgt_epi16(x3, x0);
				x2 = _mm_cmpgt_epi16(x2, x0);
				x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
				x3 = _mm_or_si128(x3, x2);
				
				x0 = _mm_loadu_si128((__m128i *)ptr_p0);
				x2 = _mm_setzero_si128();
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
				x2 = _mm_cmpgt_epi16(x2, x0);
				x0 = _mm_xor_si128(x0, x2);
				x0 = _mm_subs_epi16(x0, x2);
				x1 = x0;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
				x1 = _mm_packs_epi16(x1, x2);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
				x0 = _mm_packs_epi16(x0, x2);
				x1 = _mm_unpacklo_epi8(x1, x0);
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x2 = _mm_xor_si128(x2, x7);
				x7 = _mm_xor_si128(x7, x2);
				x6 = _mm_and_si128(x6, x2);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_subs_epi8(x6, x1);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);

				x0 = x6;
				x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
				x0 = _mm_srli_epi16(x0, 4);
				x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_0));
				x3 = _mm_or_si128(x3, x0);
				
				x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x2 = _mm_setzero_si128();
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
				x2 = _mm_cmpgt_epi16(x2, x0);
				x0 = _mm_xor_si128(x0, x2);
				x0 = _mm_subs_epi16(x0, x2);
				x1 = x0;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
				x1 = _mm_packs_epi16(x1, x2);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
				x0 = _mm_packs_epi16(x0, x2);
				x1 = _mm_unpacklo_epi8(x1, x0);
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x2 = _mm_xor_si128(x2, x5);
				x5 = _mm_xor_si128(x5, x2);
				x4 = _mm_and_si128(x4, x2);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_subs_epi8(x4, x1);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				x0 = x4;
				x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
				x0 = _mm_srli_epi16(x0, 4);
				x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_1));
				x3 = _mm_or_si128(x3, x0);
				
				_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x3);
			}
		}
	}
}

void __stdcall afs_analyze_1_sse2_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr;
	BYTE *ptr[2];
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE];
	buf_ptr = buffer;
	
	x3 = _mm_load_si128((__m128i *)pw_thre_shift);
	for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			//afs_analyze_1_mmx_sub
			x1 = _mm_setzero_si128();
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
			x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
			
			_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
		}
	}
	dst += 8;

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
	for (int ih = 1; ih < h; ih++, dst += 8, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				//afs_analyze_1_mmx_loop
				//former field line
				//analyze motion
				x1 = _mm_setzero_si128();
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
				x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
				x2 = x3;
				x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
				
				//analyze non-shift
				x0 = _mm_loadu_si128((__m128i *)ptr_p0);
				x1 = _mm_setzero_si128();
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
				x0 = _mm_cmpgt_epi16(x0, x3);
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x7);
				x7 = _mm_xor_si128(x7, x1);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_and_si128(x6, x0);
				x6 = _mm_subs_epi16(x6, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);


				x1 = x6;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_0));
				x2 = _mm_or_si128(x2, x1);

				
				//analyze shift
				x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x1 = _mm_setzero_si128();
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
				x0 = _mm_cmpgt_epi16(x0, x3);
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x5);
				x5 = _mm_xor_si128(x5, x1);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_and_si128(x4, x0);
				x4 = _mm_subs_epi16(x4, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				
				x1 = x4;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_1));
				x2 = _mm_or_si128(x2, x1);
				
				_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
			}
		}
	}
}

void __stdcall afs_analyze_2_sse2_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr;
	BYTE *ptr[2];
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE];
	buf_ptr = buffer;

	for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
			//afs_analyze_2_mmx_sub
			x1 = _mm_setzero_si128();
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
			x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
			x0 = _mm_xor_si128(x0, x1);
			x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
			
			_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
		}
	}
	dst += 8;

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
	for (int ih = 1; ih < h; ih++, dst += 8, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				//afs_analyze_1_mmx_loop
				//former field line
				//analyze motion
				x1 = _mm_setzero_si128();
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
				x1 = _mm_cmpgt_epi16(x1, x0); //x1 = sign(*p0 - *p1) = (0 > mm0)
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1); //x0 = abs(*p0 - *p1)
				x2 = x3;
				x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
				
				//analyze non-shift
				x0 = _mm_loadu_si128((__m128i *)ptr_p0);
				x1 = _mm_setzero_si128();
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x7);
				x7 = _mm_xor_si128(x7, x1);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_and_si128(x6, x0);
				x6 = _mm_subs_epi16(x6, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);
				x1 = x6;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_0));
				x2 = _mm_or_si128(x2, x1);
				
				//analyze shift
				x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x1 = _mm_setzero_si128();
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
				x1 = _mm_cmpgt_epi16(x1, x0);
				x0 = _mm_xor_si128(x0, x1);
				x0 = _mm_subs_epi16(x0, x1);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x5);
				x5 = _mm_xor_si128(x5, x1);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_and_si128(x4, x0);
				x4 = _mm_subs_epi16(x4, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				x1 = x4;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_1));
				x2 = _mm_or_si128(x2, x1);
				
				_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
			}
		}
	}	
}

#include <tmmintrin.h> //SSSE3

void __stdcall afs_analyze_12_ssse3(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	for (int jw = 0; jw < 3; jw++) {
		BYTE *ptr_dst = (BYTE *)dst + jw*16;
		BYTE *ptr_p0 = (BYTE *)p0  + jw*16;
		BYTE *ptr_p1 = (BYTE *)p1  + jw*16;
		__m128i x0, x1, x2, x3, x4, x5, x6, x7;
		
		x0 = _mm_loadu_si128((__m128i *)ptr_p0);
		x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1));
		x0 = _mm_abs_epi16(x0);
		x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
		x2 = _mm_load_si128((__m128i *)pw_thre_shift);
		_mm_prefetch((char *)(ptr_p0 + step6), _MM_HINT_T0);
		_mm_prefetch((char *)(ptr_p1 + step6), _MM_HINT_T0);
		x3 = _mm_cmpgt_epi16(x3, x0);
		x2 = _mm_cmpgt_epi16(x2, x0);
		x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
		x3 = _mm_or_si128(x3, x2);
		_mm_storeu_si128((__m128i *)ptr_dst, x3);
		
		x4 = _mm_setzero_si128();
		x5 = _mm_setzero_si128();
		x6 = _mm_setzero_si128();
		x7 = _mm_setzero_si128();
		
		BYTE *ptr[2];
		for (int ih = 1; ih < h; ih++) {
			ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
			ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
			ptr_dst += 48;
			x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step6));
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6)));
			x0 = _mm_abs_epi16(x0);
			x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
			x2 = _mm_load_si128((__m128i *)pw_thre_shift);
			x3 = _mm_cmpgt_epi16(x3, x0);
			x2 = _mm_cmpgt_epi16(x2, x0);
			x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
			x3 = _mm_or_si128(x3, x2);
			
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x2 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
			x2 = _mm_cmpgt_epi16(x2, x0);
			x0 = _mm_xor_si128(x0, x2);
			x0 = _mm_subs_epi16(x0, x2);
			x1 = x0;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_packs_epi16(x1, x2);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
			x0 = _mm_packs_epi16(x0, x2);
			x1 = _mm_unpacklo_epi8(x1, x0);
			x2 = _mm_xor_si128(x2, x7);
			x7 = _mm_xor_si128(x7, x2);
			x6 = _mm_and_si128(x6, x2);
			x6 = _mm_and_si128(x6, x1);
			x6 = _mm_subs_epi8(x6, x1);

			_mm_prefetch((char *)(ptr_p0 + (step6<<1)), _MM_HINT_T0);

			x0 = x6;
			x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
			x0 = _mm_srli_epi16(x0, 4);
			x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_0));
			x3 = _mm_or_si128(x3, x0);
			
			_mm_prefetch((char *)(ptr_p1 + (step6<<1)), _MM_HINT_T0);
			
			x0 = _mm_loadu_si128((__m128i *)(ptr[0]));
			x2 = _mm_setzero_si128();
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
			x2 = _mm_cmpgt_epi16(x2, x0);
			x0 = _mm_xor_si128(x0, x2);
			x0 = _mm_subs_epi16(x0, x2);
			x1 = x0;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_packs_epi16(x1, x2);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
			x0 = _mm_packs_epi16(x0, x2);
			x1 = _mm_unpacklo_epi8(x1, x0);
			x2 = _mm_xor_si128(x2, x5);
			x5 = _mm_xor_si128(x5, x2);
			x4 = _mm_and_si128(x4, x2);
			x4 = _mm_and_si128(x4, x1);
			x4 = _mm_subs_epi8(x4, x1);
			x0 = x4;
			x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
			x0 = _mm_srli_epi16(x0, 4);
			x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_1));
			x3 = _mm_or_si128(x3, x0);
			
			_mm_storeu_si128((__m128i *)ptr_dst, x3);
			
			ptr_p0 += step6;
			ptr_p1 += step6;
		}
	}
}

void __stdcall afs_analyze_12_ssse3_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr;
	BYTE *ptr[2];
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE];
	buf_ptr = buffer;

	for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1));
			x0 = _mm_abs_epi16(x0);
			x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
			x2 = _mm_load_si128((__m128i *)pw_thre_shift);
			x3 = _mm_cmpgt_epi16(x3, x0);
			x2 = _mm_cmpgt_epi16(x2, x0);
			x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
			x3 = _mm_or_si128(x3, x2);
			_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x3);
		}
	}
	dst += 8;

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
	
	for (int ih = 1; ih < h; ih++, dst += 8, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0 + step6));
				x1 = x0;
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6)));
				x0 = _mm_abs_epi16(x0);
				x3 = _mm_load_si128((__m128i *)pw_thre_motion[jw]);
				x2 = _mm_load_si128((__m128i *)pw_thre_shift);
				x3 = _mm_cmpgt_epi16(x3, x0);
				x2 = _mm_cmpgt_epi16(x2, x0);
				x3 = _mm_and_si128(x3, _mm_load_si128((__m128i *)pw_mask_2motion_0));
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0));
				x3 = _mm_or_si128(x3, x2);
			
				x2 = _mm_loadu_si128((__m128i *)ptr_p0);
				x2 = _mm_subs_epi16(x2, x1);
				x0 = _mm_abs_epi16(x2);
				x2 = _mm_cmpeq_epi16(x2, x0);
				x1 = x0;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
				x1 = _mm_packs_epi16(x1, x1);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
				x0 = _mm_packs_epi16(x0, x0);
				x1 = _mm_unpacklo_epi8(x1, x0);
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x2 = _mm_xor_si128(x2, x7);
				x7 = _mm_xor_si128(x7, x2);
				x6 = _mm_and_si128(x6, x2);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_subs_epi8(x6, x1);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);

				x0 = x6;
				x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
				x0 = _mm_srli_epi16(x0, 4);
				x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_0));
				x3 = _mm_or_si128(x3, x0);
			
				x2 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x2 = _mm_subs_epi16(x2, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
				x0 = _mm_abs_epi16(x2);
				x2 = _mm_cmpeq_epi16(x2, x0);
				x1 = x0;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_deint));
				x1 = _mm_packs_epi16(x1, x1);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_shift));
				x0 = _mm_packs_epi16(x0, x0);
				x1 = _mm_unpacklo_epi8(x1, x0);
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x2 = _mm_xor_si128(x2, x5);
				x5 = _mm_xor_si128(x5, x2);
				x4 = _mm_and_si128(x4, x2);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_subs_epi8(x4, x1);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				x0 = x4;
				x0 = _mm_cmpgt_epi8(x0, _mm_load_si128((__m128i *)pb_thre_count));
				x0 = _mm_srli_epi16(x0, 4);
				x0 = _mm_and_si128(x0, _mm_load_si128((__m128i *)pw_mask_12stripe_1));
				x3 = _mm_or_si128(x3, x0);
			
				_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x3);
			}
		}
	}
}

void __stdcall afs_analyze_1_ssse3(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7; 
	x3 = _mm_load_si128((__m128i *)pw_thre_shift);
	for (int jw = 0; jw < 3; jw++) {
		BYTE *ptr_dst = (BYTE *)dst + 16 * jw;
		BYTE *ptr_p0 = (BYTE *)p0  + 16 * jw;
		BYTE *ptr_p1 = (BYTE *)p1  + 16 * jw;
		//afs_analyze_1_mmx_sub
		x0 = _mm_loadu_si128((__m128i *)ptr_p0);
		x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
		_mm_prefetch((char *)(ptr_p0 + step6), _MM_HINT_T0);
		_mm_prefetch((char *)(ptr_p1 + step6), _MM_HINT_T0);
		x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
		x2 = x3;
		x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
		
		_mm_storeu_si128((__m128i *)ptr_dst, x2);
		
		x4 = _mm_setzero_si128();
		x5 = _mm_setzero_si128();
		x6 = _mm_setzero_si128();
		x7 = _mm_setzero_si128();
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
		BYTE *ptr[2];
		for (int ih = 1; ih < h; ih++) {
			ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
			ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
			//afs_analyze_1_mmx_loop
			//former field line
			ptr_dst += 48;
			//analyze motion
			x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
			x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
			
			//analyze non-shift
			x1 = _mm_loadu_si128((__m128i *)ptr_p0);
			x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
			x0 = _mm_abs_epi16(x1);
			x1 = _mm_cmpeq_epi16(x1, x0);
			x0 = _mm_cmpgt_epi16(x0, x3);
			x1 = _mm_xor_si128(x1, x7);
			x7 = _mm_xor_si128(x7, x1);
			x6 = _mm_and_si128(x6, x1);
			x6 = _mm_and_si128(x6, x0);
			x6 = _mm_subs_epi16(x6, x0);

			_mm_prefetch((char *)(ptr_p0 + (step6<<1)), _MM_HINT_T0);

			x1 = x6;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_0));
			x2 = _mm_or_si128(x2, x1);

			_mm_prefetch((char *)(ptr_p1 + (step6<<1)), _MM_HINT_T0);
			
			//analyze shift
			x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
			x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
			x0 = _mm_abs_epi16(x1);
			x1 = _mm_cmpeq_epi16(x1, x0);
			x0 = _mm_cmpgt_epi16(x0, x3);
			x1 = _mm_xor_si128(x1, x5);
			x5 = _mm_xor_si128(x5, x1);
			x4 = _mm_and_si128(x4, x1);
			x4 = _mm_and_si128(x4, x0);
			x4 = _mm_subs_epi16(x4, x0);
			x1 = x4;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_1));
			x2 = _mm_or_si128(x2, x1);
			
			_mm_storeu_si128((__m128i *)ptr_dst, x2);
			
			ptr_p1 += step6;
			ptr_p0 += step6;
		}
	}	
}

void __stdcall afs_analyze_2_ssse3(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7; 
	for (int jw = 0; jw < 3; jw++) {
		BYTE *ptr_dst = (BYTE *)dst + 16 * jw;
		BYTE *ptr_p0 = (BYTE *)p0  + 16 * jw;
		BYTE *ptr_p1 = (BYTE *)p1  + 16 * jw;
		x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
		//afs_analyze_2_mmx_sub
		x0 = _mm_loadu_si128((__m128i *)ptr_p0);
		x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
		_mm_prefetch((char *)(ptr_p0 + step6), _MM_HINT_T0);
		_mm_prefetch((char *)(ptr_p1 + step6), _MM_HINT_T0);
		x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
		x2 = x3;
		x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
		x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
		
		_mm_storeu_si128((__m128i *)ptr_dst, x2);
		
		x4 = _mm_setzero_si128();
		x5 = _mm_setzero_si128();
		x6 = _mm_setzero_si128();
		x7 = _mm_setzero_si128();
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
		BYTE *ptr[2];
		for (int ih = 1; ih < h; ih++) {
			ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
			ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
			//afs_analyze_1_mmx_loop
			//former field line
			ptr_dst += 48;
			//analyze motion
			x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
			x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
			
			//analyze non-shift
			x1 = _mm_loadu_si128((__m128i *)ptr_p0);
			x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
			x0 = _mm_abs_epi16(x1);
			x1 = _mm_cmpeq_epi16(x1, x0);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_xor_si128(x1, x7);
			x7 = _mm_xor_si128(x7, x1);
			x6 = _mm_and_si128(x6, x1);
			x6 = _mm_and_si128(x6, x0);
			x6 = _mm_subs_epi16(x6, x0);

			_mm_prefetch((char *)(ptr_p0 + (step6<<1)), _MM_HINT_T0);

			x1 = x6;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_0));
			x2 = _mm_or_si128(x2, x1);

			_mm_prefetch((char *)(ptr_p1 + (step6<<1)), _MM_HINT_T0);
			
			//analyze shift
			x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
			x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
			x0 = _mm_abs_epi16(x1);
			x1 = _mm_cmpeq_epi16(x1, x0);
			x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
			x1 = _mm_xor_si128(x1, x5);
			x5 = _mm_xor_si128(x5, x1);
			x4 = _mm_and_si128(x4, x1);
			x4 = _mm_and_si128(x4, x0);
			x4 = _mm_subs_epi16(x4, x0);
			x1 = x4;
			x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
			x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_1));
			x2 = _mm_or_si128(x2, x1);
			
			_mm_storeu_si128((__m128i *)ptr_dst, x2);
			
			ptr_p1 += step6;
			ptr_p0 += step6;
		}
	}	
}

void __stdcall afs_analyze_1_ssse3_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr;
	BYTE *ptr[2];
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE];
	buf_ptr = buffer;
	
	x3 = _mm_load_si128((__m128i *)pw_thre_shift);
	for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			//afs_analyze_1_mmx_sub
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
			x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
			
			_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
		}
	}
	dst += 8;

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
	for (int ih = 1; ih < h; ih++, dst += 8, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				//afs_analyze_1_mmx_loop
				//former field line
				//analyze motion
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
				x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
				x2 = x3;
				x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_1motion_0)); //x2 &= 4000h
				
				//analyze non-shift
				x1 = _mm_loadu_si128((__m128i *)ptr_p0);
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
				x0 = _mm_cmpgt_epi16(x0, x3);
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x7);
				x7 = _mm_xor_si128(x7, x1);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_and_si128(x6, x0);
				x6 = _mm_subs_epi16(x6, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);


				x1 = x6;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_0));
				x2 = _mm_or_si128(x2, x1);

				
				//analyze shift
				x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
				x0 = _mm_cmpgt_epi16(x0, x3);
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x5);
				x5 = _mm_xor_si128(x5, x1);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_and_si128(x4, x0);
				x4 = _mm_subs_epi16(x4, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				
				x1 = x4;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count1));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_1stripe_1));
				x2 = _mm_or_si128(x2, x1);
				
				_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
			}
		}
	}
}

void __stdcall afs_analyze_2_ssse3_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	const int step6 = step * 6;
	const int width6 = width * 6;
	const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 4;
	__m128i x0, x1, x2, x3, x4, x5, x6, x7;
	BYTE *buf_ptr;
	BYTE *ptr[2];
	
	BYTE *ptr_dst = (BYTE *)dst;
	BYTE *ptr_p0 = (BYTE *)p0;
	BYTE *ptr_p1 = (BYTE *)p1;
	BYTE __declspec(align(16)) buffer[BUFFER_SIZE];
	buf_ptr = buffer;

	for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
		for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16) {
			x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
			//afs_analyze_2_mmx_sub
			x0 = _mm_loadu_si128((__m128i *)ptr_p0);
			x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)ptr_p1)); //x0 = *p0 - *p1
			x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
			x2 = x3;
			x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_motion > abs(*p0 - *p1))
			x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
			
			_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
		}
	}
	dst += 8;

	for (BYTE *buf_fin = buffer + width6; buf_ptr < buf_fin; buf_ptr += 32) {
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 +  0), _mm_setzero_si128());
		_mm_store_si128((__m128i*)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6 + 16), _mm_setzero_si128());
	}
		
 // if(abs_01diff < thre_motion) flag |= motion;
 // (new_sign, abs_diff) <= last - *p;
 // last = *p;
 // count_add = (new_sign ^ sign < 0);
 // sign = new_sign;
 // if(abs_diff < thre_shift/deint) count = 0;
 // count += count_add;
 // if(count >= thre_count) flag |= stripe;
		
	for (int ih = 1; ih < h; ih++, dst += 8, p0 += step, p1 += step) {
		ptr_dst = (BYTE *)dst;
		ptr_p0 = (BYTE *)p0;
		ptr_p1 = (BYTE *)p1;
		buf_ptr = buffer;
		for (int kw = 0; kw < width6; kw += 48, ptr_dst += 48 * h) {
			for (int jw = 0; jw < 3; jw++, ptr_p0 += 16, ptr_p1 += 16, buf_ptr += 16) {
				x3 = _mm_load_si128((__m128i *)(pw_thre_motion[jw]));
				ptr[((tb_order == 0) + ih + 0) & 0x01] = ptr_p1;
				ptr[((tb_order == 0) + ih + 1) & 0x01] = ptr_p0;
				//afs_analyze_1_mmx_loop
				//former field line
				//analyze motion
				x0 = _mm_loadu_si128((__m128i *)(ptr_p0+step6));
				x0 = _mm_subs_epi16(x0, _mm_loadu_si128((__m128i *)(ptr_p1+step6))); //x0 = *p0 - *p1
				x0 = _mm_abs_epi16(x0); //x0 = abs(*p0 - *p1)
				x2 = x3;
				x2 = _mm_cmpgt_epi16(x2, x0); //x2 = (thre_shift > abs(*p0 - *p1))
				x2 = _mm_and_si128(x2, _mm_load_si128((__m128i *)pw_mask_2motion_0)); //x2 &= 4000h
				
				//analyze non-shift
				x1 = _mm_loadu_si128((__m128i *)ptr_p0);
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr_p0+step6)));
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
				x7 = _mm_load_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6));
				x6 = _mm_load_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x7);
				x7 = _mm_xor_si128(x7, x1);
				x6 = _mm_and_si128(x6, x1);
				x6 = _mm_and_si128(x6, x0);
				x6 = _mm_subs_epi16(x6, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 0 * BLOCK_SIZE_YCP * 6), x7);
				_mm_store_si128((__m128i *)(buf_ptr + 1 * BLOCK_SIZE_YCP * 6), x6);
				x1 = x6;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_0));
				x2 = _mm_or_si128(x2, x1);
				
				//analyze shift
				x1 = _mm_loadu_si128((__m128i *)(ptr[0]));
				x1 = _mm_subs_epi16(x1, _mm_loadu_si128((__m128i *)(ptr[1]+step6)));
				x0 = _mm_abs_epi16(x1);
				x1 = _mm_cmpeq_epi16(x1, x0);
				x0 = _mm_cmpgt_epi16(x0, _mm_load_si128((__m128i *)pw_thre_deint));
				x5 = _mm_load_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6));
				x4 = _mm_load_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6));
				x1 = _mm_xor_si128(x1, x5);
				x5 = _mm_xor_si128(x5, x1);
				x4 = _mm_and_si128(x4, x1);
				x4 = _mm_and_si128(x4, x0);
				x4 = _mm_subs_epi16(x4, x0);
				_mm_store_si128((__m128i *)(buf_ptr + 2 * BLOCK_SIZE_YCP * 6), x5);
				_mm_store_si128((__m128i *)(buf_ptr + 3 * BLOCK_SIZE_YCP * 6), x4);
				x1 = x4;
				x1 = _mm_cmpgt_epi16(x1, _mm_load_si128((__m128i *)pw_thre_count2));
				x1 = _mm_and_si128(x1, _mm_load_si128((__m128i *)pw_mask_2stripe_1));
				x2 = _mm_or_si128(x2, x1);
				
				_mm_storeu_si128((__m128i *)(ptr_dst + jw*16), x2);
			}
		}
	}	
}
void __stdcall afs_analyze_shrink_info_sse2(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_simd(dst, src, h, width, si_pitch, SSE2);
}
void __stdcall afs_analyze_shrink_info_ssse3(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_simd(dst, src, h, width, si_pitch, SSSE3|SSE2);
}
void __stdcall afs_analyze_shrink_info_sse4_1(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_simd(dst, src, h, width, si_pitch, SSE41|SSSE3|SSE2);
}
void __stdcall afs_analyze_shrink_info_sse2_plus(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_simd_plus(dst, src, h, width, si_pitch, SSE2);
}
void __stdcall afs_analyze_shrink_info_ssse3_plus(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_simd_plus(dst, src, h, width, si_pitch, SSSE3|SSE2);
}
void __stdcall afs_analyze_shrink_info_sse4_1_plus(BYTE *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_simd_plus(dst, src, h, width, si_pitch, SSE41|SSSE3|SSE2);
}
*/
