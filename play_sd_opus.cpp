/*
  Arduino Audiocodecs

  Copyright (c) 2020 jcj83429

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

#include "play_sd_opus.h"

// max bitrate opus frames don't go above 1.5kB
#define OPUS_BITSTREAM_BUF_SIZE 1600

// The time it takes to play this many samples must be greater than the time it takes to decode one frame to avoid buffer underflow. 
// teensy 3.6 @ 120MHz: 768
// teensy 3.6 @ 96MHz: 1024
// teensy 3.6 @ 72MHz: 2048
#define OPUS_BUF_SIZE 2048

// This should be at least the size of two opus frames
#define OPUS_DECBUF_SIZE (1920 * 2)

static AudioPlaySdOpus 	*opusobjptr;

void decodeOpus(void);

void AudioPlaySdOpus::stop(void)
{
	NVIC_DISABLE_IRQ(IRQ_AUDIOCODEC);
	playing = codec_stopped;
	if (buf[1]) {free(buf[1]);buf[1] = NULL;}
	if (buf[0]) {free(buf[0]);buf[0] = NULL;}
	freeBuffer();
	if (decbuf) {free(decbuf);decbuf = NULL;}
	if (opusDecoder){
		opus_decoder_destroy(opusDecoder);
		opusDecoder = NULL;
	}
	ogg_reader_clear();
}

int AudioPlaySdOpus::play(void)
{

	lastError = ERR_CODEC_NONE;
	initVars();

	bitstream_buf = allocBuffer(OPUS_BITSTREAM_BUF_SIZE);
	if (!bitstream_buf) return ERR_CODEC_OUT_OF_MEMORY;
	
	opusobjptr = this;

	buf[0] = (int16_t *) malloc(OPUS_BUF_SIZE * sizeof(int16_t));
	buf[1] = (int16_t *) malloc(OPUS_BUF_SIZE * sizeof(int16_t));
	decbuf = (int16_t *) malloc(OPUS_DECBUF_SIZE * sizeof(int16_t));

	// init opus decoder
	int opusDecoderCreateErr;
	opusDecoder = opus_decoder_create(48000, 2, &opusDecoderCreateErr);
	samplerate = 48000;
	_channels = 2;
	if (!buf[0] || !buf[1] || !decbuf || opusDecoderCreateErr)
	{
		lastError = ERR_CODEC_OUT_OF_MEMORY;
		stop();
		return lastError;
	}
	
	if(!ogg_reader_init()){
		stop();
		Serial.println("file is not opus");
		return ERR_CODEC_FORMAT;
	}
	if(codectype != OGG_CODEC_OPUS){
		Serial.print("codectype ");
		Serial.print(codectype);
		Serial.println("is not opus");
		return ERR_CODEC_FORMAT;
	}

	_VectorsRam[IRQ_AUDIOCODEC + 16] = &decodeOpus;
	initSwi();

	decoded_length[0] = 0;
	decoded_length[1] = 0;
	decbuflen = 0;
	decoding_block = 0;
	decoding_state = 0;

	play_pos = 0;

	if(!parse_opus_header()){
		stop();
		Serial.println("failed to parse OpusHead");
		return ERR_CODEC_FORMAT;
	}
	
	if(!parse_metadata_tags()){
		stop();
		Serial.println("failed to parse OpusTags");
		return ERR_CODEC_FORMAT;
	}
	
	decodeOpus();
	if(lastError){
		return lastError;
	}
	decoding_block = 1;

	playing = codec_playing;

    return lastError;
}

bool AudioPlaySdOpus::seek(uint32_t timesec){
	if(!isPlaying()){
		return false;
	}
	uint64_t granulePos = (uint64_t)timesec * samplerate;
	if(granulePos >= maxgranulepos){
		return false;
	}
	pause(true);
	uint64_t landedGranulePos;
	bool success = seekToGranulePos(granulePos, &landedGranulePos);
	if(!success){
		stop();
		return false;
	}
	
	samples_played = landedGranulePos;
	for(int i = 0; i < 2; i++){
		decoding_block = 1 - decoding_block;
		decoded_length[decoding_block] = 0;
		decodeOpus();
		if(!isPlaying()){
			// hit error
			return false;
		}
	}
	pause(false);
	return true;
}


//runs in ISR
__attribute__ ((optimize("O2")))
void AudioPlaySdOpus::update(void)
{
	audio_block_t	*block_left;
	audio_block_t	*block_right;

	//paused or stopped ?
	if (playing != codec_playing) return;

	//chain decoder-interrupt.
	//to give the user-sketch some cpu-time, only chain
	//if the swi is not active currently.
	//In addition, check before if there waits work for it.
	int db = decoding_block;
	if (!NVIC_IS_ACTIVE(IRQ_AUDIOCODEC))
		if (decoded_length[db] < OPUS_BUF_SIZE)
			NVIC_TRIGGER_INTERRUPT(IRQ_AUDIOCODEC);

	//determine the block we're playing from
	int playing_block = 1 - db;
	if (decoded_length[playing_block] <= 0) return;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;

	uintptr_t pl = play_pos;

	if (_channels == 2) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}

		memcpy_frominterleaved(&block_left->data[0], &block_right->data[0], buf[playing_block] + pl);

		pl += AUDIO_BLOCK_SAMPLES * 2;
		transmit(block_left, 0);
		transmit(block_right, 1);
		release(block_right);
		decoded_length[playing_block] -= AUDIO_BLOCK_SAMPLES * 2;

	} else
	{
		// if we're playing mono, no right-side block
		// let's do a (hopefully good optimized) simple memcpy
		memcpy(block_left->data, buf[playing_block] + pl, AUDIO_BLOCK_SAMPLES * sizeof(short));

		pl += AUDIO_BLOCK_SAMPLES;
		transmit(block_left, 0);
		transmit(block_left, 1);
		decoded_length[playing_block] -= AUDIO_BLOCK_SAMPLES;

	}

	samples_played += AUDIO_BLOCK_SAMPLES;

	release(block_left);

	//Switch to the next block if we have no data to play anymore:
	if (decoded_length[playing_block] == 0 && decoded_length[decoding_block] == OPUS_BUF_SIZE)
	{
		decoding_block = playing_block;
		play_pos = 0;
	} else
	play_pos = pl;

}

void AudioPlaySdOpus::fill_buf_from_decbuf(void){
	int db = decoding_block;
	if(decoded_length[db] < OPUS_BUF_SIZE && decbuflen){
		//Serial.print(decbuflen);
		//Serial.print(" ");
		//Serial.println(decoded_length[db]);
		int16_t samples_to_copy = min(decbuflen, OPUS_BUF_SIZE - decoded_length[db]);
		memcpy(buf[db] + decoded_length[db], decbuf, samples_to_copy * sizeof(*decbuf));
		if(decbuflen > samples_to_copy){
			memmove(decbuf, decbuf + samples_to_copy, (decbuflen - samples_to_copy) * sizeof(*decbuf));
		}
		decbuflen -= samples_to_copy;
		decoded_length[db] += samples_to_copy;
	}
}

bool AudioPlaySdOpus::parse_opus_header(void){
	uint32_t pktBytes;
	read_packet_result res = read_next_packet(bitstream_buf, OPUS_BITSTREAM_BUF_SIZE, &pktBytes);
	if(res != READ_PACKET_COMPLETE){
		return false;
	}
	if(strncmp("OpusHead", (char*)bitstream_buf, 8)){
		return false;
	}
	output_gain = *((int16_t*)(bitstream_buf + 16)) / 256.f;
	return true;
}

bool AudioPlaySdOpus::parse_metadata_tags(void){
	uint32_t readBytes;
	uint32_t bufCursor = 0;
	uint32_t bufBytes = 0;
	read_packet_result readRes;
	bool firstTime = true;
	uint32_t bytesToSkip = 0;
	int numTags = 0;
	do{
		readRes = read_next_packet(bitstream_buf + bufBytes, OPUS_BITSTREAM_BUF_SIZE - bufBytes, &readBytes, READ_PACKET_HOLDPOS);
		if(readRes == READ_PACKET_FAIL){
			return false;
		}
		bufBytes += readBytes;
		if(bytesToSkip){
			uint32_t skip = min(bytesToSkip, bufBytes);
			bytesToSkip -= skip;
			bufCursor = skip;
		}
		
		if(firstTime){
			if(strncmp("OpusTags", (char*)bitstream_buf, 8)){
				return false;
			}
			firstTime = false;
			bufCursor += 8;
			uint32_t vendorStrLen = *((uint32_t*)(bitstream_buf + bufCursor));
			bufCursor += 4 + vendorStrLen;
			numTags = *((uint32_t*)(bitstream_buf + bufCursor));
			bufCursor += 4;
		}
		
		while(bufBytes - bufCursor >= 4 && numTags){
			uint32_t nextTagSize = *((uint32_t*)(bitstream_buf + bufCursor));
			if(4 + nextTagSize > bufBytes - bufCursor){
				if(nextTagSize > 1024){
					//skip this tag
					bytesToSkip = 4 + nextTagSize - (bufBytes - bufCursor);
					bufCursor = bufBytes;
					numTags--;
				}
				break;
			}
			/*
			if(strncmp("R128_TRACK_GAIN=", (char*)bitstream_buf + bufCursor + 4, 16) == 0){
				if(nextTagSize >= 17){
					char buf[10] = {0};
					strncpy(buf, (char*)bitstream_buf + bufCursor + 4 + 16, nextTagSize - 16);
					replaygain_track_gain_db = output_gain + atoi(buf) / 256.f + 5; // The R128 gain is 5dB lower than ReplayGain
				}
			}else if(strncmp("R128_ALBUM_GAIN=", (char*)bitstream_buf + bufCursor + 4, 16) == 0){
				if(nextTagSize >= 17){
					char buf[10] = {0};
					strncpy(buf, (char*)bitstream_buf + bufCursor + 4 + 16, nextTagSize - 16);
					replaygain_album_gain_db = output_gain + atoi(buf) / 256.f + 5;
				}
			}
			*/
			bufCursor += 4 + nextTagSize;
			numTags--;
		}
		if(!numTags){
			// skip blank space at the end
			bufCursor = bufBytes;
		}
		
		if(bufBytes > bufCursor){
			memmove(bitstream_buf, bitstream_buf + bufCursor, bufBytes - bufCursor);
		}
		bufBytes -= bufCursor;
		bufCursor = 0;
	}while(readRes != READ_PACKET_COMPLETE);
	return true;
}

//decoding-interrupt
//__attribute__ ((optimize("O2"))) <- does not work here, bug in g++
void decodeOpus(void)
{
	AudioPlaySdOpus *o = opusobjptr;
	
	o->fill_buf_from_decbuf();

	while(OPUS_DECBUF_SIZE - o->decbuflen >= 1920){
		uint32_t pktBytes;
		read_packet_result pktReadResult = o->read_next_packet(o->bitstream_buf, OPUS_BITSTREAM_BUF_SIZE, &pktBytes);
		if(pktReadResult != READ_PACKET_COMPLETE){
			Serial.println("packet read error");
			o->stop();
			return;
		}
		int decodeResult = opus_decode(o->opusDecoder, o->bitstream_buf, pktBytes, o->decbuf + o->decbuflen, (OPUS_DECBUF_SIZE - o->decbuflen) / 2, false);
		if(decodeResult < 0){
			Serial.print("decoder error");
			Serial.println(decodeResult);
			o->stop();
			return;
		}
		o->decbuflen += decodeResult * 2;
		o->fill_buf_from_decbuf();
	}
}
