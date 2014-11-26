/*
	Helix library Arduino Audio Library MP3/AAC objects

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

 /* The Helix-Library is modified for Teensy 3.1 */
#ifndef TEENSYDUINO
#error	This platform is not supported.
#endif

#ifndef mp3aac_h_
#define mp3aac_h_

#include "SD.h"

#define IRQ_AUDIO		IRQ_SOFTWARE	// see AudioStream.cpp
#define IRQ_AUDIO2		56				// use a "reserved" (free) interrupt vector

extern File			file;

extern uint8_t 		*sd_buf;
extern uint8_t		*sd_p;
extern int			sd_left;
extern uint32_t 	size_id3;

extern int16_t 		*buf[2];
extern uint32_t		decoded_length[2];
extern int32_t		decoding_block;
extern int32_t 		play_pos;
extern uint32_t	    samples_played;
extern int			playing;

extern uint32_t		decode_cycles_max;
extern uint32_t		decode_cycles_max_sd;

void init_interrupt( void (*decoder)(void) );

unsigned int fillReadBuffer(File file, uint8_t *sd_buf, uint8_t *data, uint32_t dataLeft, uint32_t sd_bufsize);
unsigned int skipID3(uint8_t *sd_buf);

#endif