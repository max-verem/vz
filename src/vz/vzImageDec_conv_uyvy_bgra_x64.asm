;    ViZualizator
;    (Real-Time TV graphics production system)
;
;    Copyright (C) 2011 Maksym Veremeyenko.
;    This file is part of ViZualizator (Real-Time TV graphics production system).
;    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2011.
;
;    ViZualizator is free software; you can redistribute it and/or modify
;    it under the terms of the GNU General Public License as published by
;    the Free Software Foundation; either version 2 of the License, or
;    (at your option) any later version.
;
;    ViZualizator is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;    GNU General Public License for more details.
;
;    You should have received a copy of the GNU General Public License
;    along with ViZualizator; if not, write to the Free Software
;    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

.const

sr      equ     6
dr      equ     16 ; 2 ^(10 - sr)

kYv     equ     (1192 + dr - 1)/dr ; 1.164 * 1024
kCrRv   equ     (1634 + dr - 1)/dr ; 1.596 * 1024
kCrGv   equ     (-832 + dr - 1)/dr ; -0.813 * 1024
kCbGv   equ     (-400 + dr - 1)/dr ; -0.391 * 1024
kCbBv   equ     (2066 + dr - 1)/dr ; 2.018 * 1024

xmm_mask1 equ xmm15
xmm_c16 equ xmm14
xmm_c128 equ xmm13
xmm_kY equ xmm12
xmm_kCrR equ xmm11
xmm_kCrG equ xmm10
xmm_kCbG equ xmm9
xmm_kCbB equ xmm8
xmm_zero equ xmm7

.data
mask1   db      0ffh, 0, 0ffh, 0, 0ffh, 0, 0ffh, 0, 0ffh, 0, 0ffh, 0, 0ffh, 0, 0ffh, 0
kY      dw      kYv, kYv, kYv, kYv, kYv, kYv, kYv, kYv
kCrR    dw      kCrRv, kCrRv, kCrRv, kCrRv, kCrRv, kCrRv, kCrRv, kCrRv
kCrG    dw      kCrGv, kCrGv, kCrGv, kCrGv, kCrGv, kCrGv, kCrGv, kCrGv
kCbG    dw      kCbGv, kCbGv, kCbGv, kCbGv, kCbGv, kCbGv, kCbGv, kCbGv
kCbB    dw      kCbBv, kCbBv, kCbBv, kCbBv, kCbBv, kCbBv, kCbBv, kCbBv
c16     dw      16, 16, 16, 16, 16, 16, 16, 16
c128    dw      128, 128, 128, 128, 128, 128, 128, 128

.const
src equ rcx
dst equ rdx
rows equ r8
cols equ r9
delta_src_pitch equ r10
delta_dst_pitch equ r11
cols_cnt equ rax
.code
; void conv_uyvy_bgra(void* src, void* dst, int rows, int cols, int delta_src_pitch, int delta_dst_pitch)
conv_uyvy_bgra PROC
; save stack
    PUSH rbp
    MOV rbp, rsp
    SUB rsp, 0100h

; save non-volatile xmm register
    MOVDQA [rsp + 00h], xmm_zero
    MOVDQA [rsp + 10h], xmm_kCbB
    MOVDQA [rsp + 30h], xmm_kCbG
    MOVDQA [rsp + 40h], xmm_kCrG
    MOVDQA [rsp + 50h], xmm_kCrR
    MOVDQA [rsp + 60h], xmm_kY
    MOVDQA [rsp + 70h], xmm_c128
    MOVDQA [rsp + 80h], xmm_c16
    MOVDQA [rsp + 90h], xmm_mask1

; load pitches
    MOV delta_src_pitch, qword ptr[rbp + 48]
    MOV delta_dst_pitch, qword ptr[rbp + 56]

; load constants
    PXOR xmm_zero, xmm_zero
    MOVAPD xmm_mask1, XMMWORD PTR mask1
    MOVAPD xmm_c16, XMMWORD PTR c16
    MOVAPD xmm_c128, XMMWORD PTR c128
    MOVAPD xmm_kY, XMMWORD PTR kY
    MOVAPD xmm_kCrR, XMMWORD PTR kCrR
    MOVAPD xmm_kCrG, XMMWORD PTR kCrG
    MOVAPD xmm_kCbG, XMMWORD PTR kCbG
    MOVAPD xmm_kCbB, XMMWORD PTR kCbB

; rows loop
rows_loop_in:

; cols loop
    MOV cols_cnt, cols
cols_loop_in:

