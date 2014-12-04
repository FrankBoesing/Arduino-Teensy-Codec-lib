/*
	Helix library Arduino interface

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

	push	{r0, r3-r12}
	mov r12, #4 - 1
	.align 2	
.loop:

	.irp offset, 1,2,3,4	
	
	//load 8 words (32 byte)  at once into r3-r10 increase adress of r2
	ldmia r2!,{r3, r4, r5, r6, r7, r8, r9, r10}

	pkhbt r11, r3, r4, lsl #0
	str r11,[r0],#4
	pkhbt r11, r5, r6, lsl #0
	str r11,[r0],#4
	pkhbt r11, r7, r8, lsl #0
	str r11,[r0],#4
	pkhbt r11, r9, r10, lsl #0	
	str r11,[r0],#4
	
	pkhbt r11, r4, r3, lsl #0
	str r11,[r1],#4
	pkhbt r11, r6, r5, lsl #0
	str r11,[r1],#4
	pkhbt r11, r8, r7, lsl #0
	str r11,[r1],#4
	pkhbt r11, r10, r9, lsl #0
	str r11,[r1],#4
	
	.endr
	
	subs r12, #1
	bhs .loop
	
	pop	{r0, r3-r12}

	BX lr
.END