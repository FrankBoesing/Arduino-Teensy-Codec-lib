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

 /* The Helix-Library is modified for Teensy 3.1 */

#ifndef TEENSYDUINO
#error	This platform is not supported.
#endif

#ifndef play_sd_aac_h_
#define play_sd_aac_h_

#include "codecs.h"
#include "AudioStream.h"
#include "spi_interrupt.h"
#include "aac/aacdec.h"

/* todo
#define ERR_HAAC_NONE   	  	   0
#define ERR_HAAC_FILE_NOT_FOUND    1
#define ERR_HAAC_OUT_OF_MEMORY     2
#define ERR_HAAC_FORMAT			   3	//File is not 44.1 KHz, 16Bit mono or stereo
*/


struct _ATOM		{unsigned int position;unsigned int size;};
struct _ATOMINFO	{uint32_t size;char name[4];};


class AudioPlaySdAac : public AudioStream
{
public:
	AudioPlaySdAac(void) : AudioStream(0, NULL) { stop(); }
	bool play(const char *filename) ;
	bool pause(bool paused);
	void stop(void);
	bool isPlaying(void);
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	uint32_t bitrate(void);

	void processorUsageMaxResetDecoder(void);
	float processorUsageMaxDecoder(void);
	float processorUsageMaxSD(void);

	void setupDecoder(int channels, int samplerate, int profile);
	_ATOM findMp4Atom(const char *atom, uint32_t posi);
private:

	uint32_t	duration;

	bool setupMp4(void);
	void update(void)  __attribute__ ((optimize(2)));
};


#endif
