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

#include <Helix.h>
#include "mp3/mp3dec.h"
#include "mp3/assembly.h"

#include "mp3/bitstream.c"
#include "mp3/buffers.c"
#include "mp3/coder.h"
#include "mp3/dct32.c"
#include "mp3/dequant.c"
#include "mp3/dqchan.c"
#include "mp3/huffman.c"
#include "mp3/hufftabs.c"
#include "mp3/imdct.c"
#include "mp3/mp3dec.c"
#include "mp3/mp3tabs.c"
#include "mp3/scalfact.c"
#include "mp3/stproc.c"
#include "mp3/subband.c"
#include "mp3/trigtabs.c"
//#include "mp3/polyphase.c" // C implementation


HelixMp3::HelixMp3()
{
}

uint32_t HelixMp3::fillReadBuffer(uint8_t* data, uint32_t dataLeft)
{
  memmove(sd_buf, data, dataLeft);

  uint32_t spaceLeft = SD_BUF_SIZE - dataLeft;
  uint32_t read = 0;
  uint16_t n;

  //Read 512 - Byte blocks (faster than other amounts)
  if (spaceLeft>0)
  {
	do {
		n = file.read(sd_buf + dataLeft, min(512, spaceLeft));
		dataLeft += n;
		spaceLeft -= n;
		read +=n;
	} while (spaceLeft>=512 && n == 512 );
  }


  if( n<512 && SD_BUF_SIZE - dataLeft > 0 )
  { //Rest mit 0 füllen
    memset(sd_buf + dataLeft, SD_BUF_SIZE - dataLeft, 0);
  }

  return read;
}

//Skip ID3-Tags at the beginning of the file.
//http://id3.org/id3v2.4.0-structure

void HelixMp3::skipID3(void)
{
	// read = fillReadBuffer(file, sd_buf, sd_p, 0);

	if ( sd_buf[0]=='I' && sd_buf[1]=='D' && sd_buf[2]=='3' &&
		sd_buf[3]<0xff && sd_buf[4]<0xff &&
		sd_buf[6]<0x80 && sd_buf[7]<0x80 && sd_buf[8]<0x80 && sd_buf[9]<0x80) {

		int skip = ((sd_buf[6] & 0x7f) << 21) |
				((sd_buf[7] & 0x7f) << 14) |
				((sd_buf[8] & 0x7f) <<  7) |
				 (sd_buf[9] & 0x7f);

		file.seek(skip);
		sd_left = 0;
	}
	else
	sd_left = read;
}

void HelixMp3::init(void)
{
}

