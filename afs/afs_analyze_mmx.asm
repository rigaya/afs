.586
.mmx
.model flat

_TEXT64 segment page public use32 'CODE'
_TEXT64 ends

_DATA64 segment page public use32 'DATA'

pw_thre_shift		dq	00000000000000000h
pw_thre_deint		dq	00000000000000000h
pw_thre_motion		dq	00000000000000000h
pw_thre_motion1		dq	00000000000000000h
pw_thre_motion2		dq	00000000000000000h
pw_thre_motion3		dq	00000000000000000h

_DATA64 ends

CONST segment dword use32 public 'CONST'
CONST ends

_BSS segment dword use32 public 'BSS'
_BSS ends

_TEXT64 segment
	align 16

pb_thre_count		dq	00302030203020302h
pw_thre_count2		dq	00002000200020002h
pw_thre_count1		dq	00003000300030003h
pw_mask_2stripe_0	dq	00001000100010001h
pw_mask_2stripe_1	dq	00002000200020002h
pw_mask_1stripe_0	dq	00010001000100010h
pw_mask_1stripe_1	dq	00020002000200020h
pw_mask_12stripe_0	dq	00011001100110011h
pw_mask_12stripe_1	dq	00022002200220022h
pw_mask_2motion_0	dq	00400040004000400h
pw_mask_1motion_0	dq	04000400040004000h

pb_mask_1stripe_01	dq	00000000030303030h
pb_mask_12stripe_01	dq	00000000033333333h
pb_mask_12motion_01	dq	000000000cccccccch
pw_mask_12stripe_01	dq	00033003300330033h
pw_mask_12motion_01	dq	000cc00cc00cc00cch
pw_mask_lowbyte		dq	000ff00ff00ff00ffh
pw_mask_ffff		dq	0000000000000ffffh

PUBLIC C _afs_analyze_set_threshold_mmx@16

;void __stdcall afs_analyze_set_threshold_mmx(
;  [esp+04] int            thre_shift
;  [esp+08] int            thre_deint
;  [esp+12] int            thre_Ymotion
;  [esp+16] int            thre_Cmotion
;)

_afs_analyze_set_threshold_mmx@16 PROC
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+16
		
		mov			eax, [esp+16+4]				; [thre_shift]
		lea			edx, [pw_thre_shift]
		mov			[edx  ], ax
		mov			[edx+2], ax
		mov			[edx+4], ax
		mov			[edx+6], ax

		mov			eax, [esp+16+8]				; [thre_deint]
		lea			edx, [pw_thre_deint]
		mov			[edx  ], ax
		mov			[edx+2], ax
		mov			[edx+4], ax
		mov			[edx+6], ax

		mov			eax, [esp+16+12]			; [thre_Ymotion]
		mov			ebx, [esp+16+16]			; [thre_Cmotion]
		lea			edx, [pw_thre_motion1]
		mov			[edx  ], ax
		mov			[edx+2], bx
		mov			[edx+4], bx
		mov			[edx+6], ax
		lea			edx, [pw_thre_motion2]
		mov			[edx  ], bx
		mov			[edx+2], bx
		mov			[edx+4], ax
		mov			[edx+6], bx
		lea			edx, [pw_thre_motion3]
		mov			[edx  ], bx
		mov			[edx+2], ax
		mov			[edx+4], bx
		mov			[edx+6], bx

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		ret			16

_afs_analyze_set_threshold_mmx@16 ENDP


PUBLIC C _afs_analyze_12_mmx@32

;void __stdcall afs_analyze_12_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *p0
;  [esp+12] PIXEL_YC       *p1
;  [esp+16] int            tb_order
;  [esp+20] int            step
;  [esp+24] int            h
;  [esp+28] void           *scan
;  [esp+32] int            *motion_count
;)

