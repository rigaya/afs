.586
.mmx
.model flat

_TEXT64 segment page public use32 'CODE'
_TEXT64 ends

_DATA64 segment page public use32 'DATA'
_DATA64 ends

CONST segment dword use32 public 'CONST'
CONST ends

_BSS segment dword use32 public 'BSS'
_BSS ends

_TEXT64 segment
	align 16

pqb_mask_a0			dq	00000000000000000h
pqb_mask_a1			dq	000000000000000ffh
pqb_mask_a2			dq	0000000000000ffffh
pqb_mask_a3			dq	00000000000ffffffh
pqb_mask_a4			dq	000000000ffffffffh
pqb_mask_a5			dq	0000000ffffffffffh
pqb_mask_a6			dq	00000ffffffffffffh
pqb_mask_a7			dq	000ffffffffffffffh
pqb_mask_a8			dq	0ffffffffffffffffh

pqb_mask_s0			dq	00000000000000000h
pqb_mask_s1			dq	000000000000000ffh
pqb_mask_s2			dq	0000000000000ff00h
pqb_mask_s3			dq	00000000000ff0000h
pqb_mask_s4			dq	000000000ff000000h
pqb_mask_s5			dq	0000000ff00000000h
pqb_mask_s6			dq	00000ff0000000000h
pqb_mask_s7			dq	000ff000000000000h
pqb_mask_s8			dq	0ff00000000000000h

pb_mask_03			dq	00303030303030303h
pb_mask_04			dq	00404040404040404h
pb_mask_f8			dq	0f8f8f8f8f8f8f8f8h

PUBLIC C _afs_analyzemap_filter_mmx@16

;void __stdcall afs_analyzemap_filter_mmx(
;  [esp+04] unsigned       char* sip
;  [esp+08] int            si_w
;  [esp+12] int            w
;  [esp+16] int            h
;)

_afs_analyzemap_filter_mmx@16 PROC
		push		ebp
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+28

;;; loop horizontal_1

		mov			edi, [esp+28+4]				; [sip]
		mov			ebx, [esp+28+8]				; [si_w]
		mov			ecx, [esp+28+16]			; [h]
		movq		mm6, pb_mask_03
		movq		mm7, pb_mask_04
		
afs_analyzemap_filter_mmx_looph1y:
; left most
		mov			edx, [esp+28+12]			; [w]
		movq		mm1, [edi]					; mm1 = c
		movq		mm2, [edi+8]				; mm2 = c+8
		lea			edx, [edx-8]
		
		movq		mm3, mm1
		movq		mm4, mm1
		psllq		mm3, 8
		psrlq		mm4, 8
		movq		mm5, mm1
		pand		mm5, pqb_mask_s1
		por			mm3, mm5					; mm3 = x-
		movq		mm5, mm2
		psllq		mm5, 56
		por			mm4, mm5					; mm4 = x+
		
		movq		mm5, mm3
		por			mm3, mm4
		pand		mm5, mm4
		pand		mm3, mm6					; mm3 = (x- | x+) & 03h
		pand		mm5, mm7					; mm5 = (x- & x+) & 04h
		por			mm3, mm1
		por			mm3, mm5
		movq		[edi], mm3					; [esi] = c | mm3 | mm5
		lea			esi, [edi+8]
		cmp			edx, 8
		jng			afs_analyzemap_filter_mmx_looph1x_last

afs_analyzemap_filter_mmx_looph1x:
            movq		mm0, mm1
            movq		mm1, mm2
            movq		mm2, [esi+8]
            lea			edx, [edx-8]
            
            movq		mm3, mm1
            movq		mm4, mm1
            psllq		mm3, 8
            psrlq		mm4, 8
            movq		mm5, mm0
            psrlq		mm5, 56
            por			mm3, mm5					; mm3 = x-
            movq		mm5, mm2
            psllq		mm5, 56
            por			mm4, mm5					; mm4 = x+
            
            movq		mm5, mm3
            por			mm3, mm4
            pand		mm5, mm4
            pand		mm3, mm6					; mm3 = (x- | x+) & 03h
            pand		mm5, mm7					; mm5 = (x- & x+) & 04h
            por			mm3, mm1
            por			mm3, mm5
            movq		[esi], mm3					; [esi] = c | mm3 | mm5
            lea			esi, [esi+8]
            cmp			edx, 8
            jg			afs_analyzemap_filter_mmx_looph1x

