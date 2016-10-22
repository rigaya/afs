.586
.mmx
.model flat

_TEXT64 segment page public use32 'CODE'
_TEXT64 ends

_DATA64 segment page public use32 'DATA'
sip_limit_7            dd    0
sip_limit_5            dd    0
_DATA64 ends

CONST segment dword use32 public 'CONST'
CONST ends

_BSS segment dword use32 public 'BSS'
_BSS ends

_TEXT64 segment
    align 16

pw_round_fix1        dq    00001000100010001h
pw_round_fix2        dq    00002000200020002h
dq_mask_center32    dq    00000ffffffff0000h
pw_mask_0c            dq    0000c000c000c000ch

PUBLIC C _afs_blend_mmx@32

;void __stdcall afs_blend_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *src1
;  [esp+12] PIXEL_YC       *src2
;  [esp+16] PIXEL_YC       *src3
;  [esp+20] unsigned char  *sip
;  [esp+24] unsigned int   mask
;  [esp+28] int            w
;  [esp+32] int            src_frame_size
;)

_afs_blend_mmx@32 PROC
        push        edi
        push        esi
        push        edx
        push        ecx
        push        ebx
        push        eax
; @+24

; setup coefficients

; main loop
        mov            edi, [esp+24+4]                ; [dst]
        mov            eax, [esp+24+8]                ; [src1]
        mov            esi, [esp+24+12]            ; [src2]
        mov            ebx, [esp+24+16]            ; [src3]
        mov            edx, [esp+24+20]            ; [sip]
        movd        mm5, dword ptr[esp+24+24]            ; [mask]
        mov            ecx, [esp+24+28]            ; [w]
        pxor        mm4, mm4
        punpcklbw    mm5, mm4
        movq        mm6, pw_round_fix2
        movq        mm7, dq_mask_center32
        lea            ecx, [ecx-4]
        jmp            afs_blend_mmx_loop_entry

afs_blend_mmx_loop_last3:
        lea            esi, [esi+18]
        lea            edi, [edi+18]
        lea            eax, [eax+18]
        lea            ebx, [ebx+18]
        lea            edx, [edx+3]
        lea            ecx, [ecx-3]
        jmp            afs_blend_mmx_loop_entry
afs_blend_mmx_loop_last2:
        lea            esi, [esi+12]
        lea            edi, [edi+12]
        lea            eax, [eax+12]
        lea            ebx, [ebx+12]
        lea            edx, [edx+2]
        lea            ecx, [ecx-2]
        movd        mm0, dword ptr[edx]                    ; mm0 = *sip
        pxor        mm4, mm4
        punpcklbw    mm0, mm4
        pand        mm0, mm5
        pcmpeqw        mm0, mm4                    ; mm0 = sip(33221100)
        jmp            afs_blend_mmx_loop_q2
afs_blend_mmx_loop_last1:
        lea            esi, [esi+6]
        lea            edi, [edi+6]
        lea            eax, [eax+6]
        lea            ebx, [ebx+6]
        lea            edx, [edx+1]
        lea            ecx, [ecx-1]
        movd        mm0, dword ptr[edx]                    ; mm0 = *sip
        pxor        mm4, mm4
        punpcklbw    mm0, mm4
        pand        mm0, mm5
        pcmpeqw        mm0, mm4                    ; mm0 = sip(33221100)
        jmp            afs_blend_mmx_loop_q3
afs_blend_mmx_loop:
        lea            esi, [esi+24]
        lea            edi, [edi+24]
        lea            eax, [eax+24]
        lea            ebx, [ebx+24]
        lea            edx, [edx+4]
        lea            ecx, [ecx-4]

afs_blend_mmx_loop_entry:
        movd        mm0, dword ptr[edx]                    ; mm0 = *sip
        pxor        mm4, mm4
        punpcklbw    mm0, mm4
        pand        mm0, mm5
        pcmpeqw        mm0, mm4                    ; mm0 = sip(33221100)

        movq        mm1, mm7                    ; mm1 = dq_mask_center32
        movq        mm2, mm0
        punpcklwd    mm2, mm0                    ; mm2 = sip(11110000)
        movq        mm3, mm2 ; mm3 = sip(11110000)
        psllq        mm3, 16  ; mm3 = sip(110000xx)
        pand        mm3, mm1 ; mm3 = sip(xx0000xx)
        pandn        mm1, mm2 ; mm1 = sip(11xxxx00)
        por            mm1, mm3                    ; mm1 = sip(11000000)

        movq        mm4, [esi]
        movq        mm3, [eax]                    ; mm3 = ycp1
        movq        mm2, mm4                    ; mm2 = ycp2
        paddsw        mm3, [ebx]                    ; mm3 += ycp3
        psllw        mm4, 1
        paddsw        mm3, mm4                    ; mm3 += ycp2 * 2
        paddsw        mm3, mm6                    ; mm3 += 2
        psraw        mm3, 2
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip ? mm3 : mm2
        movq        [edi], mm1