_afs_analyze_12_mmx@32 PROC
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+24

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			eax, [esp+24+16]			; [tb_order]
		mov			ebx, [esp+24+20]			; [step]
		mov			ecx, [esp+24+24]			; [h]
		lea			ebx, [ebx+ebx*2]
		lea			ebx, [ebx+ebx]				; ebx = step * 6
		movq		mm0, pw_thre_motion1
		movq		pw_thre_motion, mm0
		call		afs_analyze_12_mmx_sub

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			ecx, [esp+24+24]			; [h]
		lea			edi, [edi+8]
		lea			esi, [esi+8]
		lea			edx, [edx+8]
		movq		mm0, pw_thre_motion2
		movq		pw_thre_motion, mm0
		call		afs_analyze_12_mmx_sub

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			ecx, [esp+24+24]			; [h]
		lea			edi, [edi+16]
		lea			esi, [esi+16]
		lea			edx, [edx+16]
		movq		mm0, pw_thre_motion3
		movq		pw_thre_motion, mm0
		call		afs_analyze_12_mmx_sub

; exit
afs_analyze_12_mmx_exit:
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi

		ret			32

afs_analyze_12_mmx_sub:

; top line
; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi]
		psubsw		mm0, [edx]					; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm3, pw_thre_motion
		movq		mm2, pw_thre_shift
		pcmpgtw		mm3, mm0					; mm3 = (thre_motion > abs(*p0 - *p1))
		pcmpgtw		mm2, mm0					; mm2 = (thre_shift  > abs(*p0 - *p1))
		pand		mm3, pw_mask_2motion_0		; mm3 &= 0400h
		pand		mm2, pw_mask_1motion_0		; mm2 &= 4000h
		por			mm3, mm2

		movq		[edi], mm3

		pxor		mm4, mm4					; count1 = 0 (pb / pw=3,4)
		pxor		mm5, mm5					; sign1  = 0 (pb / pw=0,1)
		pxor		mm6, mm6					; count0 = 0 (pb / pw=3,4)
		pxor		mm7, mm7					; sign0  = 0 (pb / pw=0,1)

		lea			ecx, [ecx-1]
		test		eax, 1
		jne			afs_analyze_12_mmx_latter

;  if(abs_01diff < thre_motion) flag |= motion;
;  (new_sign, abs_diff) <= last - *p;
;  last = *p;
;  count_add = (new_sign ^ sign < 0);
;  sign = new_sign;
;  if(abs_diff < thre_shift/deint) count = 0;
;  count += count_add;
;  if(count >= thre_count) flag |= stripe;

afs_analyze_12_mmx_loop:

; former field line
		lea			edi, [edi+24]

; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi+ebx]
		psubsw		mm0, [edx+ebx]				; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm3, pw_thre_motion
		movq		mm2, pw_thre_shift
		pcmpgtw		mm3, mm0					; mm3 = (thre_motion > abs(*p0 - *p1))
		pcmpgtw		mm2, mm0					; mm2 = (thre_shift  > abs(*p0 - *p1))
		pand		mm3, pw_mask_2motion_0		; mm3 &= 0400h
		pand		mm2, pw_mask_1motion_0		; mm2 &= 4000h
		por			mm3, mm2

; - analyze non-shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm2, mm2
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm2, mm0					; mm2 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm2
		psubsw		mm0, mm2					; mm0 = abs(last - *p)
		movq		mm1, mm0
		pcmpgtw		mm1, pw_thre_deint			; mm1 = (abs(last - *p) > thre_deint)
		packsswb	mm1, mm2
		pcmpgtw		mm0, pw_thre_shift			; mm0 = (abs(last - *p) > thre_shift)
		packsswb	mm0, mm2
		punpcklbw	mm1, mm0					; mm1 = count_sub (deint=low, shift=high)
		pxor		mm2, mm7					; mm2 = sign(last - *p) ^ sign0
		pxor		mm7, mm2					; sign0 = sign(last - *p)
		pand		mm6, mm2					; count0 &= mm2
		pand		mm6, mm1					; count0 &= mm1
		psubsb		mm6, mm1					; count0 -= count_sub
		movq		mm0, mm6
		pcmpgtb		mm0, pb_thre_count			; mm0 = (count0 > (thre_count-1))
		psrlw		mm0, 4
		pand		mm0, pw_mask_12stripe_0
		por			mm3, mm0