afs_analyzemap_filter_mmx_looph1x_last:
		movq		mm0, [pqb_mask_a0 + edx*8]
		
		movq		mm3, mm2
		movq		mm4, mm2
		psllq		mm3, 8
		pand		mm4, mm0
		psrlq		mm4, 8
		movq		mm5, mm1
		psrlq		mm5, 56
		por			mm3, mm5					; mm3 = x-
		movq		mm5, mm2
		pand		mm5, [pqb_mask_s0 + edx*8]
		por			mm4, mm5					; mm4 = x+
		
		movq		mm5, mm3
		por			mm3, mm4
		pand		mm5, mm4
		pand		mm3, mm6					; mm3 = (x- | x+) & 03h
		pand		mm5, mm7					; mm5 = (x- & x+) & 04h
		por			mm3, mm2
		por			mm3, mm5
		pand		mm3, mm0
		movq		[esi], mm3					; [esi] = c | mm3 | mm5
		lea			edi, [edi + ebx]
		dec			ecx
		jne			afs_analyzemap_filter_mmx_looph1y

;;; loop vertical_1

		mov			edi, [esp+28+4]				; [sip]
		mov			ecx, ebx
		shr			ecx, 3						; ecx = sip_w>>3
		por			mm7, mm6					; mm7 = pb_mask_07
		
afs_analyzemap_filter_mmx_loopv1x:
; top most
		mov			edx, [esp+28+16]			; [h]
		movq		mm1, [edi]					; mm1 = c
		movq		mm2, [edi+ebx]				; mm2 = y+
		lea			edx, [edx-2]
		
		movq		mm3, mm2
		pand		mm3, mm1
		pand		mm3, mm7					; mm3 = (c & y+) & 07h
		por			mm3, mm1					; mm3 = c | mm3
		movq		[edi], mm3
		lea			esi, [edi+ebx]

afs_analyzemap_filter_mmx_loopv1y:
            movq		mm0, mm1
            movq		mm1, mm2
            movq		mm2, [esi+ebx]
            
            movq		mm3, mm2
            pand		mm3, mm0
            pand		mm3, mm7					; mm3 = (y- & y+) & 07h
            por			mm3, mm1					; mm3 = c | mm3
            movq		[esi], mm3
            lea			esi, [esi+ebx]
            
            dec			edx
            jne			afs_analyzemap_filter_mmx_loopv1y

		movq		mm3, mm2
		pand		mm3, mm1
		pand		mm3, mm7					; mm3 = (y- & c) & 07h
		por			mm3, mm2					; mm3 = c | mm3
		movq		[esi], mm3
		lea			edi, [edi+8]
		dec			ecx
		jne			afs_analyzemap_filter_mmx_loopv1x

;;; loop horizontal_2

		mov			edi, [esp+28+4]				; [sip]
		mov			ebx, [esp+28+8]				; [si_w]
		mov			ecx, [esp+28+16]			; [h]
		movq		mm6, pb_mask_f8
		
afs_analyzemap_filter_mmx_looph2y:
; left most
		mov			edx, [esp+28+12]			; [w]
		movq		mm1, [edi]					; mm1 = c
		movq		mm2, [edi+8]				; mm2 = c+8
		lea			edx, [edx-8]
		
		movq		mm3, mm1
		movq		mm4, mm1
		psllq		mm3, 8
		psrlq		mm4, 8
		movq		mm5, mm1
		pand		mm5, pqb_mask_s1
		por			mm3, mm5					; mm3 = x-
		movq		mm5, mm2
		psllq		mm5, 56
		por			mm4, mm5					; mm4 = x+
		
		pand		mm3, mm4
		por			mm3, mm6					; mm3 = (x- & x+) | f8h
		pand		mm3, mm1
		movq		[edi], mm3					; [esi] = c & mm3
		lea			esi, [edi+8]
		cmp			edx, 8
		jng			afs_analyzemap_filter_mmx_looph2x_last

