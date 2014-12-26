/*
	Arduino Audiocodecs

	Copyright (c) 2014 Frank Bösing

	This library is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this library.  If not, see <http://www.gnu.org/licenses/>.

	The helix decoder itself as a different license, look at the subdirectories for more info.

	Diese Bibliothek ist freie Software: Sie können es unter den Bedingungen
	der GNU General Public License, wie von der Free Software Foundation,
	Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
	veröffentlichten Version, weiterverbreiten und/oder modifizieren.

	Diese Bibliothek wird in der Hoffnung, dass es nützlich sein wird, aber
	OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
	Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
	Siehe die GNU General Public License für weitere Details.

	Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
	Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.

	Der Helixdecoder selbst hat eine eigene Lizenz, bitte für mehr Informationen
	in den Unterverzeichnissen nachsehen.

 */

//Fast copy decoded data (interleaved LRLRLR) to the two audiobuffers (LLL, RRR)
 
#if TEENSYDUINO < 121 
#warning Audiomemory might not be aligned(4). Can cause a crash. See memcpy_frominterleaved.S for more Info
#endif
/*

To Fix this warning:
1) Change typedef struct audio_block_struct in AudioStream.h to  :

typedef struct audio_block_struct {
	unsigned char ref_count;
	unsigned char memory_pool_index;
	unsigned char reserve1; // Add this Line
	unsigned char reserve2; // Add this Line
	int16_t data[AUDIO_BLOCK_SAMPLES];
} audio_block_t;

2) Comment the warning with "//"

The reason is, that audio_block_struct is not alignable(4) without these lines.
Unaligned accesses between upper/lower ram are not supported by teensy-hardware.
It's possible (and i've seen this) that an audioblock is located exactly at this border.

*/

.cpu cortex-m4
.syntax unified
.thumb
.text
.align	2


/* void memcpy_frominterleaved(short *dst1, short *dst2, short *src); */
 .global	memcpy_frominterleaved
.thumb_func	
	memcpy_frominterleaved:

	@ r0: dst1
	@ r1: dst2
	@ r2: src

	push	{r3-r12,r14}
	add r14, r2, #512
	.align 2	
.loop:

	.irp offset, 1,2	
	
	//load 8 words (32 byte)  at once into r3-r10 increase adress of r2
	ldmia r2!,{r5, r6, r7, r8, r9, r10, r11, r12}

	pkhbt r3, r5, r6
	pkhbt r4, r6, r5
	
	pkhbt r5, r7, r8
	pkhbt r6, r8, r7

	pkhbt r7, r9, r10
	pkhbt r8, r10, r9

	pkhbt r9, r11, r12
	pkhbt r10, r12, r11

	stmia r0!,{r3,r5,r7,r9}
	stmia r1!,{r4,r6,r8,r10}	
	
	.endr

	cmp r14, r2
	bne .loop

	pop	{r3-r12,r14}

	BX lr
.END