; - analyze shift
		movq		mm0, [edx]					; mm0 = last = *(p1-step)
		pxor		mm2, mm2
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm2, mm0					; mm2 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm2
		psubsw		mm0, mm2					; mm0 = abs(last - *p)
		movq		mm1, mm0
		pcmpgtw		mm1, pw_thre_deint			; mm1 = (abs(last - *p) > thre_deint)
		packsswb	mm1, mm2
		pcmpgtw		mm0, pw_thre_shift			; mm0 = (abs(last - *p) > thre_shift)
		packsswb	mm0, mm2
		punpcklbw	mm1, mm0					; mm1 = count_sub (deint=low, shift=high)
		pxor		mm2, mm5					; mm2 = sign(last - *p) ^ sign1
		pxor		mm5, mm2					; sign1 = sign(last - *p)
		pand		mm4, mm2					; count1 &= mm2
		pand		mm4, mm1					; count1 &= mm1
		psubsb		mm4, mm1					; count1 -= count_sub
		movq		mm0, mm4
		pcmpgtb		mm0, pb_thre_count			; mm0 = (count1 > (thre_count-1))
		psrlw		mm0, 4
		pand		mm0, pw_mask_12stripe_1
		por			mm3, mm0

		movq		[edi], mm3

		lea			esi, [esi+ebx]
		lea			edx, [edx+ebx]
		dec			ecx
		jne			afs_analyze_12_mmx_latter
		ret

afs_analyze_12_mmx_latter:

; latter field line
		lea			edi, [edi+24]

; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi+ebx]
		psubsw		mm0, [edx+ebx]				; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm3, pw_thre_motion
		movq		mm2, pw_thre_shift
		pcmpgtw		mm3, mm0					; mm3 = (thre_motion > abs(*p0 - *p1))
		pcmpgtw		mm2, mm0					; mm2 = (thre_shift  > abs(*p0 - *p1))
		pand		mm3, pw_mask_2motion_0		; mm3 &= 0400h
		pand		mm2, pw_mask_1motion_0		; mm2 &= 4000h
		por			mm3, mm2

; - analyze non-shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm2, mm2
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm2, mm0					; mm2 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm2
		psubsw		mm0, mm2					; mm0 = abs(last - *p)
		movq		mm1, mm0
		pcmpgtw		mm1, pw_thre_deint			; mm1 = (abs(last - *p) > thre_deint)
		packsswb	mm1, mm2
		pcmpgtw		mm0, pw_thre_shift			; mm0 = (abs(last - *p) > thre_shift)
		packsswb	mm0, mm2
		punpcklbw	mm1, mm0					; mm1 = count_sub (deint=low, shift=high)
		pxor		mm2, mm7					; mm2 = sign(last - *p) ^ sign0
		pxor		mm7, mm2					; sign0 = sign(last - *p)
		pand		mm6, mm2					; count0 &= mm2
		pand		mm6, mm1					; count0 &= mm1
		psubsb		mm6, mm1					; count0 -= count_sub
		movq		mm0, mm6
		pcmpgtb		mm0, pb_thre_count			; mm0 = (count0 > (thre_count-1))
		psrlw		mm0, 4
		pand		mm0, pw_mask_12stripe_0
		por			mm3, mm0

