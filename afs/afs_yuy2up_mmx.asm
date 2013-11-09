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

pw_1				dd	000010001h

;extern void __stdcall afs_yuy2up_mmx(PIXEL_YC *ycp, int n);

PUBLIC C _afs_yuy2up_mmx@12

_afs_yuy2up_mmx@12 PROC
		push		edi
		push		esi
		push		edx
		push		ecx
		push		ebx
		push		eax
; @+24

; main loop
		mov			edi, [esp+24+4]				; [dst]
		mov			esi, [esp+24+8]				; [src]
		mov			ecx, [esp+24+12]			; [step]
		movd		mm7, pw_1
		movd		mm3, dword ptr[esi+2]
		movd		mm0, dword ptr[esi+2]
		movd		mm1, dword ptr[esi+14]
		lea			esi, [esi+26]

afs_yuy2up_mmx_loop:
		movq		mm6, [esi-26]
		movq		mm2, [esi]
		lea			ecx, [ecx-2]
		movq		mm4, mm0
		movq		mm5, mm2
		paddsw		mm4, mm1
		paddsw		mm5, mm3
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm0 + mm1 - (mm2+mm3-mm0-mm1)/8 + 1) / 2
		lea			esi, [esi+12]
		lea			edi, [edi+12]
		cmp			ecx,4
		je			afs_yuy2up_mmx_exit_01_4
		jl			afs_yuy2up_mmx_exit_01_3

		movq		mm6, [esi-26]
		movq		mm3, [esi]
		lea			ecx, [ecx-2]
		movq		mm4, mm1
		movq		mm5, mm0
		paddsw		mm4, mm2
		paddsw		mm5, mm3
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm1 + mm2 - (mm3+mm0-mm1-mm2)/8 + 1) / 2
		lea			esi, [esi+12]
		lea			edi, [edi+12]
		cmp			ecx,4
		je			afs_yuy2up_mmx_exit_12_4
		jl			afs_yuy2up_mmx_exit_12_3

		movq		mm6, [esi-26]
		movq		mm0, [esi]
		lea			ecx, [ecx-2]
		movq		mm4, mm2
		movq		mm5, mm0
		paddsw		mm4, mm3
		paddsw		mm5, mm1
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm2 + mm3 - (mm0+mm1-mm2-mm3)/8 + 1) / 2
		lea			esi, [esi+12]
		lea			edi, [edi+12]
		cmp			ecx,4
		je			afs_yuy2up_mmx_exit_23_4
		jl			afs_yuy2up_mmx_exit_23_3

		movq		mm6, [esi-26]
		movq		mm1, [esi]
		lea			ecx, [ecx-2]
		movq		mm4, mm0
		movq		mm5, mm1
		paddsw		mm4, mm3
		paddsw		mm5, mm2
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm3 + mm0 - (mm1+mm2-mm3-mm0)/8 + 1) / 2
		lea			esi, [esi+12]
		lea			edi, [edi+12]
		cmp			ecx,4
		je			afs_yuy2up_mmx_exit_30_4
		jl			afs_yuy2up_mmx_exit_30_3
		jmp			afs_yuy2up_mmx_loop