afs_blend_mmx_loop_q2:
        movq        mm1, mm0
        psrlq        mm1, 16
        movq        mm2, mm1
        punpcklwd    mm1, mm2                    ; mm1 = sip(22221111)

        movq        mm4, [esi+8]
        movq        mm3, [eax+8]                ; mm3 = ycp1
        movq        mm2, mm4                    ; mm2 = ycp2
        paddsw        mm3, [ebx+8]                ; mm3 += ycp3
        psllw        mm4, 1
        paddsw        mm3, mm4                    ; mm3 += ycp2 * 2
        paddsw        mm3, mm6                    ; mm3 += 2
        psraw        mm3, 2
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip ? mm3 : mm2
        movq        [edi+8], mm1

afs_blend_mmx_loop_q3:
        movq        mm1, mm7                    ; mm1 = dq_mask_center32
        movq        mm2, mm0
        punpckhwd    mm2, mm0                    ; mm2 = sip(33332222)
        movq        mm3, mm2
        psrlq        mm3, 16
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip(33333322)

        movq        mm4, [esi+16]
        movq        mm3, [eax+16]                ; mm3 = ycp1
        movq        mm2, mm4                    ; mm2 = ycp2
        paddsw        mm3, [ebx+16]                ; mm3 += ycp3
        psllw        mm4, 1
        paddsw        mm3, mm4                    ; mm3 += ycp2 * 2
        paddsw        mm3, mm6                    ; mm3 += 2
        psraw        mm3, 2
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip ? mm3 : mm2
        movq        [edi+16], mm1

        cmp            ecx,3
        jg            afs_blend_mmx_loop
        je            afs_blend_mmx_loop_last3
        cmp            ecx,1
        jg            afs_blend_mmx_loop_last2
        je            afs_blend_mmx_loop_last1

; exit
afs_blend_mmx_exit:
        emms

        pop            eax
        pop            ebx
        pop            ecx
        pop            edx
        pop            esi
        pop            edi

        ret        32

_afs_blend_mmx@32 ENDP

PUBLIC C _afs_mie_spot_mmx@32

;void __stdcall afs_mie_spot_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *src1
;  [esp+12] PIXEL_YC       *src2
;  [esp+16] PIXEL_YC       *src3
;  [esp+20] PIXEL_YC       *src4
;  [esp+24] PIXEL_YC       *src_spot
;  [esp+28] int            w
;  [esp+32] int            src_frame_size
;)

_afs_mie_spot_mmx@32 PROC
        push        ebp
        push        edi
        push        esi
        push        edx
        push        ecx
        push        ebx
        push        eax
; @+28

; setup coefficients

; main loop
        mov            edi, [esp+28+4]                ; [dst]
        mov            esi, [esp+28+8]                ; [src1]
        mov            eax, [esp+28+12]            ; [src2]
        mov            ebx, [esp+28+16]            ; [src3]
        mov            edx, [esp+28+20]            ; [src4]
        mov            ebp, [esp+28+24]            ; [src_spot]
        mov            ecx, [esp+28+28]            ; [w]
        movq        mm6, pw_round_fix1
        movq        mm7, pw_round_fix2
        lea            ecx, [ecx-4]
        jmp            afs_mie_spot_mmx_loop_entry

afs_mie_spot_mmx_loop_last3:
        lea            ebp, [ebp+18]
        lea            esi, [esi+18]
        lea            edi, [edi+18]
        lea            eax, [eax+18]
        lea            ebx, [ebx+18]
        lea            edx, [edx+18]
        lea            ecx, [ecx-3]
        jmp            afs_mie_spot_mmx_loop_entry
afs_mie_spot_mmx_loop_last2:
        lea            ebp, [ebp+12]
        lea            esi, [esi+12]
        lea            edi, [edi+12]
        lea            eax, [eax+12]
        lea            ebx, [ebx+12]
        lea            edx, [edx+12]
        lea            ecx, [ecx-2]
        jmp            afs_mie_spot_mmx_loop_q2