; - analyze shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm2, mm2
		psubsw		mm0, [edx+ebx]				; mm0 = last - *p1
		pcmpgtw		mm2, mm0					; mm2 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm2
		psubsw		mm0, mm2					; mm0 = abs(last - *p)
		movq		mm1, mm0
		pcmpgtw		mm1, pw_thre_deint			; mm1 = (abs(last - *p) > thre_deint)
		packsswb	mm1, mm2
		pcmpgtw		mm0, pw_thre_shift			; mm0 = (abs(last - *p) > thre_shift)
		packsswb	mm0, mm2
		punpcklbw	mm1, mm0					; mm1 = count_sub (deint=low, shift=high)
		pxor		mm2, mm5					; mm2 = sign(last - *p) ^ sign1
		pxor		mm5, mm2					; sign1 = sign(last - *p)
		pand		mm4, mm2					; count1 &= mm2
		pand		mm4, mm1					; count1 &= mm1
		psubsb		mm4, mm1					; count1 -= count_sub
		movq		mm0, mm4
		pcmpgtb		mm0, pb_thre_count			; mm0 = (count1 > (thre_count-1))
		psrlw		mm0, 4
		pand		mm0, pw_mask_12stripe_1
		por			mm3, mm0

		movq		[edi], mm3

		lea			esi, [esi+ebx]
		lea			edx, [edx+ebx]
		dec			ecx
		jne			afs_analyze_12_mmx_loop
		ret

_afs_analyze_12_mmx@32 ENDP


PUBLIC C _afs_analyze_shrink_info_mmx@16

;void __stdcall afs_analyze_shrink_info_mmx(
;  [esp+04] unsigned char  *dst
;  [esp+08] PIXEL_YC       *src
;  [esp+12] int            h
;  [esp+16] int            si_pitch
;)

_afs_analyze_shrink_info_mmx@16 PROC
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+24

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [src]
		mov			ecx, [esp+24+12]			; [h]
		mov			ebx, [esp+24+16]			; [si_pitch]
		pxor		mm4, mm4					; or prev1
		pxor		mm5, mm5					; or prev2
		pxor		mm6, mm6					; or prev3
		xor			edx, edx					; or prev4
		movq		mm7, pw_mask_ffff

		call		afs_analyze_shrink_info_mmx_sub
		movd		edx, mm0
		lea			esi, [esi+24]
		call		afs_analyze_shrink_info_mmx_sub
		movq		mm6, mm0
		lea			esi, [esi+24]
		call		afs_analyze_shrink_info_mmx_sub
		movq		mm5, mm0
		lea			esi, [esi+24]
		call		afs_analyze_shrink_info_mmx_sub
		movq		mm4, mm0
		lea			esi, [esi+24]

		lea			ecx, [ecx-4]
afs_analyze_shrink_info_mmx_loop:
		call		afs_analyze_shrink_info_mmx_sub
		movq		mm2, mm6
		por			mm2, mm5
		por			mm2, mm4
		pand		mm2, pb_mask_12stripe_01
		pand		mm1, pb_mask_1stripe_01
		por			mm2, mm1
		movd		eax, mm2
		or			eax, edx
		movd		edx, mm6
		movq		mm6, mm5
		movq		mm5, mm4
		movq		mm4, mm0
		mov			[edi], eax
		lea			esi, [esi+24]
		lea			edi, [edi+ebx]
		loop		afs_analyze_shrink_info_mmx_loop

		movq		mm0, mm6
		por			mm0, mm5
		por			mm0, mm4
		pand		mm0, pb_mask_12stripe_01
		movd		eax, mm0
		or			eax, edx
		mov			[edi], eax

		movq		mm0, mm5
		por			mm0, mm4
		pand		mm0, pb_mask_12stripe_01
		por			mm0, mm6
		movd		dword ptr[edi+ebx], mm0

		lea			edi, [edi+ebx*2]

		movq		mm0, mm4
		pand		mm0, pb_mask_12stripe_01
		por			mm0, mm5
		movd		dword ptr[edi], mm0

		movd		dword ptr[edi+ebx], mm4

