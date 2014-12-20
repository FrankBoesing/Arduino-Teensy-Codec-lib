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


#ifndef play_sd_flac_h_
#define play_sd_flac_h_

#include "codecs.h"
#include "AudioStream.h"
#include "spi_interrupt.h"
#include "flac/all.h"
#include "audiobuffer.h"

#define FLAC_USE_SWI


#define FLAC_BUFFERS(x)  (x*2)

class AudioPlaySdFlac : public AudioCodec
{
public:
	AudioPlaySdFlac(void){};
	int play(const char *filename);
	bool pause(bool paused);
	void stop(void);
	bool isPlaying(void);

	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	uint32_t bitrate(void);

	void processorUsageMaxResetDecoder(void);
	float processorUsageMaxDecoder(void);
	float processorUsageMaxSD(void);

//protected:
	File		file;
	uint32_t	samples_played = 0;

	uint16_t	minbuffers = 0;
	uint16_t 	playing = 0;
	uint16_t	channels = 0;

	AudioBuffer *audiobuffer;

	static FLAC__StreamDecoder	*hFLACDecoder ;
	static uint32_t	decode_cycles_max;
	static uint32_t	decode_cycles_max_sd;
	static uint32_t	decode_cycles_sd;

	friend FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data);
	friend FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
    friend FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
    friend FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
	friend FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
	friend FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
	friend void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

	void update(void);
	friend void decode(void) ;

//private:
};

#endif