short int HelixMp3::play(const char *filename, AudioPlayQueue *leftChannel, AudioPlayQueue *rightChannel)
{
	int	decode_res= ERR_MP3_NONE;

	if (!leftChannel || !rightChannel)
		return ERR_HMP3_NO_QUEUE;

	file = SD.open(filename);
	if (!file) return ERR_HMP3_FILE_NOT_FOUND;

	sd_buf = (uint8_t *) malloc(SD_BUF_SIZE);
	if (!sd_buf) {
		file.close();
		return ERR_HMP3_OUT_OF_MEMORY;
	}

	buf = (int16_t *) malloc(MP3_BUF_SIZE * sizeof(int16_t));
	if (!buf) {
		file.close();
		free(sd_buf);
		return ERR_HMP3_OUT_OF_MEMORY;
	}

	hMP3Decoder = MP3InitDecoder();
	if (!hMP3Decoder) {
		file.close();
		free(sd_buf);
		free(buf);
		return ERR_HMP3_OUT_OF_MEMORY;
	}

	//DBG.printf("Bytes Free: %d \r\n", FreeRam());

	decode_res = ERR_MP3_NONE;
	framesDecoded = 0;
	sd_eof = false;
	sd_p = sd_buf;

	read = fillReadBuffer(sd_buf, 0);
	skipID3();

	do
	{
		if (sd_left < (2 * MAINBUF_SIZE) && !sd_eof) {
			uint32_t read = fillReadBuffer( sd_p, sd_left);
			sd_left += read;
			sd_p = sd_buf;
		}

		// find start of next MP3 frame - assume EOF if no sync found
		int offset = MP3FindSyncWord(sd_p, sd_left);

		if (offset < 0) {
			//DBG.println("Mp3Decode: No sync"); //no error at end of file
			sd_eof = true;
			break;
		}

		sd_p += offset;
		sd_left -= offset;

		// decode one MP3 frame - if offset < 0 then sd_left was less than a full frame

		decode_res = MP3Decode(hMP3Decoder, &sd_p, &sd_left,(short*)buf, 0);

		switch (decode_res)
		{
			case ERR_MP3_INDATA_UNDERFLOW:
				{
					//This is not really an error at the end of the file:
					//DBG.println("Mp3Decode: Decoding error ERR_MP3_INDATA_UNDERFLOW");
					sd_eof = true;
					break;
				}
			/*
			case ERR_MP3_FREE_BITRATE_SYNC:
				{
					break;
				}
			*/
			case ERR_MP3_MAINDATA_UNDERFLOW:
				{
					//DBG.print("Mp3Decode: Maindata underflow\r\n");
					// do nothing - next call to decode will provide more mainData
					break;
				}
			case ERR_MP3_NONE:
				{
					MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);

					if (framesDecoded == 0)
					{
						//DBG.printf("Mp3Decode: %d Hz %d Bit %d Channels\r\n", mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample, mp3FrameInfo.nChans);
						if((mp3FrameInfo.samprate != 44100) || (mp3FrameInfo.bitsPerSample != 16) || (mp3FrameInfo.nChans > 2)) {
							//DBG.println("Mp3Decode: incompatible MP3 file.");
							sd_eof = true;
							decode_res = ERR_HMP3_FORMAT;
							break;
					}
				}

				framesDecoded++;

				int len = mp3FrameInfo.outputSamps;

				int j = 0;
				int16_t *lb;
				int16_t *rb;

				if (mp3FrameInfo.nChans==1) {

				//1-Channel mono:
				while (len > 0) {

					//todo: Ask Paul if there is a better way
					//a) to play mono
					//b) without two while-wait loops

					//while (!leftChannel->available()) {yield();/*idlePlay();*/}
					lb = leftChannel->getBuffer();
					//while (!rightChannel->available()) {yield();/*idlePlay();*/}
					rb = rightChannel->getBuffer();

					//Todo: Is 2 x memcpy faster ?
					//(esp.with newest newlib which has as very good optimization)

					for (int i=0; i < AUDIO_BLOCK_SAMPLES / 2; i++){

						*lb++ = buf[j];
						*rb++ = buf[j];
						*lb++ = buf[j++];
						*rb++ = buf[j++];

					}

					leftChannel->playBuffer();
					rightChannel->playBuffer();
					len -= AUDIO_BLOCK_SAMPLES;
				}
				}

				else {

				//2-Channel stereo:
				while (len > 0) {

					//while (!leftChannel->available()) {yield();/*idlePlay();*/}
					lb = leftChannel->getBuffer();
					//while (!rightChannel->available()) {yield();/*idlePlay();*/}
					rb = rightChannel->getBuffer();

					//deinterlace audiodata. lrlrlrlr -> llll rrrr

					for (int i=0; i < AUDIO_BLOCK_SAMPLES/2; i++){
						*lb++ = buf[j++];
						*rb++ = buf[j++];
						*lb++ = buf[j++];
						*rb++ = buf[j++];
					}

					leftChannel->playBuffer();
					rightChannel->playBuffer();

					len -= AUDIO_BLOCK_SAMPLES * 2;
				} //Stereo
				}

				break;
			}

			default :
			{
				//DBG.println("Mp3Decode: Decoding error");
				sd_eof = true;
				//decode_res = ERR_HMP3_DECODING_ERROR;
				break;
			}
		}

	} while(!sd_eof || decode_res < 0 );

	file.close();
	free(buf);
	free(sd_buf);
	MP3FreeDecoder(hMP3Decoder);

	return decode_res;
}