afs_analyze_shrink_info_mmx_exit:
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi
		ret			16

afs_analyze_shrink_info_mmx_sub:
; return mm0 = stripe info / mm1 = or component

		movq		mm0, [esi]
		movq		mm1, mm0
		movq		mm2, mm0
		movq		mm3, mm1
		pand		mm0, mm7

		psrlq		mm1, 16
		psrlq		mm2, 32
		pand		mm1, mm7
		pand		mm2, mm7

		psllq		mm7, 16

		psrlq		mm3, 32
		pand		mm3, mm7
		por			mm0, mm3

		movq		mm3, [esi+8]
		psllq		mm3, 16
		pand		mm3, mm7
		por			mm1, mm3

		movq		mm3, [esi+8]
		pand		mm3, mm7
		por			mm2, mm3

		psllq		mm7, 16

		movq		mm3, [esi+8]
		pand		mm3, mm7
		por			mm0, mm3

		movq		mm3, [esi+8]
		psrlq		mm3, 16
		pand		mm3, mm7
		por			mm1, mm3

		movq		mm3, [esi+16]
		psllq		mm3, 32
		pand		mm3, mm7
		por			mm2, mm3

		psllq		mm7, 16

		movq		mm3, [esi+16]
		psllq		mm3, 32
		pand		mm3, mm7
		por			mm0, mm3

		movq		mm3, [esi+16]
		psllq		mm3, 16
		pand		mm3, mm7
		por			mm1, mm3

		movq		mm3, [esi+16]
		pand		mm3, mm7
		por			mm2, mm3

		psrlq		mm7, 48

		movq		mm3, mm1
		por			mm1, mm0
		por			mm1, mm2
		pand		mm0, mm3
		pand		mm0, mm2

		psllw		mm1, 8
		psraw		mm0, 8
		psraw		mm1, 8
		packsswb	mm0, mm3					; mm0 = and [----3210]
		packsswb	mm1, mm3					; mm1 =  or [----3210]
		pand		mm0, pb_mask_12motion_01
		pand		mm1, pb_mask_12stripe_01
		por			mm0, mm1
		
		ret

_afs_analyze_shrink_info_mmx@16 ENDP

PUBLIC C _afs_analyze_1_mmx@32

;void __stdcall afs_analyze_1_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *p0
;  [esp+12] PIXEL_YC       *p1
;  [esp+16] int            tb_order
;  [esp+20] int            step
;  [esp+24] int            h
;  [esp+28] void           *scan
;  [esp+32] int            *motion_count
;)

_afs_analyze_1_mmx@32 PROC
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+24

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			eax, [esp+24+16]			; [tb_order]
		mov			ebx, [esp+24+20]			; [step]
		mov			ecx, [esp+24+24]			; [h]
		lea			ebx, [ebx+ebx*2]
		lea			ebx, [ebx+ebx]				; ebx = step * 6
		movq		mm3, pw_thre_shift
		call		afs_analyze_1_mmx_sub

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			ecx, [esp+24+24]			; [h]
		lea			edi, [edi+8]
		lea			esi, [esi+8]
		lea			edx, [edx+8]
		call		afs_analyze_1_mmx_sub

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			ecx, [esp+24+24]			; [h]
		lea			edi, [edi+16]
		lea			esi, [esi+16]
		lea			edx, [edx+16]
		call		afs_analyze_1_mmx_sub

; exit
afs_analyze_1_mmx_exit:
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi

		ret			32

afs_analyze_1_mmx_sub:

; top line
; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi]
		psubsw		mm0, [edx]					; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm2, mm3
		pcmpgtw		mm2, mm0					; mm2 = (thre_motion > abs(*p0 - *p1))
		pand		mm2, pw_mask_1motion_0		; mm2 &= 4000h

		movq		[edi], mm2

		pxor		mm4, mm4					; count1 = 0
		pxor		mm5, mm5					; sign1  = 0
		pxor		mm6, mm6					; count0 = 0
		pxor		mm7, mm7					; sign0  = 0

		lea			ecx, [ecx-1]
		test		eax, 1
		jne			afs_analyze_1_mmx_latter

