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

xmm_pix_src equ xmm0
xmm_pix_dst equ xmm1
xmm_mask1 equ xmm2

.data
mask1   db      0, 0, 0, 0ffh, 0, 0, 0, 0ffh, 0, 0, 0, 0ffh, 0, 0, 0, 0ffh

.const
src equ rcx
dst equ rdx
rows equ r8
cols equ r9
delta_src_pitch equ r10
delta_dst_pitch equ r11
cols_cnt equ rax
.code
; void conv_bgra_aaaa(void* src, void* dst, int rows, int cols, int delta_src_pitch, int delta_dst_pitch)
conv_bgra_aaaa PROC
; save stack
    PUSH rbp
    MOV rbp, rsp
    SUB rsp, 0100h

; save non-volatile xmm register
    MOVDQA [rsp + 00h], xmm_pix_src
    MOVDQA [rsp + 10h], xmm_pix_dst
    MOVDQA [rsp + 30h], xmm_mask1

; load pitches
    MOV delta_src_pitch, qword ptr[rbp + 48]
    MOV delta_dst_pitch, qword ptr[rbp + 56]

; load constants
    MOVAPD xmm_mask1, XMMWORD PTR mask1

; rows loop
rows_loop_in:

; cols loop
    MOV cols_cnt, cols
cols_loop_in:

; load pixels
    MOVAPD xmm_pix_src, XMMWORD PTR [src]
    ADD src, 16

    PAND xmm_pix_src, xmm_mask1

    MOVAPD xmm_pix_dst, xmm_pix_src

    PSRLDQ xmm_pix_src, 1
    PXOR xmm_pix_dst, xmm_pix_src

    PSRLDQ xmm_pix_src, 1
    PXOR xmm_pix_dst, xmm_pix_src

    PSRLDQ xmm_pix_src, 1
    PXOR xmm_pix_dst, xmm_pix_src

; save pixels
    MOVAPD XMMWORD PTR [dst], xmm_pix_dst
    ADD dst, 16

; check if col finished
    SUB cols_cnt, 4
    TEST cols_cnt, cols_cnt
    JNZ cols_loop_in

; add delta pitches
    ADD src, delta_src_pitch
    ADD dst, delta_dst_pitch
    DEC rows
    TEST rows, rows
    JNZ rows_loop_in

; restore non-volatile xmm register
    MOVDQA xmm_pix_src, [rsp + 00h]
    MOVDQA xmm_pix_dst, [rsp + 10h]
    MOVDQA xmm_mask1, [rsp + 30h]

; restore stack pointer
    MOV rsp, rbp
    POP rbp

    RET

conv_bgra_aaaa ENDP

end