afs_mie_spot_mmx_loop_last1:
        lea            ebp, [ebp+6]
        lea            esi, [esi+6]
        lea            edi, [edi+6]
        lea            eax, [eax+6]
        lea            ebx, [ebx+6]
        lea            edx, [edx+6]
        lea            ecx, [ecx-1]
        jmp            afs_mie_spot_mmx_loop_q3
afs_mie_spot_mmx_loop:
        lea            ebp, [ebp+24]
        lea            esi, [esi+24]
        lea            edi, [edi+24]
        lea            eax, [eax+24]
        lea            ebx, [ebx+24]
        lea            edx, [edx+24]
        lea            ecx, [ecx-4]

afs_mie_spot_mmx_loop_entry:
        movq        mm0, [esi]
        paddsw        mm0, [eax]
        paddsw        mm0, [ebx]
        paddsw        mm0, [edx]
        paddsw        mm0, mm7
        psraw        mm0, 2
        paddsw        mm0, [ebp]
        paddsw        mm0, mm6
        psraw        mm0, 1
        movq        [edi], mm0

afs_mie_spot_mmx_loop_q2:
        movq        mm0, [esi+8]
        paddsw        mm0, [eax+8]
        paddsw        mm0, [ebx+8]
        paddsw        mm0, [edx+8]
        paddsw        mm0, mm7
        psraw        mm0, 2
        paddsw        mm0, [ebp+8]
        paddsw        mm0, mm6
        psraw        mm0, 1
        movq        [edi+8], mm0

afs_mie_spot_mmx_loop_q3:
        movq        mm0, [esi+16]
        paddsw        mm0, [eax+16]
        paddsw        mm0, [ebx+16]
        paddsw        mm0, [edx+16]
        paddsw        mm0, mm7
        psraw        mm0, 2
        paddsw        mm0, [ebp+16]
        paddsw        mm0, mm6
        psraw        mm0, 1
        movq        [edi+16], mm0

        cmp            ecx,3
        jg            afs_mie_spot_mmx_loop
        je            afs_mie_spot_mmx_loop_last3
        cmp            ecx,1
        jg            afs_mie_spot_mmx_loop_last2
        je            afs_mie_spot_mmx_loop_last1

; exit
afs_mie_spot_mmx_exit:
        emms

        pop            eax
        pop            ebx
        pop            ecx
        pop            edx
        pop            esi
        pop            edi
        pop            ebp

        ret        32

_afs_mie_spot_mmx@32 ENDP

PUBLIC C _afs_mie_inter_mmx@28

;void __stdcall afs_mie_inter_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *src1
;  [esp+12] PIXEL_YC       *src2
;  [esp+16] PIXEL_YC       *src3
;  [esp+20] PIXEL_YC       *src4
;  [esp+24] int            w
;  [esp+28] int            src_frame_size
;)

_afs_mie_inter_mmx@28 PROC
        push        edi
        push        esi
        push        edx
        push        ecx
        push        ebx
        push        eax
; @+24

; setup coefficients

; main loop
        mov            edi, [esp+24+4]                ; [dst]
        mov            esi, [esp+24+8]                ; [src1]
        mov            eax, [esp+24+12]            ; [src2]
        mov            ebx, [esp+24+16]            ; [src3]
        mov            edx, [esp+24+20]            ; [src4]
        mov            ecx, [esp+24+24]            ; [w]
        movq        mm7, pw_round_fix2
        lea            ecx, [ecx-4]
        jmp            afs_mie_inter_mmx_loop_entry

afs_mie_inter_mmx_loop_last3:
        lea            esi, [esi+18]
        lea            edi, [edi+18]
        lea            eax, [eax+18]
        lea            ebx, [ebx+18]
        lea            edx, [edx+18]
        lea            ecx, [ecx-3]
        jmp            afs_mie_inter_mmx_loop_entry
afs_mie_inter_mmx_loop_last2:
        lea            esi, [esi+12]
        lea            edi, [edi+12]
        lea            eax, [eax+12]
        lea            ebx, [ebx+12]
        lea            edx, [edx+12]
        lea            ecx, [ecx-2]
        jmp            afs_mie_inter_mmx_loop_q2
afs_mie_inter_mmx_loop_last1:
        lea            esi, [esi+6]
        lea            edi, [edi+6]
        lea            eax, [eax+6]
        lea            ebx, [ebx+6]
        lea            edx, [edx+6]
        lea            ecx, [ecx-1]
        jmp            afs_mie_inter_mmx_loop_q3
afs_mie_inter_mmx_loop:
        lea            esi, [esi+24]
        lea            edi, [edi+24]
        lea            eax, [eax+24]
        lea            ebx, [ebx+24]
        lea            edx, [edx+24]
        lea            ecx, [ecx-4]