;  if(abs_01diff < thre_motion) flag |= motion;
;  (new_sign, abs_diff) <= last - *p;
;  last = *p;
;  count_add = (new_sign ^ sign < 0);
;  sign = new_sign;
;  if(abs_diff < thre_shift/deint) count = 0;
;  count += count_add;
;  if(count >= thre_count) flag |= stripe;

afs_analyze_1_mmx_loop:

; former field line
		lea			edi, [edi+24]

; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi+ebx]
		psubsw		mm0, [edx+ebx]				; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm2, mm3
		pcmpgtw		mm2, mm0					; mm2 = (thre_shift > abs(*p0 - *p1))
		pand		mm2, pw_mask_1motion_0		; mm2 &= 4000h

; - analyze non-shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm1, mm1
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, mm3					; mm0 = (abs(last - *p) > thre_shift)
		pxor		mm1, mm7					; mm2 = sign(last - *p) ^ sign0
		pxor		mm7, mm1					; sign0 = sign(last - *p)
		pand		mm6, mm1					; count0 &= mm1
		pand		mm6, mm0					; count0 &= mm0
		psubsw		mm6, mm0					; count0 -= count_sub
		movq		mm1, mm6
		pcmpgtw		mm1, pw_thre_count1			; mm0 = (count0 > (thre_count-1))
		pand		mm1, pw_mask_1stripe_0
		por			mm2, mm1

; - analyze shift
		movq		mm0, [edx]					; mm0 = last = *(p1-step)
		pxor		mm1, mm1
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, mm3					; mm0 = (abs(last - *p) > thre_shift)
		pxor		mm1, mm5					; mm2 = sign(last - *p) ^ sign1
		pxor		mm5, mm1					; sign1 = sign(last - *p)
		pand		mm4, mm1					; count1 &= mm1
		pand		mm4, mm0					; count1 &= mm0
		psubsw		mm4, mm0					; count1 -= count_sub
		movq		mm1, mm4
		pcmpgtw		mm1, pw_thre_count1			; mm0 = (count1 > (thre_count-1))
		pand		mm1, pw_mask_1stripe_1
		por			mm2, mm1

		movq		[edi], mm2

		lea			esi, [esi+ebx]
		lea			edx, [edx+ebx]
		dec			ecx
		jne			afs_analyze_1_mmx_latter
		ret

afs_analyze_1_mmx_latter:

; latter field line
		lea			edi, [edi+24]

; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi+ebx]
		psubsw		mm0, [edx+ebx]				; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm2, mm3
		pcmpgtw		mm2, mm0					; mm2 = (thre_shift > abs(*p0 - *p1))
		pand		mm2, pw_mask_1motion_0		; mm2 &= 4000h

; - analyze non-shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm1, mm1
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, mm3					; mm0 = (abs(last - *p) > thre_shift)
		pxor		mm1, mm7					; mm2 = sign(last - *p) ^ sign0
		pxor		mm7, mm1					; sign0 = sign(last - *p)
		pand		mm6, mm1					; count0 &= mm1
		pand		mm6, mm0					; count0 &= mm0
		psubsw		mm6, mm0					; count0 -= count_sub
		movq		mm1, mm6
		pcmpgtw		mm1, pw_thre_count1			; mm0 = (count0 > (thre_count-1))
		pand		mm1, pw_mask_1stripe_0
		por			mm2, mm1

