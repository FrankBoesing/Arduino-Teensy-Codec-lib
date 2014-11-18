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

/*
 Fills buffer as much as possible,
 reads 512-byte-blocks from sd.
 
 IN:  Pointer in buffer, number of remaining data
 OUT: Number of bytes in buffer
*/   
uint32_t Helix::fillReadBuffer(uint8_t *data, uint32_t dataLeft)
{
  memmove(sd_buf, data, dataLeft);

  uint32_t spaceLeft = SD_BUF_SIZE - dataLeft;
  uint32_t read = dataLeft;
  uint16_t n;

  //Read 512 - byte blocks (faster)
  if (spaceLeft>0)
  {
	do {
		n = file.read(sd_buf + dataLeft, min(512, spaceLeft));
		dataLeft += n;
		spaceLeft -= n;
		read +=n;
	} while (spaceLeft>=512 && n == 512 );
  }

  if( n < 512 )
  { //Rest mit 0 füllen
    memset(sd_buf + dataLeft, SD_BUF_SIZE - dataLeft, 0);
  }
 
  return read;
}

//Skip ID3-Tags at the beginning of the file.
//http://id3.org/id3v2.4.0-structure
void Helix::skipID3(void)
{
	if (sd_buf[0]=='I' && sd_buf[1]=='D' && sd_buf[2]=='3' &&
		sd_buf[3]<0xff && sd_buf[4]<0xff &&
		sd_buf[6]<0x80 && sd_buf[7]<0x80 &&
		sd_buf[8]<0x80 && sd_buf[9]<0x80)
	{
		// bytes 6-9:offset of maindata, with bit.7=0:
		int ofs =	((sd_buf[6] & 0x7f) << 21) |
				((sd_buf[7] & 0x7f) << 14) |
				((sd_buf[8] & 0x7f) <<  7) |
				 (sd_buf[9] & 0x7f);
		int b = ofs & 0xfffffe00;	

		file.seek(b);
		//Fill buffer from the beginning with fresh data
		read = fillReadBuffer(sd_buf, 0);
		sd_p = sd_buf + ofs-b;
		sd_left = read;
	}

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

	int lmax = 0;
	DBG.println(filename);
	
	if (!lftChannel || !rghtChannel)
		return ERR_HMP3_NO_QUEUE;

	leftChannel = lftChannel;
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

	DBG.print("Bytes Free: ");
	DBG.println(FreeRam());
	
	decode_res = ERR_MP3_NONE;
	framesDecoded = 0;
	sd_eof = false;
	sd_p = sd_buf;

	//Read-ahead full buffer
	read = fillReadBuffer(sd_buf, 0);

	//Skip ID3, if existent
	skipID3();

	do
	{
		if (sd_left < MAINBUF_SIZE && !sd_eof) {
			uint32_t read = fillReadBuffer( sd_p, sd_left);
			sd_left = read;
			sd_p = sd_buf;
		}

		// find start of next MP3 frame - assume EOF if no sync found
		int offset = MP3FindSyncWord(sd_p, sd_left);

		if (offset < 0) {
			//DBG.println("No sync"); //no error at end of file
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
					//DBG.println("Decoding error ERR_MP3_INDATA_UNDERFLOW");
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
						//DBG.printf("%d Hz %d Bit %d Channels\r\n", mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample, mp3FrameInfo.nChans);
						if((mp3FrameInfo.samprate != 44100) || (mp3FrameInfo.bitsPerSample != 16) || (mp3FrameInfo.nChans > 2)) {
							//DBG.println("incompatible MP3 file.");
							sd_eof = true;
							decode_res = ERR_HMP3_FORMAT;
							break;
						}
					}
					framesDecoded++;
					fillAudioBuffers(mp3FrameInfo.nChans, mp3FrameInfo.outputSamps);
					break;					
			}

			default :
			{
				//DBG.print("Decoding error");
				//DBG.println(decode_res);
				sd_eof = true;
				break;
			}
		}

	} while(!sd_eof || decode_res < 0 ); //TODO: is checking decode_res ok for streams ?

	file.close();
	free(buf);
	free(sd_buf);
	MP3FreeDecoder(hMP3Decoder);

	return decode_res;
}

/* ===================================================================================================
   class HelixAac
   ===================================================================================================*/

ATOM HelixAac::findMp4Atom(const char *atom, uint32_t posi){

	bool r;
	ATOM ret;	
	struct {uint32_t size;char name[4];} atomInfo;
	
	ret.position = posi;	
	do
	{
		r = file.seek(ret.position);
		file.read((uint8_t *) &atomInfo, sizeof(atomInfo));		
	
		ret.size = HSWAP_UINT32(atomInfo.size);
		if (strncmp(atom, atomInfo.name, 4)==0){			
			//DBG.print(atomInfo.name[0]);DBG.print(atomInfo.name[1]);DBG.print(atomInfo.name[2]);DBG.print(atomInfo.name[3]);
			//DBG.print(": Size ");DBG.print(ret.size, HEX);DBG.print(" Pos ");DBG.println(ret.position, HEX);
			return ret;
		}
		ret.position += ret.size ;
	} while (r);
	
	ret.position = 0;
	ret.size = 0;
	return ret;
	
}


