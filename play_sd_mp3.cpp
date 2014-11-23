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

 
#include "play_sd_mp3.h"
#include "spi_interrupt.h"
#include "SD.h"

#define MP3_SD_BUF_SIZE	2048 								//Enough space for a complete stereo frame
#define MP3_BUF_SIZE	(MAX_NCHAN * MAX_NGRAN * MAX_NSAMP) //MP3 output buffer

#define IRQ_AUDIO		IRQ_SOFTWARE	// see AudioStream.cpp
#define IRQ_AUDIO2		56				// use a "reserved" (free) interrupt vector

static	volatile bool 	playing;
	
static	File			file;
static	uint8_t 		sd_buf[MP3_SD_BUF_SIZE];
static	uint8_t			*sd_p;
static	int				sd_left;
	
static	int16_t 		buf[2][MP3_BUF_SIZE * sizeof(int16_t)];
static	unsigned int	decoded_length[2];
static	int				decoding_block;

static	unsigned int    blocks_played;

volatile uint32_t		decode_cycles_max;
volatile uint32_t		decode_cycles_max_sd;
	
static	HMP3Decoder		hMP3Decoder;
static	MP3FrameInfo	mp3FrameInfo;	


static void decode(void) __attribute__ ((hot));
static unsigned int fillReadBuffer(File file, uint8_t *buf, uint8_t *data, uint32_t dataLeft);
static unsigned skipID3(uint8_t *buf) __attribute__ ((cold));

void AudioPlaySdMp3::stop(void){	  
 if (playing) {
		AudioStopUsingSPI();
		__disable_irq();
		audio_block_t *b1 = block_left;		
		audio_block_t *b2 = block_right;		
		playing=false;
		__enable_irq();
		if (b1) release(b1);
		if (b2) release(b2);		
 }
 file.close();
 block_left = NULL;
 block_right = NULL;
 /*
 if (buf[0]) {free(buf[0]);buf[0]=NULL;}
 if (buf[1]) {free(buf[1]);buf[1]=NULL;}
 if (sd_buf) {free(sd_buf);sd_buf=NULL;}
 */
 if (hMP3Decoder) {MP3FreeDecoder(hMP3Decoder);hMP3Decoder=NULL;};
}

bool AudioPlaySdMp3::isPlaying(void){	
 return playing;
}

uint32_t AudioPlaySdMp3::positionMillis(void){
 return 0; //TODO
}

uint32_t AudioPlaySdMp3::lengthMillis(void){
 return 0; //TODO
}

void AudioPlaySdMp3::processorUsageMaxResetDecoder(void){
	__disable_irq();
	decode_cycles_max = 0;
	decode_cycles_max_sd = 0;
	__enable_irq();
};

float AudioPlaySdMp3::processorUsageMaxDecoder(void){
	return  (decode_cycles_max / (0.026*F_CPU)) * 100;
};
	
float AudioPlaySdMp3::processorUsageMaxSD(void){
	return  decode_cycles_max_sd / (0.026*F_CPU)  * 100;
};	 
	
bool AudioPlaySdMp3::play(const char *filename){
	stop();
	
/*	
	sd_buf = (uint8_t *) malloc(MP3_SD_BUF_SIZE);
	if (!sd_buf) {
		return false;
	}

	int16_t *buf = (int16_t *) malloc(MP3_BUF_SIZE * sizeof(int16_t));
	if (!buf) {		
		return false;
	}
*/
	
	file = SD.open(filename);
	if (!file) return false;	
		
	//Read-ahead 10 Bytes to detect ID3
	sd_left = file.read(sd_buf, 10);
		
	//Skip ID3, if existent
	int skip = skipID3(sd_buf);
	if (skip) {
		int b = skip & 0xfffffe00;	
		file.seek(b);
		sd_left = 0;
	}
	//Fill buffer from the beginning with fresh data
	sd_left = fillReadBuffer(file, sd_buf, sd_buf, sd_left);
		
	hMP3Decoder = MP3InitDecoder();

	if (!sd_left || !hMP3Decoder) {
		stop();
		return false;
	}
	
	//upgrade original audiointerrupt if needed (hackish...)
	int audioIntPrio = NVIC_GET_PRIORITY(IRQ_AUDIO);
	if (audioIntPrio == 240) {
		audioIntPrio = 224;
		NVIC_SET_PRIORITY(IRQ_AUDIO, 224);
	}
	
	_VectorsRam[IRQ_AUDIO2 + 16] = decode;
	
	NVIC_SET_PRIORITY(IRQ_AUDIO2, 240);  
	NVIC_ENABLE_IRQ(IRQ_AUDIO2);

	decoded_length[0] = 0;
	decoded_length[1] = 0;

	blocks_played = 0;
	
	decode_cycles_max_sd = 0;
	decode_cycles_max = 0;
		
	decoding_block = 0;
	sd_p = sd_buf;	
	
	decode();
	if((mp3FrameInfo.samprate != 44100) || (mp3FrameInfo.bitsPerSample != 16) || (mp3FrameInfo.nChans > 2)) {
		Serial.println("incompatible MP3 file.");
		playing = false;
		return false;
	}		
	decoding_block = 1;
	
	playing = true;	
	AudioStartUsingSPI();
    return true;
}