; - analyze shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm1, mm1
		psubsw		mm0, [edx+ebx]				; mm0 = last - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, mm3					; mm0 = (abs(last - *p) > thre_shift)
		pxor		mm1, mm5					; mm2 = sign(last - *p) ^ sign0
		pxor		mm5, mm1					; sign1 = sign(last - *p)
		pand		mm4, mm1					; count1 &= mm1
		pand		mm4, mm0					; count1 &= mm0
		psubsw		mm4, mm0					; count1 -= count_sub
		movq		mm1, mm4
		pcmpgtw		mm1, pw_thre_count1			; mm0 = (count1 > (thre_count-1))
		pand		mm1, pw_mask_1stripe_1
		por			mm2, mm1

		movq		[edi], mm2

		lea			esi, [esi+ebx]
		lea			edx, [edx+ebx]
		dec			ecx
		jne			afs_analyze_1_mmx_loop
		ret

_afs_analyze_1_mmx@32 ENDP


PUBLIC C _afs_analyze_2_mmx@32

;void __stdcall afs_analyze_2_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *p0
;  [esp+12] PIXEL_YC       *p1
;  [esp+16] int            tb_order
;  [esp+20] int            step
;  [esp+24] int            h
;  [esp+28] void           *scan
;  [esp+32] int            *motion_count
;)

_afs_analyze_2_mmx@32 PROC
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+24

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			eax, [esp+24+16]			; [tb_order]
		mov			ebx, [esp+24+20]			; [step]
		mov			ecx, [esp+24+24]			; [h]
		lea			ebx, [ebx+ebx*2]
		lea			ebx, [ebx+ebx]				; ebx = step * 6
		movq		mm3, pw_thre_motion1
		call		afs_analyze_2_mmx_sub

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			ecx, [esp+24+24]			; [h]
		lea			edi, [edi+8]
		lea			esi, [esi+8]
		lea			edx, [edx+8]
		movq		mm3, pw_thre_motion2
		call		afs_analyze_2_mmx_sub

		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [p0]
		mov			edx, [esp+24+12]			; [p1]
		mov			ecx, [esp+24+24]			; [h]
		lea			edi, [edi+16]
		lea			esi, [esi+16]
		lea			edx, [edx+16]
		movq		mm3, pw_thre_motion3
		call		afs_analyze_2_mmx_sub

; exit
afs_analyze_2_mmx_exit:
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi

		ret			32

afs_analyze_2_mmx_sub:

; top line
; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi]
		psubsw		mm0, [edx]					; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm2, mm3
		pcmpgtw		mm2, mm0					; mm2 = (thre_motion > abs(*p0 - *p1))
		pand		mm2, pw_mask_2motion_0		; mm2 &= 0400h

		movq		[edi], mm2

		pxor		mm4, mm4					; count1 = 0
		pxor		mm5, mm5					; sign1  = 0
		pxor		mm6, mm6					; count0 = 0
		pxor		mm7, mm7					; sign0  = 0

		lea			ecx, [ecx-1]
		test		eax, 1
		jne			afs_analyze_2_mmx_latter

;  if(abs_01diff < thre_motion) flag |= motion;
;  (new_sign, abs_diff) <= last - *p;
;  last = *p;
;  count_add = (new_sign ^ sign < 0);
;  sign = new_sign;
;  if(abs_diff < thre_shift/deint) count = 0;
;  count += count_add;
;  if(count >= thre_count) flag |= stripe;

afs_analyze_2_mmx_loop:

; former field line
		lea			edi, [edi+24]

; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi+ebx]
		psubsw		mm0, [edx+ebx]				; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm2, mm3
		pcmpgtw		mm2, mm0					; mm2 = (thre_motion > abs(*p0 - *p1))
		pand		mm2, pw_mask_2motion_0		; mm2 &= 0400h

; - analyze non-shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm1, mm1
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, pw_thre_deint			; mm0 = (abs(last - *p) > thre_deint)
		pxor		mm1, mm7					; mm2 = sign(last - *p) ^ sign0
		pxor		mm7, mm1					; sign0 = sign(last - *p)
		pand		mm6, mm1					; count0 &= mm1
		pand		mm6, mm0					; count0 &= mm0
		psubsw		mm6, mm0					; count0 -= count_sub
		movq		mm1, mm6
		pcmpgtw		mm1, pw_thre_count2			; mm0 = (count0 > (thre_count-1))
		pand		mm1, pw_mask_2stripe_0
		por			mm2, mm1

