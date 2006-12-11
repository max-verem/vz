/*
    ViZualizator
    (Real-Time TV graphics production system)

    Copyright (C) 2005 Maksym Veremeyenko.
    This file is part of ViZualizator (Real-Time TV graphics production system).
    Contributed by Maksym Veremeyenko, verem@m1stereo.tv, 2005.

    ViZualizator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ViZualizator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ViZualizator; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

static int sse_supported = 0;

void inline arhc_sse2_detect(void)
{
	/* detect SSE2 support */
	__asm
	{
		mov		eax, 1
		cpuid
		test	edx, 4000000h		/* test bit 26 (SSE2) */
		jz		no_sse2
		mov		dword ptr[sse_supported], 1
no_sse2:
	};
};

#define DUPL_LINES_ARCH_SSE2_DETECT arhc_sse2_detect();

void inline copy_data_twice_mmx(void* dst,void *src)
{
	__asm
	{
		// loading source surface pointer
		mov		esi, dword ptr[src];

		// loading dst surface pointer
		mov		edi, dword ptr[dst];

		// counter
		mov		ecx, 45;

		/* load offset */
		mov		eax, 720*4

		// loop data copy
cp:
		movq		MM0, qword ptr[esi]
		movq		MM1, qword ptr[esi+8]
		movq		MM2, qword ptr[esi+16]
		movq		MM3, qword ptr[esi+24]
		movq		MM4, qword ptr[esi+32]
		movq		MM5, qword ptr[esi+40]
		movq		MM6, qword ptr[esi+48]
		movq		MM7, qword ptr[esi+56]
		add			esi,64

		/* save block copy #1 */
		movq		qword ptr[edi],MM0
		movq		qword ptr[edi+8],MM1
		movq		qword ptr[edi+16],MM2
		movq		qword ptr[edi+24],MM3
		movq		qword ptr[edi+32],MM4
		movq		qword ptr[edi+40],MM5
		movq		qword ptr[edi+48],MM6
		movq		qword ptr[edi+56],MM7
		
		push		edi							/* save dest counter */
		add			edi, eax					/* add offset */

		/* save block copy #2 */
		movq		qword ptr[edi],MM0
		movq		qword ptr[edi+8],MM1
		movq		qword ptr[edi+16],MM2
		movq		qword ptr[edi+24],MM3
		movq		qword ptr[edi+32],MM4
		movq		qword ptr[edi+40],MM5
		movq		qword ptr[edi+48],MM6
		movq		qword ptr[edi+56],MM7


		pop			edi							/* restore dest counter */
		add			edi,64						/* increment */
		loop		cp;


		emms
	}

	return;

};

/* 
http://www.hayestechnologies.com/en/techsimd.htm#SSE2
http://www.jorgon.freeserve.co.uk/TestbugHelp/XMMfpins2.htm

*/


void inline copy_data_twice_sse(void* dst,void *src)
{
	__asm
	{
		// loading source surface pointer
		mov		esi, dword ptr[src];

		// loading dst surface pointer
		mov		edi, dword ptr[dst];

		// counter
		mov		ecx, 22

		/* load offset */
		mov		eax, 720*4


		// loop data copy
cp1:
		movupd		xmm0, [esi +   0]
		movupd		xmm1, [esi +  16]
		movupd		xmm2, [esi +  32]
		movupd		xmm3, [esi +  48]
		movupd		xmm4, [esi +  64]
		movupd		xmm5, [esi +  80]
		movupd		xmm6, [esi +  96]
		movupd		xmm7, [esi + 112]
		add			esi, 128

		/* save block copy #1 */
		movupd		[edi +   0], xmm0
		movupd		[edi +  16], xmm1
		movupd		[edi +  32], xmm2
		movupd		[edi +  48], xmm3
		movupd		[edi +  64], xmm4
		movupd		[edi +  80], xmm5
		movupd		[edi +  96], xmm6
		movupd		[edi + 112], xmm7
		
		push		edi							/* save dest counter */
		add			edi, eax					/* add offset */

		/* save block copy #2 */
		movupd		[edi +   0], xmm0
		movupd		[edi +  16], xmm1
		movupd		[edi +  32], xmm2
		movupd		[edi +  48], xmm3
		movupd		[edi +  64], xmm4
		movupd		[edi +  80], xmm5
		movupd		[edi +  96], xmm6
		movupd		[edi + 112], xmm7

		jmp			cp3
cp2:
		jmp			cp1
cp3:
		pop			edi							/* restore dest counter */
		add			edi,128						/* increment */
		loop		cp2

		/* load last block */
		movupd		xmm0, [esi +   0]
		movupd		xmm1, [esi +  16]
		movupd		xmm2, [esi +  32]
		movupd		xmm3, [esi +  48]

		/* save block copy #1 */
		movupd		[edi +   0], xmm0
		movupd		[edi +  16], xmm1
		movupd		[edi +  32], xmm2
		movupd		[edi +  48], xmm3

		add			edi, eax					/* add offset */

		/* save block copy #2 */
		movupd		[edi +   0], xmm0
		movupd		[edi +  16], xmm1
		movupd		[edi +  32], xmm2
		movupd		[edi +  48], xmm3


		emms
	}

	return;

};