afs_analyzemap_filter_mmx_looph2x:
            movq		mm0, mm1
            movq		mm1, mm2
            movq		mm2, [esi+8]
            lea			edx, [edx-8]
            
            movq		mm3, mm1
            movq		mm4, mm1
            psllq		mm3, 8
            psrlq		mm4, 8
            movq		mm5, mm0
            psrlq		mm5, 56
            por			mm3, mm5					; mm3 = x-
            movq		mm5, mm2
            psllq		mm5, 56
            por			mm4, mm5					; mm4 = x+
            
            pand		mm3, mm4
            por			mm3, mm6					; mm3 = (x- & x+) | f8h
            pand		mm3, mm1
            movq		[esi], mm3					; [esi] = c & mm3
            lea			esi, [esi+8]
            cmp			edx, 8
            jg			afs_analyzemap_filter_mmx_looph2x

afs_analyzemap_filter_mmx_looph2x_last:
		movq		mm0, [pqb_mask_a0 + edx*8]
		
		movq		mm3, mm2
		movq		mm4, mm2
		psllq		mm3, 8
		pand		mm4, mm0
		psrlq		mm4, 8
		movq		mm5, mm1
		psrlq		mm5, 56
		por			mm3, mm5					; mm3 = x-
		movq		mm5, mm2
		pand		mm5, [pqb_mask_s0 + edx*8]
		por			mm4, mm5					; mm4 = x+
		
		pand		mm3, mm4
		por			mm3, mm6					; mm3 = (x- & x+) | f8h
		pand		mm3, mm2
		pand		mm3, mm0
		movq		[esi], mm3					; [esi] = c & mm3
		lea			edi, [edi + ebx]
		dec			ecx
		jne			afs_analyzemap_filter_mmx_looph2y

;;; loop vertical_2

		mov			edi, [esp+28+4]				; [sip]
		mov			ecx, ebx
		shr			ecx, 3						; ecx = sip_w>>3
		
afs_analyzemap_filter_mmx_loopv2x:
; top most
		mov			edx, [esp+28+16]			; [h]
		movq		mm1, [edi]					; mm1 = c
		movq		mm2, [edi+ebx]				; mm2 = y+
		lea			edx, [edx-2]
		
		movq		mm3, mm2
		pand		mm3, mm1
		por			mm3, mm6					; mm3 = (c & y+) | f8h
		pand		mm3, mm1					; mm3 = c & mm3
		movq		[edi], mm3
		lea			esi, [edi+ebx]

afs_analyzemap_filter_mmx_loopv2y:
            movq		mm0, mm1
            movq		mm1, mm2
            movq		mm2, [esi+ebx]
            
            movq		mm3, mm2
            pand		mm3, mm0
            por			mm3, mm6					; mm3 = (y- & y+) | f8h
            pand		mm3, mm1					; mm3 = c & mm3
            movq		[esi], mm3
            lea			esi, [esi+ebx]
            
            dec			edx
            jne			afs_analyzemap_filter_mmx_loopv2y

		movq		mm3, mm2
		pand		mm3, mm1
		por			mm3, mm6					; mm3 = (y- & c) | f8h
		pand		mm3, mm2					; mm3 = c & mm3
		movq		[esi], mm3
		lea			edi, [edi+8]
		dec			ecx
		jne			afs_analyzemap_filter_mmx_loopv2x

; exit
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi
		pop			ebp

		ret        16

_afs_analyzemap_filter_mmx@16 ENDP

pb_mask_33			dq	03333333333333333h
pb_mask_f3			dq	0f3f3f3f3f3f3f3f3h
pb_mask_44			dq	04444444444444444h

PUBLIC C _afs_merge_scan_mmx@28