afs_yuy2up_mmx_exit_01_4:
		movq		mm6, [esi-26]
		movq		mm4, mm1
		movq		mm5, mm0
		paddsw		mm4, mm2
		paddsw		mm5, mm2
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm1 + mm2 - (mm2+mm0-mm1-mm2)/8 + 1) / 2

		movq		mm6, [esi-14]
		movq		mm4, mm2
		movq		mm5, mm1
		paddsw		mm4, mm2
		paddsw		mm5, mm2
		movq		[edi+12], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+20], mm4				; (mm2 + mm2 - (mm2+mm1-mm2-mm2)/8 + 1) / 2

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_01_3:
		movq		mm6, [esi-26]
		movq		mm4, mm1
		movq		mm5, mm0
		movq		[edi], mm6
		paddsw		mm4, mm2
		paddsw		mm5, mm2
		mov			eax, [esi-14]
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		mov			bx, [esi-10]
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm1 + mm2 - (mm2+mm0-mm1-mm2)/8 + 1) / 2
		mov			[edi+12], eax
		mov			[edi+16], bx

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_12_4:
		movq		mm6, [esi-26]
		movq		mm4, mm2
		movq		mm5, mm1
		paddsw		mm4, mm3
		paddsw		mm5, mm3
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm2 + mm3 - (mm3+mm1-mm2-mm3)/8 + 1) / 2

		movq		mm6, [esi-14]
		movq		mm4, mm3
		movq		mm5, mm2
		paddsw		mm4, mm3
		paddsw		mm5, mm3
		movq		[edi+12], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+20], mm4				; (mm3 + mm3 - (mm3+mm2-mm3-mm3)/8 + 1) / 2

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_12_3:
		movq		mm6, [esi-26]
		movq		mm4, mm2
		movq		mm5, mm1
		movq		[edi], mm6
		paddsw		mm4, mm3
		paddsw		mm5, mm3
		mov			eax, [esi-14]
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		mov			bx, [esi-10]
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm2 + mm3 - (mm3+mm1-mm2-mm3)/8 + 1) / 2
		mov			[edi+12], eax
		mov			[edi+16], bx

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_23_4:
		movq		mm6, [esi-26]
		movq		mm4, mm3
		movq		mm5, mm2
		paddsw		mm4, mm0
		paddsw		mm5, mm0
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm3 + mm0 - (mm0+mm2-mm3-mm0)/8 + 1) / 2

		movq		mm6, [esi-14]
		movq		mm4, mm0
		movq		mm5, mm3
		paddsw		mm4, mm0
		paddsw		mm5, mm0
		movq		[edi+12], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+20], mm4				; (mm0 + mm0 - (mm0+mm3-mm0-mm0)/8 + 1) / 2

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_23_3:
		movq		mm6, [esi-26]
		movq		mm4, mm3
		movq		mm5, mm2
		movq		[edi], mm6
		paddsw		mm4, mm0
		paddsw		mm5, mm0
		mov			eax, [esi-14]
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		mov			bx, [esi-10]
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm3 + mm0 - (mm0+mm2-mm3-mm0)/8 + 1) / 2
		mov			[edi+12], eax
		mov			[edi+16], bx

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_30_4:
		movq		mm6, [esi-26]
		movq		mm4, mm0
		movq		mm5, mm3
		paddsw		mm4, mm1
		paddsw		mm5, mm1
		movq		[edi], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm0 + mm1 - (mm1+mm3-mm0-mm1)/8 + 1) / 2

		movq		mm6, [esi-14]
		movq		mm4, mm1
		movq		mm5, mm0
		paddsw		mm4, mm1
		paddsw		mm5, mm1
		movq		[edi+12], mm6
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+ebx], mm4				; (mm1 + mm1 - (mm1+mm0-mm1-mm1)/8 + 1) / 2

		jmp			afs_yuy2up_mmx_exit

afs_yuy2up_mmx_exit_30_3:
		movq		mm6, [esi-26]
		movq		mm4, mm0
		movq		mm5, mm3
		movq		[edi], mm6
		paddsw		mm4, mm1
		paddsw		mm5, mm1
		mov			eax, [esi-14]
		psubsw		mm5, mm4
		paddsw		mm4, mm7
		mov			bx, [esi-10]
		psraw		mm5, 3
		psubsw		mm4, mm5
		psraw		mm4, 1
		movd		dword ptr[edi+8], mm4				; (mm0 + mm1 - (mm1+mm3-mm0-mm1)/8 + 1) / 2
		mov			[edi+12], eax
		mov			[edi+16], bx

; exit
afs_yuy2up_mmx_exit:
		emms

		pop			eax
		pop			ebx
		pop			ecx
		pop			edx
		pop			esi
		pop			edi

		ret			12

_afs_yuy2up_mmx@12 ENDP

END