//runs in ISR
void AudioPlaySdMp3::update(void)
{
	static int play_pos = 0;
	
	if (!playing) return;
	
	//chain decoder-interrupt
	NVIC_SET_PENDING(IRQ_AUDIO2);
	
	//determine the block we're playing from
	int playing_block = 1-decoding_block;
	if (decoded_length[playing_block]<=0) return;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;
	
	int16_t *b = &buf[playing_block][0];
	
	if (mp3FrameInfo.nChans == 2) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}
		
		int j = 0;
		int k = play_pos;	
		for (int i=0; i < AUDIO_BLOCK_SAMPLES / 4; i++){
				block_left->data[j]=b[k];	k++;
				block_right->data[j]=b[k];	k++;
				j++;
				block_left->data[j]=b[k];	k++;
				block_right->data[j]=b[k];	k++;
				j++;			
				block_left->data[j]=b[k];	k++;
				block_right->data[j]=b[k];	k++;
				j++;
				block_left->data[j]=b[k];	k++;
				block_right->data[j]=b[k];	k++;
				j++;							
		}	
		play_pos = k;
		transmit(block_left, 0);
		transmit(block_right, 1);
		release(block_right);
		block_right = NULL;				
		decoded_length[playing_block] -= AUDIO_BLOCK_SAMPLES * 2;		
		
	} else 
	if (mp3FrameInfo.nChans == 1) {
		// if we're playing mono, no right-side block		
		block_right = NULL;
		
		int j = 0;
		int k = play_pos;
		for (int i=0; i < AUDIO_BLOCK_SAMPLES / 4 ; i++) {			
			block_left->data[j]=b[k];
			j++;k++;
			block_left->data[j]=b[k];
			j++;k++;
			block_left->data[j]=b[k];
			j++;k++;
			block_left->data[j]=b[k];
			j++;k++;		
		}		
		play_pos = k;
		transmit(block_left, 0);
		transmit(block_left, 1);
		decoded_length[playing_block] -= AUDIO_BLOCK_SAMPLES;
		
	}	
			
	release(block_left);
	block_left = NULL;		
	
	blocks_played++;
	
	if (decoded_length[playing_block] == 0) 
	{
		decoding_block = 1-decoding_block;
		play_pos = 0;
	}

}

//decoding-interrupt
void decode(void)
{
	if (decoded_length[decoding_block]) return; //this block is playing, do NOT fill it
	
	uint32_t cycles_sd = ARM_DWT_CYCCNT;
		
	if (sd_left < 1024) { //todo: optimize 1024..
			sd_left = fillReadBuffer( file, sd_buf, sd_p, sd_left);
			sd_p = sd_buf;
	}

	cycles_sd = (ARM_DWT_CYCCNT - cycles_sd);
	if (cycles_sd > decode_cycles_max_sd ) decode_cycles_max_sd = cycles_sd;

	uint32_t cycles = ARM_DWT_CYCCNT;
	
	// find start of next MP3 frame - assume EOF if no sync found
	int offset = MP3FindSyncWord(sd_p, sd_left);

	if (offset < 0) {
			Serial.println("No sync"); //no error at end of file
			playing = false;
			return;
	}	
	
	sd_p += offset;
	sd_left -= offset;
	
	int eof = false;	
	int decode_res = MP3Decode(hMP3Decoder, &sd_p, &sd_left,(short*)&buf[decoding_block][0], 0);	
	
	switch(decode_res)	
		{
			case ERR_MP3_NONE:
				{
					MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
					decoded_length[decoding_block] = mp3FrameInfo.outputSamps;
					break;					
				}
			
			case ERR_MP3_MAINDATA_UNDERFLOW:
				{
					break;
				}

			default :
				{
					eof = true;
					break;
				}
		}
		
	__disable_irq();	
	if (eof) playing = false;
	cycles = (ARM_DWT_CYCCNT - cycles);
	if (cycles > decode_cycles_max ) decode_cycles_max = cycles;
	__enable_irq();		
}

unsigned int fillReadBuffer(File file, uint8_t *sd_buf, uint8_t *data, uint32_t dataLeft)
{
  memmove(sd_buf, data, dataLeft);

  unsigned int spaceLeft = MP3_SD_BUF_SIZE - dataLeft;
  unsigned int read = dataLeft;
  unsigned int n;

  //Read 512 - byte blocks (faster)
  if (spaceLeft>0)
  {
	do {
		n = file.read(sd_buf + dataLeft, min(512, spaceLeft));
		dataLeft += n;
		spaceLeft -= n;
		read +=n;
	} while (spaceLeft >= 512 && n == 512 );
	
	if( n < 512 && spaceLeft >= 512)
	{ //Rest mit 0 füllen
		memset(sd_buf + dataLeft, MP3_SD_BUF_SIZE - dataLeft, 0);
	}
	
  } 
  
  return read;
}

//Skip ID3-Tags at the beginning of the file.
//http://id3.org/id3v2.4.0-structure
unsigned skipID3(uint8_t *sd_buf)
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
	    return ofs;

	}
	else return 0;
}