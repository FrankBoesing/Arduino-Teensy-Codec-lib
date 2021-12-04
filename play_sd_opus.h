/*
  Arduino Audiocodecs

  Copyright (c) 2020 jcj83429

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
*/

#include "codecs.h"
#include "AudioStream.h"
#include "oggparse.h"
#include "opus/include/opus.h"

class AudioPlaySdOpus : public OggStreamReader
{
public:
	int play(FS *fs, const char *filename) {stop(); if (!fopen(fs,filename)) return ERR_CODEC_FILE_NOT_FOUND; return play();}
	int play(const char *filename) {stop();if (!fopen(filename)) return ERR_CODEC_FILE_NOT_FOUND; return play();}
	int play(const size_t p, const size_t size) {stop();if (!fopen(p,size)) return ERR_CODEC_FILE_NOT_FOUND; return play();}
	int play(const uint8_t*p, const size_t size) {stop();if (!fopen(p,size))  return ERR_CODEC_FILE_NOT_FOUND; return play();}
	void stop(void);
	bool seek(uint32_t);

protected:

	OpusDecoder     *opusDecoder = NULL;
	
	uint8_t			*bitstream_buf = NULL;
	// because opus frame sizes are not a multiple of 128, we decode into this third buffer and copy to the 2 buffers, which are multiples of 128
	int16_t			*decbuf = NULL;
	int16_t			decbuflen = 0;
	int16_t			*buf[2] = {0};
	int16_t			decoded_length[2];
	size_t			decoding_block;
	unsigned int	decoding_state; //state 0: read sd, state 1: decode

	uintptr_t		play_pos;

	float output_gain = 0;

	int play(void);
	void update(void);
	void fill_buf_from_decbuf(void);
	bool parse_opus_header(void);
	bool parse_metadata_tags(void);
	friend void decodeOpus(void);
}; 
