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

#ifndef helix_h
#define helix_h

#define SD_BUF_SIZE		3072 //Size of sd-buffer
#define MP3_BUF_SIZE	(MAX_NCHAN * MAX_NGRAN * MAX_NSAMP) //MP3 output buffer
#define AAC_BUF_SIZE	(2048) //AAC output buffer TODO: Check this

#include <Audio.h>
#include <SD.h>

//Include decoders
#include "mp3/mp3dec.h"
#include "aac/aacdec.h"

//Errorcodes
#define ERR_HMP3_NONE   	  	   0
#define ERR_HMP3_NO_QUEUE   	   1
#define ERR_HMP3_FILE_NOT_FOUND    2
#define ERR_HMP3_OUT_OF_MEMORY     3
#define ERR_HMP3_FORMAT			   4
#define ERR_HMP3_DECODING_ERROR    5


class Helix {
public:

protected:
	File			file;
	uint8_t			*sd_buf;//uint8_t sd_buf[SD_BUF_SIZE];
	uint8_t			*sd_p;
	int				sd_left;
	bool			sd_eof;
	uint32_t 		read;
	int16_t			*buf;//int16_t	buf[MP3_BUF_SIZE];
	uint32_t 		framesDecoded;

	AudioPlayQueue	*leftChannel;
	AudioPlayQueue	*rightChannel;

	uint32_t fillReadBuffer(uint8_t *data, uint32_t dataLeft);
	uint32_t skipID3(void);

	void fillAudioBuffers(int numChannels, int len);
};


class HelixMp3 : public Helix {
public:

	short int play(const char *filename,  AudioPlayQueue *lftChannel,  AudioPlayQueue *rghtChannel);

private:
	
	HMP3Decoder		hMP3Decoder;
	MP3FrameInfo	mp3FrameInfo;

};


class HelixAac : public Helix {
public:

	short int play(const char *filename,  AudioPlayQueue *lftChannel,  AudioPlayQueue *rghtChannel);

private:

	HAACDecoder		hAACDecoder;
	AACFrameInfo 	aacFrameInfo;

};

#endif