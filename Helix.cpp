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

 

#include <Helix.h>

#define DBG Serial

/* ===================================================================================================
   Base class Helix
   ===================================================================================================*/

uint32_t Helix::fillReadBuffer(uint8_t *data, uint32_t dataLeft)
{
  memmove(sd_buf, data, dataLeft);

  uint32_t spaceLeft = SD_BUF_SIZE - dataLeft;
  uint32_t read = 0;
  uint16_t n;

  //Read 512 - byte blocks (faster than other amounts)
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
uint32_t Helix::skipID3(void)
{
	uint32_t skip = 0;
	
	if (sd_buf[0]=='I' && sd_buf[1]=='D' && sd_buf[2]=='3' &&
		sd_buf[3]<0xff && sd_buf[4]<0xff &&
		sd_buf[6]<0x80 && sd_buf[7]<0x80 &&
		sd_buf[8]<0x80 && sd_buf[9]<0x80)
	{
		// bytes 6-9:offset of maindata, with bit.7=0:
		skip =	((sd_buf[6] & 0x7f) << 21) |
				((sd_buf[7] & 0x7f) << 14) |
				((sd_buf[8] & 0x7f) <<  7) |
				 (sd_buf[9] & 0x7f);
	}

	return skip;
}

void Helix::fillAudioBuffers(int numChannels, int len)
{
	int j = 0;
	int16_t *lb;
	int16_t *rb;

	if (numChannels==1) { //1-Channel mono:
		while (len > 0)
		{

			//todo: Ask Paul if there is a better way
			//a) to play mono
			//b) without two while-wait loops

			//while (!leftChannel->available()) {yield();/*idlePlay();*/}
			lb = leftChannel->getBuffer();
			//while (!rightChannel->available()) {yield();/*idlePlay();*/}
			rb = rightChannel->getBuffer();

			//copy audiodata. lllllll -> llll rrrr
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
	} //mono

	else if (numChannels==2) //2-Channel stereo:
	{
		while (len > 0)
		{

			//while (!leftChannel->available()) {yield();/*idlePlay();*/}
			lb = leftChannel->getBuffer();
			//while (!rightChannel->available()) {yield();/*idlePlay();*/}
			rb = rightChannel->getBuffer();

			//deinterlace audiodata. lrlrlrlr -> llll rrrr

			for (int i=0; i < AUDIO_BLOCK_SAMPLES / 2; i++){
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
}

/* ===================================================================================================
   class HelixMp3
   ===================================================================================================*/
   
short int HelixMp3::play(const char *filename, AudioPlayQueue *lftChannel, AudioPlayQueue *rghtChannel)
{
	int	decode_res;

	//int lmax = 0;

	if (!leftChannel || !rightChannel)
		return ERR_HMP3_NO_QUEUE;

	leftChannel = leftChannel;
	rightChannel = rghtChannel;

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

	//Read-ahead full buffer
	read = fillReadBuffer(sd_buf, 0);

	//Skip ID3, if existent
	uint32_t skip = skipID3();
	if (skip) {
		file.seek(skip);
		sd_left = 0;
	} else {
		sd_left = read;
	}

	//decoding loop
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

		//int l1 = micros();
		decode_res = MP3Decode(hMP3Decoder, &sd_p, &sd_left,(short*)buf, 0);
		//l1 = micros() - l1;
		//lmax += l1;
		//if (framesDecoded % 100==0) { Serial.println(lmax / 100); lmax=0; }

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
					framesDecoded++;
					fillAudioBuffers(mp3FrameInfo.nChans, mp3FrameInfo.outputSamps);
					break;

				}
			}

			default :
			{
				//DBG.println("Decoding error");
				sd_eof = true;
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

/* ===================================================================================================
   class HelixAac
   ===================================================================================================*/

short int HelixAac::play(const char *filename, AudioPlayQueue *lftChannel, AudioPlayQueue *rghtChannel)
{

	int	decode_res;

	//int lmax = 0;
	//DBG.print("Decode");
	if (!leftChannel || !rightChannel)
		return ERR_HMP3_NO_QUEUE;

	leftChannel = leftChannel;
	rightChannel = rghtChannel;

	file = SD.open(filename);
	if (!file) return ERR_HMP3_FILE_NOT_FOUND;

	sd_buf = (uint8_t *) malloc(SD_BUF_SIZE);
	if (!sd_buf) {
		file.close();
		return ERR_HMP3_OUT_OF_MEMORY;
	}

	buf = (int16_t *) malloc(AAC_BUF_SIZE * sizeof(int16_t));
	if (!buf) {
		file.close();
		free(sd_buf);
		return ERR_HMP3_OUT_OF_MEMORY;
	}

	hAACDecoder = AACInitDecoder();
	if (!hAACDecoder) {
		file.close();
		free(sd_buf);
		free(buf);
		return ERR_HMP3_OUT_OF_MEMORY;
	}

	//DBG.print("Bytes Free: ");
	//DBG.println(FreeRam());

	decode_res = ERR_AAC_NONE;
	framesDecoded = 0;
	sd_eof = false;
	sd_p = sd_buf;

	//Read-ahead full buffer
	read = fillReadBuffer(sd_buf, 0);

	//Skip ID3, if existent
	uint32_t skip = skipID3();
	if (skip) {
		file.seek(skip);
		sd_left = 0;
	} else {
		sd_left = read;
	}

	//decoding loop
	do
	{
		if (sd_left < (2 * MAINBUF_SIZE) && !sd_eof) {
			uint32_t read = fillReadBuffer( sd_p, sd_left);
			sd_left += read;
			sd_p = sd_buf;
		}

		// find start of next MP3 frame - assume EOF if no sync found
		int offset =AACFindSyncWord(sd_p, sd_left);

		if (offset < 0) {
			DBG.println("Mp3Decode: No sync"); //no error at end of file
			sd_eof = true;
			break;
		}

		sd_p += offset;
		sd_left -= offset;

		//int l1 = micros();
		decode_res =AACDecode(hAACDecoder, &sd_p, &sd_left,(short*)buf);

		//DBG.println(decode_res);

		//l1 = micros() - l1;
		//lmax += l1;
		//if (framesDecoded % 100==0) { DBG.println(lmax / 100); lmax=0; }

		switch (decode_res)
		{
			case ERR_AAC_INDATA_UNDERFLOW:
				{
					//This is not really an error at the end of the file:
					DBG.println("Decoding error ERR_AAC_INDATA_UNDERFLOW");
					sd_eof = true;
					break;
				}

			case ERR_AAC_NONE:
				{
					AACGetLastFrameInfo(hAACDecoder, &aacFrameInfo);

					if (framesDecoded == 0)
					{/*
							DBG.println("sampRateCore:");DBG.println(aacFrameInfo.sampRateCore);
							DBG.println("sampRateOut:");DBG.println(aacFrameInfo.sampRateOut);
							DBG.println("bitsPerSample:");DBG.println(aacFrameInfo.bitsPerSample);
							DBG.println("nChans:");DBG.println(aacFrameInfo.nChans);
					*/
						//DBG.printf("Mp3Decode: %d Hz %d Bit %d Channels\r\n", mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample, mp3FrameInfo.nChans);
						if((aacFrameInfo.sampRateOut != 44100) || (aacFrameInfo.bitsPerSample != 16) || (aacFrameInfo.nChans > 2)) {
							//DBG.println("AacDecode: incompatible AAC file.");
							sd_eof = true;
							decode_res = ERR_HMP3_FORMAT;
							break;
					}

					framesDecoded++;
					fillAudioBuffers(aacFrameInfo.nChans, aacFrameInfo.outputSamps);
					break;
				}
			}

			default :
			{
				DBG.print("Decoding error ");
				DBG.println(decode_res);
				sd_eof = true;
				break;
			}
		}

	} while(!sd_eof || decode_res < 0 );

	file.close();
	free(buf);
	free(sd_buf);
	AACFreeDecoder(hAACDecoder);

	return decode_res;
}