afs_mie_inter_mmx_loop_entry:
        movq        mm0, [esi]
        paddsw        mm0, [eax]
        paddsw        mm0, [ebx]
        paddsw        mm0, [edx]
        paddsw        mm0, mm7
        psraw        mm0, 2
        movq        [edi], mm0

afs_mie_inter_mmx_loop_q2:
        movq        mm0, [esi+8]
        paddsw        mm0, [eax+8]
        paddsw        mm0, [ebx+8]
        paddsw        mm0, [edx+8]
        paddsw        mm0, mm7
        psraw        mm0, 2
        movq        [edi+8], mm0

afs_mie_inter_mmx_loop_q3:
        movq        mm0, [esi+16]
        paddsw        mm0, [eax+16]
        paddsw        mm0, [ebx+16]
        paddsw        mm0, [edx+16]
        paddsw        mm0, mm7
        psraw        mm0, 2
        movq        [edi+16], mm0

        cmp            ecx,3
        jg            afs_mie_inter_mmx_loop
        je            afs_mie_inter_mmx_loop_last3
        cmp            ecx,1
        jg            afs_mie_inter_mmx_loop_last2
        je            afs_mie_inter_mmx_loop_last1

; exit
afs_mie_inter_mmx_exit:
        emms

        pop            eax
        pop            ebx
        pop            ecx
        pop            edx
        pop            esi
        pop            edi

        ret        28

_afs_mie_inter_mmx@28 ENDP


PUBLIC C _afs_deint4_mmx@40

;void __stdcall afs_deint4_mmx(
;  [esp+04] PIXEL_YC       *dst
;  [esp+08] PIXEL_YC       *src1
;  [esp+12] PIXEL_YC       *src3
;  [esp+16] PIXEL_YC       *src4
;  [esp+20] PIXEL_YC       *src5
;  [esp+24] PIXEL_YC       *src7
;  [esp+28] unsigned char  *sip
;  [esp+32] unsigned int   mask
;  [esp+36] int            w
;  [esp+40] int            src_frame_size
;)

_afs_deint4_mmx@40 PROC
        push        ebp
        push        edi
        push        esi
        push        edx
        push        ecx
        push        ebx
        push        eax
; @+28

; setup coefficients

; main loop
        mov            edi, [esp+28+4]                ; [dst]
        mov            edx, [esp+28+28]            ; [sip]
        movd        mm5, dword ptr[esp+28+32]            ; [mask]
        mov            ecx, [esp+28+36]            ; [w]
        lea            ecx, [edx+ecx]
        lea            ecx, [ecx-5]
        mov            sip_limit_5, ecx
        lea            ecx, [ecx-2]
        mov            sip_limit_7, ecx
        mov            ecx, [esp+28+8]                ; [src1]
        mov            eax, [esp+28+12]            ; [src3]
        mov            esi, [esp+28+16]            ; [src4]
        mov            ebx, [esp+28+20]            ; [src5]
        mov            ebp, [esp+28+24]            ; [src7]
        pxor        mm4, mm4
        punpcklbw    mm5, mm4
        movq        mm6, pw_round_fix1
        movq        mm7, dq_mask_center32
        jmp            afs_deint4_mmx_loop_entry

afs_deint4_mmx_loop_last3:
        lea            esi, [esi+18]
        lea            edi, [edi+18]
        lea            eax, [eax+18]
        lea            ebx, [ebx+18]
        lea            ecx, [ecx+18]
        lea            ebp, [ebp+18]
        lea            edx, [edx+3]
        jmp            afs_deint4_mmx_loop_entry
afs_deint4_mmx_loop_last2:
        lea            esi, [esi+12]
        lea            edi, [edi+12]
        lea            eax, [eax+12]
        lea            ebx, [ebx+12]
        lea            ecx, [ecx+12]
        lea            ebp, [ebp+12]
        lea            edx, [edx+2]
        movd        mm0, dword ptr[edx]                    ; mm0 = *sip
        punpcklbw    mm0, mm4
        pand        mm0, mm5
        pcmpeqw        mm0, mm4                    ; mm0 = sip(33221100)
        jmp            afs_deint4_mmx_loop_q2
afs_deint4_mmx_loop_last1:
        lea            esi, [esi+6]
        lea            edi, [edi+6]
        lea            eax, [eax+6]
        lea            ebx, [ebx+6]
        lea            ecx, [ecx+6]
        lea            ebp, [ebp+6]
        lea            edx, [edx+1]
        movd        mm0, dword ptr[edx]                    ; mm0 = *sip
        punpcklbw    mm0, mm4
        pand        mm0, mm5
        pcmpeqw        mm0, mm4                    ; mm0 = sip(33221100)
        jmp            afs_deint4_mmx_loop_q3