; - analyze shift
		movq		mm0, [edx]					; mm0 = last = *(p1-step)
		pxor		mm1, mm1
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, pw_thre_deint			; mm0 = (abs(last - *p) > thre_deint)
		pxor		mm1, mm5					; mm2 = sign(last - *p) ^ sign1
		pxor		mm5, mm1					; sign1 = sign(last - *p)
		pand		mm4, mm1					; count1 &= mm1
		pand		mm4, mm0					; count1 &= mm0
		psubsw		mm4, mm0					; count1 -= count_sub
		movq		mm1, mm4
		pcmpgtw		mm1, pw_thre_count2			; mm0 = (count1 > (thre_count-1))
		pand		mm1, pw_mask_2stripe_1
		por			mm2, mm1

		movq		[edi], mm2

		lea			esi, [esi+ebx]
		lea			edx, [edx+ebx]
		dec			ecx
		jne			afs_analyze_2_mmx_latter
		ret

afs_analyze_2_mmx_latter:

; latter field line
		lea			edi, [edi+24]

; - analyze motion
		pxor		mm1, mm1
		movq		mm0, [esi+ebx]
		psubsw		mm0, [edx+ebx]				; mm0 = *p0 - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(*p0 - *p1) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(*p0 - *p1)
		movq		mm2, mm3
		pcmpgtw		mm2, mm0					; mm2 = (thre_motion > abs(*p0 - *p1))
		pand		mm2, pw_mask_2motion_0		; mm2 &= 0400h

; - analyze non-shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm1, mm1
		psubsw		mm0, [esi+ebx]				; mm0 = last - *p0
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, pw_thre_deint			; mm0 = (abs(last - *p) > thre_deint)
		pxor		mm1, mm7					; mm2 = sign(last - *p) ^ sign0
		pxor		mm7, mm1					; sign0 = sign(last - *p)
		pand		mm6, mm1					; count0 &= mm1
		pand		mm6, mm0					; count0 &= mm0
		psubsw		mm6, mm0					; count0 -= count_sub
		movq		mm1, mm6
		pcmpgtw		mm1, pw_thre_count2			; mm0 = (count0 > (thre_count-1))
		pand		mm1, pw_mask_2stripe_0
		por			mm2, mm1

; - analyze shift
		movq		mm0, [esi]					; mm0 = last = *(p0-step)
		pxor		mm1, mm1
		psubsw		mm0, [edx+ebx]				; mm0 = last - *p1
		pcmpgtw		mm1, mm0					; mm1 = sign(last - *p) = (0 > mm0)
		pxor		mm0, mm1
		psubsw		mm0, mm1					; mm0 = abs(last - *p)
		pcmpgtw		mm0, pw_thre_deint			; mm0 = (abs(last - *p) > thre_deint)
		pxor		mm1, mm5					; mm2 = sign(last - *p) ^ sign0
		pxor		mm5, mm1					; sign1 = sign(last - *p)
		pand		mm4, mm1					; count1 &= mm1
		pand		mm4, mm0					; count1 &= mm0
		psubsw		mm4, mm0					; count1 -= count_sub
		movq		mm1, mm4
		pcmpgtw		mm1, pw_thre_count2			; mm0 = (count1 > (thre_count-1))
		pand		mm1, pw_mask_2stripe_1
		por			mm2, mm1

		movq		[edi], mm2

		lea			esi, [esi+ebx]
		lea			edx, [edx+ebx]
		dec			ecx
		jne			afs_analyze_2_mmx_loop
		ret

_afs_analyze_2_mmx@32 ENDP

END
