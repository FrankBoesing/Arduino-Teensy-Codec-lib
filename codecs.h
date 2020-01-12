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

#if !defined(__MK20DX256__) && !defined(__MK64FX512__) && !defined(__MK66FX1M0__) && !defined(__IMXRT1052__) && !defined(__IMXRT1062__)
#error	This platform is not supported.
#endif

#ifndef codecs_h_
#define codecs_h_

#include <Arduino.h>
#include <AudioStream.h>
#include <spi_interrupt.h>

#define ERR_CODEC_NONE				0
#define ERR_CODEC_FILE_NOT_FOUND    1
#define ERR_CODEC_OUT_OF_MEMORY     2
#define ERR_CODEC_FORMAT			3	//File is not 44.1 KHz, 16Bit mono or stereo

#define IRQ_AUDIO			IRQ_SOFTWARE	// see AudioStream.cpp

#if defined(__IMXRT1052__) && !defined(__IMXRT1062__)
#define IRQ_AUDIOCODEC		IRQ_Reserved1
#else
#define IRQ_AUDIOCODEC		55				// use a "reserved" (free) interrupt vector
#endif

#define IRQ_AUDIOCODEC_PRIO	240				// lowest priority

#define AUDIOCODECS_SAMPLE_RATE			(((int)(AUDIO_SAMPLE_RATE / 100)) * 100) //44100

#define NVIC_STIR			(*(volatile uint32_t *)0xE000EF00) //Software Trigger Interrupt Register
#define NVIC_TRIGGER_INTERRUPT(x)    NVIC_STIR=(x)
#define NVIC_IS_ACTIVE(n)	(*((volatile uint32_t *)0xE000E300 + ((n) >> 5)) & (1 << ((n) & 31)))

#define PATCH_PRIO 			{if (NVIC_GET_PRIORITY(IRQ_AUDIO) == IRQ_AUDIOCODEC_PRIO) {NVIC_SET_PRIORITY(IRQ_AUDIO, IRQ_AUDIOCODEC_PRIO-16);}}

#define SERFLASH_CS 				6	//Chip Select W25Q128FV SPI Flash
#define SPICLOCK 			30000000

extern "C" { void memcpy_frominterleaved(int16_t *dst1, int16_t *dst2, int16_t *src); }
size_t skipID3(uint8_t *sd_buf);

enum codec_filetype {codec_none, codec_file, codec_flash, codec_serflash};
enum codec_playstate {codec_stopped, codec_playing, codec_paused};

/**
 * The CodecFileBase represents an open file. It needs to implement methods to access the file
 */
class CodecFileBase
{
public:
	virtual bool   f_eof(void);
	virtual bool   fseek(const size_t position);
	virtual size_t fposition(void);
	virtual size_t fsize(void);
	virtual size_t fread(uint8_t buffer[],size_t bytes);
};

class AudioCodec : public AudioStream
{
public:

	AudioCodec(void) : AudioStream(0, NULL) {initVars();}

	int play(CodecFileBase *file) {stop(); currentFile = file; return play();}
	bool pause(const bool paused);
	virtual void stop() = 0;
	virtual int play() = 0;
	bool isPlaying(void) {return playing > 0;}
	unsigned positionMillis(void) { return (AUDIO_SAMPLE_RATE_EXACT / 1000) * samples_played;}
	unsigned lengthMillis(void) {return max(fsize() / (bitrate / 8 ) * 1000,  positionMillis());} //Ignores VBR
	int channels(void) {return _channels;}
	int bitRate(void) {return bitrate;}
	void processorUsageMaxResetDecoder(void){__disable_irq();decode_cycles_max = decode_cycles_max_read = 0;__enable_irq();}
	int freeRam(void);

	uint8_t *allocBuffer(size_t size) { rdbufsize = size;  bufptr = (uint8_t *) calloc(size,1); return bufptr;}
	void freeBuffer(void){ if (bufptr !=NULL) {free(bufptr);bufptr = NULL; } rdbufsize = 0;}
	size_t fillReadBuffer(uint8_t *sd_buf, uint8_t *data, size_t dataLeft, size_t sd_bufsize);
	//size_t fillReadBuffer(uint8_t *data, size_t dataLeft);

	static short	lastError;

	static unsigned decode_cycles;
	static unsigned	decode_cycles_read;
	static unsigned decode_cycles_max;
	static unsigned decode_cycles_max_read;

protected:

	CodecFileBase	*currentFile;

	unsigned		samples_played;

	unsigned short	_channels;
	unsigned short	bitrate;

	volatile codec_playstate playing;

	void initVars(void) {samples_played=_channels=bitrate=decode_cycles=decode_cycles_read=decode_cycles_max=decode_cycles_max_read = 0;playing=codec_stopped;}
	void initSwi(void) {PATCH_PRIO;NVIC_SET_PRIORITY(IRQ_AUDIOCODEC, IRQ_AUDIOCODEC_PRIO);NVIC_ENABLE_IRQ(IRQ_AUDIOCODEC);}

	// These are just convenience functions that redirect to the corresponding functions on currentFile
	bool   f_eof(void) {return currentFile->f_eof();}
	bool   fseek(const size_t position) {return currentFile->fseek(position);}
	size_t fposition(void) {return currentFile->fposition();}
	size_t fsize(void) {return currentFile->fsize();}
	size_t fread(uint8_t buffer[],size_t bytes) {return currentFile->fread(buffer, bytes);}

	uint8_t* bufptr;
	size_t rdbufsize;
};

#endif