afs_deint4_mmx_loop:
        lea            esi, [esi+24]
        lea            edi, [edi+24]
        lea            eax, [eax+24]
        lea            ebx, [ebx+24]
        lea            ecx, [ecx+24]
        lea            ebp, [ebp+24]
        lea            edx, [edx+4]

afs_deint4_mmx_loop_entry:
        movd        mm0, dword ptr[edx]                    ; mm0 = *sip
        punpcklbw    mm0, mm4
        pand        mm0, mm5
        pcmpeqw        mm0, mm4                    ; mm0 = sip(33221100)

        movq        mm1, mm7                    ; mm1 = dq_mask_center32
        movq        mm2, mm0
        punpcklwd    mm2, mm0                    ; mm2 = sip(11110000)
        movq        mm3, mm2
        psllq        mm3, 16
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip(11000000)

        movq        mm2, [ecx]                    ; mm2 = src1
        movq        mm3, [eax]                    ; mm3 = src3
        paddsw        mm2, [ebp]                    ; mm2 += src7
        paddsw        mm3, [ebx]                    ; mm3 += src5
        psubsw        mm2, mm3                    ; mm2 = (src1+src7) - (src3+src5)
        psraw        mm2, 3
        psubsw        mm3, mm2
        paddsw        mm3, mm6                    ; mm3 += 1
        psraw        mm3, 1                        ; mm3 = ((src3+src5) - ((src1+src7)-(src3+src5))/8 + 1)/2
        movq        mm2, [esi]                    ; mm2 = src4
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip ? mm3 : mm2
        movq        [edi], mm1

afs_deint4_mmx_loop_q2:
        movq        mm1, mm0
        psrlq        mm1, 16
        movq        mm2, mm1
        punpcklwd    mm1, mm2                    ; mm1 = sip(22221111)

        movq        mm2, [ecx+8]                ; mm2 = src1
        movq        mm3, [eax+8]                ; mm3 = src3
        paddsw        mm2, [ebp+8]                ; mm2 += src7
        paddsw        mm3, [ebx+8]                ; mm3 += src5
        psubsw        mm2, mm3                    ; mm2 = (src1+src7) - (src3+src5)
        psraw        mm2, 3
        psubsw        mm3, mm2
        paddsw        mm3, mm6                    ; mm3 += 1
        psraw        mm3, 1                        ; mm3 = ((src3+src5) - ((src1+src7)-(src3+src5))/8 + 1)/2
        movq        mm2, [esi+8]                ; mm2 = src4
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip ? mm3 : mm2
        movq        [edi+8], mm1

afs_deint4_mmx_loop_q3:
        movq        mm1, mm7                    ; mm1 = dq_mask_center32
        movq        mm2, mm0
        punpckhwd    mm2, mm0                    ; mm2 = sip(33332222)
        movq        mm3, mm2
        psrlq        mm3, 16
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip(33333322)

        movq        mm2, [ecx+16]                ; mm2 = src1
        movq        mm3, [eax+16]                ; mm3 = src3
        paddsw        mm2, [ebp+16]                ; mm2 += src7
        paddsw        mm3, [ebx+16]                ; mm3 += src5
        psubsw        mm2, mm3                    ; mm2 = (src1+src7) - (src3+src5)
        psraw        mm2, 3
        psubsw        mm3, mm2
        paddsw        mm3, mm6                    ; mm3 += 1
        psraw        mm3, 1                        ; mm3 = ((src3+src5) - ((src1+src7)-(src3+src5))/8 + 1)/2
        movq        mm2, [esi+16]                ; mm2 = src4
        pand        mm3, mm1
        pandn        mm1, mm2
        por            mm1, mm3                    ; mm1 = sip ? mm3 : mm2
        movq        [edi+16], mm1

        cmp            sip_limit_7, edx
        jg            afs_deint4_mmx_loop
        je            afs_deint4_mmx_loop_last3
        cmp            sip_limit_5, edx
        jg            afs_deint4_mmx_loop_last2
        je            afs_deint4_mmx_loop_last1

; exit
afs_deint4_mmx_exit:
        emms

        pop            eax
        pop            ebx
        pop            ecx
        pop            edx
        pop            esi
        pop            edi
        pop            ebp

        ret        40

_afs_deint4_mmx@40 ENDP

END