void HelixAac::findMp4Mdat(void)
{
	//go through the boxes to find the interesting atoms:
	uint32_t moov = findMp4Atom("moov",0).position;
	uint32_t trak = findMp4Atom("trak", moov + 8).position;
	uint32_t mdia = findMp4Atom("mdia", trak + 8).position;
	uint32_t minf = findMp4Atom("minf", mdia + 8).position;		
	uint32_t stbl = findMp4Atom("stbl", minf + 8).position;
	
	//stsd sample description box - great info to parametrize the decoder
	findMp4Atom("stsd", stbl + 8);
	file.read(sd_p, 0x30);	
	
	channels	= sd_p[0x21];
	bits		= sd_p[0x23];	
	samplerate	= ((uint16_t) sd_p[0x28]<<8 | (uint16_t) sd_p[0x29]);
	
	//stco - chunk offset atom
	findMp4Atom("stco", stbl + 8);	
	file.read(sd_p, 0x10);
	//read entry #1 from chunk table, ignore entry #0 for bginning of audiodata:
	firstFrame = (uint32_t) sd_p[0x0c] << 24 | (uint32_t) sd_p[0x0d] << 16 |
						(uint16_t) sd_p[0x0e] << 8 |  sd_p[0x0f];
	
	//find mdat-chunk to calculate the size of the audiodata
	ATOM mdat = findMp4Atom("mdat",0);
	sizeOfData = mdat.size - (firstFrame - (mdat.position + 8));
	/*
	DBG.print("stsd: chan=");DBG.print(channels);
	DBG.print(" bits=");DBG.print(bits);
	DBG.print(" samplerate=");
	DBG.print(samplerate);
	DBG.print(" firstFrame=");
	DBG.print(firstFrame, HEX);	
	DBG.print(" size=");
	DBG.println(sizeOfData, HEX);
	*/
	return; 
}
   
short int HelixAac::play(const char *filename, AudioPlayQueue *lftChannel, AudioPlayQueue *rghtChannel)
{

	int	decode_res = ERR_AAC_NONE;

	//int lmax = 0;
	DBG.println(filename);
	
	if (!lftChannel || !rghtChannel)
		return ERR_HMP3_NO_QUEUE;

	leftChannel = lftChannel;
	rightChannel = rghtChannel;

	isRAW = true;
	char *c = strrchr(filename, '.');
	if ( (stricmp(c, ".mp4")==0) ) isRAW = false;
	else 
	if ( (stricmp(c, ".m4a")==0) ) isRAW = false;
	
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

	uint32_t consumed = 0;
	framesDecoded = 0;
	sd_eof = false;
	sd_p = sd_buf;
	
	if (!isRAW) { //setup for mp4/m4u
		
		findMp4Mdat();
		file.seek(firstFrame);
		sd_left = fillReadBuffer(sd_buf, 0);//Read-ahead full buffer
		
		memset(&aacFrameInfo, 0, sizeof(AACFrameInfo));
		aacFrameInfo.nChans = channels;
		aacFrameInfo.bitsPerSample = bits;
		aacFrameInfo.sampRateCore = samplerate;
		aacFrameInfo.profile = AAC_PROFILE_LC;
		AACSetRawBlockParams(hAACDecoder, 0, &aacFrameInfo);	

	} else { //setup aac:
		sd_left = fillReadBuffer(sd_buf, 0);//Read-ahead full buffer
		skipID3();	
		sizeOfData = 0; //can't know.. 
	}

	do
	{
	
		if (sd_left < SD_BUF_SIZE / 2 && !sd_eof) {
			read = fillReadBuffer( sd_p, sd_left);
			sd_p = sd_buf;
			sd_left = read;			
			if (!read) sd_eof=true;
		}

		if (isRAW) { //i.e. *.aac-files or streams
			// find start of next MP3 frame - assume EOF if no sync found
			int offset =AACFindSyncWord(sd_p, sd_left);

			if (offset < 0) {
				DBG.println("No sync"); //no error at end of file
				sd_eof = true;
				break;		
			}
			
			sd_p += offset;
			sd_left -= offset;

		}

		//int l1 = micros();
		uint32_t leftBefore = sd_left;
		decode_res =AACDecode(hAACDecoder, &sd_p, &sd_left,(short*)buf);
		consumed += (leftBefore - sd_left);
		//l1 = micros() - l1;
		//lmax += l1;
		//if (framesDecoded % 100==0) { DBG.println(lmax / 100); lmax=0; }

		if (!decode_res) 
		{
			
			AACGetLastFrameInfo(hAACDecoder, &aacFrameInfo);
			if (framesDecoded == 0) {
				DBG.print("bitrate:");DBG.println(aacFrameInfo.bitRate);//TODO Why is this =0 ? Bug ?
				DBG.print("sampRateCore:");DBG.println(aacFrameInfo.sampRateCore);
				DBG.print("sampRateOut:");DBG.println(aacFrameInfo.sampRateOut);
				DBG.print("bitsPerSample:");DBG.println(aacFrameInfo.bitsPerSample);
				DBG.print("nChans:");DBG.println(aacFrameInfo.nChans);
				DBG.print("Samples:");DBG.println(aacFrameInfo.outputSamps);
					
				if((aacFrameInfo.sampRateOut != 44100) || (aacFrameInfo.bitsPerSample != 16) || (aacFrameInfo.nChans > 2)) 
				{
					DBG.println("incompatible AAC file.");
					sd_eof = true;
					decode_res = ERR_HMP3_FORMAT;
					break;
				}			
			}

			framesDecoded++;				
			fillAudioBuffers(aacFrameInfo.nChans, aacFrameInfo.outputSamps);

		} else {
			//This should never be the case. hopefully. it helps skipping bad frames, but is noisy.
			sd_left --;
			sd_p ++;		
		}

		if (sizeOfData > 0 && consumed >= sizeOfData) sd_eof = true;
	
	} while(!sd_eof /*&& !decode_res*/  ); //to check decode_res is evil for streams. (really ? TODO?)

	file.close();
	free(buf);
	free(sd_buf);
	AACFreeDecoder(hAACDecoder);

	return decode_res;
}
