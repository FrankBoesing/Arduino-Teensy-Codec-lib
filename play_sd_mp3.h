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
 6790´tzui#
	Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
	Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.

	Der Helixdecoder selbst hat eine eigene Lizenz, bitte für mehr Informationen
	in den Unterverzeichnissen nachsehen.

 */

 /* The Helix-Library is modified for Teensy 3.1 */

#ifndef play_sd_mp3_h_
#define play_sd_mp3_h_

#include "codecs.h"
#include "AudioStream.h"
#include "spi_interrupt.h"
#include "mp3/mp3dec.h"

class AudioPlaySdMp3 : public AudioCodec
{
public:
	void stop(void);
	int play(FS *fs, const char *filename) {stop(); if (!fopen(fs,filename)) return ERR_CODEC_FILE_NOT_FOUND; return play();}
	int play(const char *filename) {stop();if (!fopen(filename)) return ERR_CODEC_FILE_NOT_FOUND; return play();}
	int play(const size_t p, const size_t size) {stop();if (!fopen(p,size)) return ERR_CODEC_FILE_NOT_FOUND; return play();}
	int play(const uint8_t*p, const size_t size) {stop();if (!fopen(p,size))  return ERR_CODEC_FILE_NOT_FOUND; return play();}

protected:
	uint8_t			*sd_buf;
	uint8_t			*sd_p;
	int				sd_left;

	short			*buf[2];
	size_t			decoded_length[2];
	size_t			decoding_block;
	unsigned int	decoding_state; //state 0: read sd, state 1: decode

	size_t 	  		size_id3;
	uintptr_t 		play_pos;

	HMP3Decoder		hMP3Decoder;
	MP3FrameInfo	mp3FrameInfo;

	int play(void);
	void update(void);
	friend void decodeMp3(void);
};


#endif