;void __stdcall afs_merge_scan_mmx(
;  [esp+04] unsigned       char* dst
;  [esp+08] unsigned       char* src0
;  [esp+12] unsigned       char* src1
;  [esp+16] int            si_w
;  [esp+20] int            h
;  [esp+24] int            x_start
;  [esp+28] int            x_fin
;)

_afs_merge_scan_mmx@28 PROC
		push		ebp
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+28

		xor			eax, eax
		mov			ebx, [esp+28+16]			; [si_w]
		mov			ecx, ebx
		shr			ecx, 3						; ecx = sip_w>>3
		movq		mm6, pb_mask_33
		movq		mm7, pb_mask_f3
		
afs_merge_scan_mmx_loopx:
; top most
		mov			edi, [esp+28+4]				; [dst]
		mov			esi, [esp+28+8]				; [src0]
		mov			ebp, [esp+28+12]			; [src1]
		lea			edi, [edi+eax]
		lea			esi, [esi+eax]
		lea			ebp, [ebp+eax]
		mov			edx, [esp+28+20]			; [h]
		movq		mm0, [esi]					; mm0 = 0c
		movq		mm1, [esi+ebx]				; mm1 = 0y+
		movq		mm2, [ebp]					; mm2 = 1c
		movq		mm3, [ebp+ebx]				; mm3 = 1y+
		lea			edx, [edx-2]
		movq		mm4, mm0
		por			mm4, mm1
		por			mm4, mm7
		pand		mm4, mm0					; mm4 = (0c | 0y+ | f3h) & c
		movq		mm5, mm2
		por			mm5, mm3
		por			mm5, mm7
		pand		mm5, mm2					; mm5 = (1c | 1y+ | f3h) & c
		pand		mm4, mm5
		pand		mm4, pb_mask_44				; mm4 = mm4 & mm5 & 44h
		movq		mm5, mm0
		pandn		mm5, mm6					; mm5 = ~c & 33h
		por			mm4, mm5					; mm4 = mm4 | mm5
		movq		[edi], mm4
		lea			edi, [edi+ebx]
		lea			esi, [esi+ebx]
		lea			ebp, [ebp+ebx]
		
afs_merge_scan_mmx_loopy:
		movq		mm4, mm0
		movq		mm0, mm1
		movq		mm1, [esi+ebx]
		por			mm4, mm1
		por			mm4, mm7
		pand		mm4, mm0					; mm4 = (0y- | 0y+ | f3h) & c
		movq		mm5, mm2
		movq		mm2, mm3
		movq		mm3, [ebp+ebx]
		por			mm5, mm3
		por			mm5, mm7
		pand		mm5, mm2					; mm5 = (1y- | 1y+ | f3h) & c
		pand		mm4, mm5
		pand		mm4, pb_mask_44				; mm4 = mm4 & mm5 & 44h
		movq		mm5, mm0
		pandn		mm5, mm6					; mm5 = ~c & 33h
		por			mm4, mm5					; mm4 = mm4 | mm5
		movq		[edi], mm4
		lea			edi, [edi+ebx]
		lea			esi, [esi+ebx]
		lea			ebp, [ebp+ebx]
		
		dec			edx
		jne			afs_merge_scan_mmx_loopy

		movq		mm4, mm0
		por			mm4, mm1
		por			mm4, mm7
		pand		mm4, mm1					; mm4 = (0y- | 0c | f3h) & c
		movq		mm5, mm2
		por			mm5, mm3
		por			mm5, mm7
		pand		mm5, mm3					; mm5 = (1y- | 1c | f3h) & c
		pand		mm4, mm5
		pand		mm4, pb_mask_44				; mm4 = mm4 & mm5 & 44h
		movq		mm5, mm1
		pandn		mm5, mm6					; mm5 = ~c & 33h
		por			mm4, mm5					; mm4 = mm4 | mm5
		movq		[edi], mm4
		lea			eax, [eax+8]
		dec			ecx
		jne			afs_merge_scan_mmx_loopx

; exit
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi
		pop			ebp

		ret        28

_afs_merge_scan_mmx@28 ENDP

END