; load pixels
    MOVAPD xmm0, XMMWORD PTR [src]
    ADD src, 16

    PSHUFLW xmm2, xmm0, 010100000b
    PSHUFLW xmm3, xmm0, 011110101b

    PSHUFHW xmm4, xmm2, 010100000b
    PSHUFHW xmm5, xmm3, 011110101b

    PSRLDQ xmm0, 1

    PAND   xmm4, xmm_mask1
    PAND   xmm5, xmm_mask1
    PAND   xmm0, xmm_mask1

; Y  plane in xmm0
; Cr plane in xmm5
; Cb plane in xmm4

    PSUBW  xmm0, xmm_c16
    PSUBW  xmm4, xmm_c128
    PSUBW  xmm5, xmm_c128

    MOVAPD xmm2, xmm4 ; Cb
    MOVAPD xmm3, xmm5 ; Cr

    PMULLW xmm0, xmm_kY
    PMULLW xmm3, xmm_kCrR
    PMULLW xmm5, xmm_kCrG
    PMULLW xmm2, xmm_kCbG
    PMULLW xmm4, xmm_kCbB

; R plane
    PADDW  xmm3, xmm0
; B plane
    PADDW  xmm4, xmm0
; G plane
    PADDW  xmm0, xmm5
    PADDW  xmm0, xmm2

; shift for normal prec
    PSRAW  xmm3, sr; R
    PSRAW  xmm4, sr; B
    PSRAW  xmm0, sr; G

; clamp for max value 0xFF
    PMINSW xmm3, xmm_mask1
    PMINSW xmm4, xmm_mask1
    PMINSW xmm0, xmm_mask1

; clamp for min value 0
    PMAXSW xmm3, xmm_zero
    PMAXSW xmm4, xmm_zero
    PMAXSW xmm0, xmm_zero

; lower part
; --------------------------

; unpack alpha
    MOVAPD xmm1, xmm_mask1
    PUNPCKLBW xmm1, xmm_zero

; unpack RED
    MOVAPD xmm2, xmm3
    PUNPCKLBW xmm2, xmm_zero
; put
    PSLLDQ xmm1, 1
    PXOR xmm1, xmm2

; unpack GREEN
    MOVAPD xmm2, xmm0
    PUNPCKLBW xmm2, xmm_zero
; put
    PSLLDQ xmm1, 1
    PXOR xmm1, xmm2

; unpack BLUE
    MOVAPD xmm2, xmm4
    PUNPCKLBW xmm2, xmm_zero
; put
    PSLLDQ xmm1, 1
    PXOR xmm1, xmm2

; --------------------------

; save pixels
    MOVAPD XMMWORD PTR [dst], xmm1
    ADD dst, 16

; higher part
; --------------------------

; unpack alpha
    MOVAPD xmm1, xmm_mask1
    PUNPCKHBW xmm1, xmm_zero

; unpack RED
    MOVAPD xmm2, xmm3
    PUNPCKHBW xmm2, xmm_zero
; put
    PSLLDQ xmm1, 1
    PXOR xmm1, xmm2

; unpack GREEN
    MOVAPD xmm2, xmm0
    PUNPCKHBW xmm2, xmm_zero
; put
    PSLLDQ xmm1, 1
    PXOR xmm1, xmm2

; unpack BLUE
    MOVAPD xmm2, xmm4
    PUNPCKHBW xmm2, xmm_zero
; put
    PSLLDQ xmm1, 1
    PXOR xmm1, xmm2

; --------------------------

; save pixels
    MOVAPD XMMWORD PTR [dst], xmm1
    ADD dst, 16

; check if col finished
    SUB cols_cnt, 8
    TEST cols_cnt, cols_cnt
    JNZ cols_loop_in

; add delta pitches
    ADD src, delta_src_pitch
    ADD dst, delta_dst_pitch
    DEC rows
    TEST rows, rows
    JNZ rows_loop_in

; restore non-volatile xmm register
    MOVDQA xmm_zero, [rsp + 00h]
    MOVDQA xmm_kCbB, [rsp + 10h]
    MOVDQA xmm_kCbG, [rsp + 30h]
    MOVDQA xmm_kCrG, [rsp + 40h]
    MOVDQA xmm_kCrR, [rsp + 50h]
    MOVDQA xmm_kY, [rsp + 60h]
    MOVDQA xmm_c128, [rsp + 70h]
    MOVDQA xmm_c16, [rsp + 80h]
    MOVDQA xmm_mask1, [rsp + 90h]

; restore stack pointer
    MOV rsp, rbp
    POP rbp

    RET

conv_uyvy_bgra ENDP

end
