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

#ifndef play_sd_mp3_h_
#define play_sd_mp3_h_

#include "AudioStream.h"
#include "SD.h"

#include "mp3/mp3dec.h"

/* todo
#define ERR_HMP3_NONE   	  	   0
#define ERR_HMP3_FILE_NOT_FOUND    1
#define ERR_HMP3_OUT_OF_MEMORY     2
#define ERR_HMP3_FORMAT			   3	//File is not 44.1 KHz, 16Bit mono or stereo
*/

class AudioPlaySdMp3 : public AudioStream
{
public:
	AudioPlaySdMp3(void) : AudioStream(0, NULL) { stop(); }		
	bool play(const char *filename) ;
	void stop(void);
	bool isPlaying(void);
	uint32_t positionMillis(void);
	uint32_t lengthMillis(void);
	
	void processorUsageMaxResetDecoder(void);	
	float processorUsageMaxDecoder(void);
	float processorUsageMaxSD(void);
	
	virtual void update(void);
	
private:
	audio_block_t	*block_left;
	audio_block_t	*block_right;	
};
	
#endif
