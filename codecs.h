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

#ifndef __MK20DX256__
#error	This platform is not supported.
#endif

#ifndef codecs_h_
#define codecs_h_



#include "AudioStream.h"
#include "SD.h"

#define ERR_CODEC_NONE				0
#define ERR_CODEC_FILE_NOT_FOUND    1
#define ERR_CODEC_OUT_OF_MEMORY     2
#define ERR_CODEC_FORMAT			3	//File is not 44.1 KHz, 16Bit mono or stereo


#define IRQ_AUDIO			IRQ_SOFTWARE	// see AudioStream.cpp
#define IRQ_AUDIOCODEC		56				// use a "reserved" (free) interrupt vector
#define IRQ_AUDIOCODEC_PRIO	0XFF			// use a "reserved" (free) interrupt vector

#define AUDIOCODECS_SAMPLE_RATE			(((int)(AUDIO_SAMPLE_RATE / 100)) * 100) //44100

#define NVIC_STIR			(*(volatile uint32_t *)0xE000EF00) //Software Trigger Interrupt Register
#define NVIC_TRIGGER_INTERRUPT(x)    NVIC_STIR=(x)
#define NVIC_IS_ACTIVE(n)	(*((volatile uint32_t *)0xE000E300 + ((n) >> 5)) & (1 << ((n) & 31)))

#ifdef __cplusplus
extern "C" {
#endif
void memcpy_frominterleaved(short *dst1, short *dst2, short *src);
#ifdef __cplusplus
}
#endif

extern int lastError;

void init_interrupt();
size_t fillReadBuffer(File file, uint8_t *sd_buf, uint8_t *data, size_t dataLeft, size_t sd_bufsize);
uint16_t fread16(File file, size_t position);
uint32_t fread32(File file, size_t position);
size_t skipID3(uint8_t *sd_buf);

class AudioCodec : public AudioStream
{
public:
	AudioCodec(void) : AudioStream(0, NULL) {};
	int	getLastError(void);
protected:

};

#endif