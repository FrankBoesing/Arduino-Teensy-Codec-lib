/*
	Arduino Audiocodecs

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


#include "play_sd_flac.h"
#include "flac/share/endswap.h"

#ifdef FLAC_USE_SWI
#define FLAC_BUF_SIZE	(128) 	//FLAC output buffer, TODO: choose size automatically
#endif


static File				file;

#ifdef FLAC_USE_SWI
static short			*buf[2]; //output buffers
static size_t			decoded_length[2];
static size_t			decoding_block;
#endif 

unsigned char 			channels;

static uint32_t			decode_cycles_max;
static uint32_t			decode_cycles_max_sd;

static unsigned int		playing;

static FLAC__StreamDecoder	*hFLACDecoder;
void 					*client_Data;


static FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data);
static FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
static FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
static FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
static void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

static void flacstop(void);
//static void decode(void);

void AudioPlaySdFlac::stop(void)
{
	flacstop();
}

bool AudioPlaySdFlac::pause(bool paused)
{
	if (playing) {
		if (paused) playing = 2; 
		else playing = 1;
	}
	return (playing == 2); 
}


bool AudioPlaySdFlac::isPlaying(void)
{
	return (playing > 0);
}

uint32_t AudioPlaySdFlac::positionMillis(void)
{
	return (AUDIO_SAMPLE_RATE_EXACT / 1000) * samples_played;
}

uint32_t AudioPlaySdFlac::lengthMillis(void)
{/*
	if (playing) {
		return (file.size() - size_id3) / (flacFrameInfo.bitrate / 8 ) * 1000;
	}

	else */	return 0;
}

uint32_t AudioPlaySdFlac::bitrate(void)
{/*
	if (playing) {
		return flacFrameInfo.bitrate;
	}
	else */ return 0;
}

void AudioPlaySdFlac::processorUsageMaxResetDecoder(void){
	__disable_irq();
	decode_cycles_max = 0;
	decode_cycles_max_sd = 0;
	__enable_irq();
};

float AudioPlaySdFlac::processorUsageMaxDecoder(void){
//TODO
	return 0;
};

float AudioPlaySdFlac::processorUsageMaxSD(void){
//TODO
	return 0;
};


int AudioPlaySdFlac::play(const char *filename){
	stop();

	lastError = ERR_CODEC_NONE;
	
	hFLACDecoder =  FLAC__stream_decoder_new();
	
	if (!hFLACDecoder)
	{
		lastError = ERR_CODEC_OUT_OF_MEMORY;
		goto PlayErr;		
	}
	
#ifndef FLAC_USE_SWI
	client_Data = this;
#else
	buf[0] = (short *) malloc(FLAC_BUF_SIZE * sizeof(int16_t));
	buf[1] = (short *) malloc(FLAC_BUF_SIZE * sizeof(int16_t));

	if (!buf[0] || !buf[1] || !hFLACDecoder)
	{
		lastError = ERR_CODEC_OUT_OF_MEMORY;
		goto PlayErr;
	}
	
	init_interrupt();	
	_VectorsRam[IRQ_AUDIOCODEC + 16] = &decode;
	NVIC_SET_PRIORITY(IRQ_AUDIOCODEC, IRQ_AUDIOCODEC_PRIO);
	NVIC_ENABLE_IRQ(IRQ_AUDIOCODEC);	
	
	decoded_length[0] = 0;
	decoded_length[1] = 0;
	decoding_block = 0;	
	play_pos = 0;
	
#endif	
		
	file = SD.open(filename);
	
	if (!file) 
	{
		lastError = ERR_CODEC_FILE_NOT_FOUND;
		goto PlayErr;		
	}
		
	samples_played = 0;

	decode_cycles_max_sd = 0;
	decode_cycles_max = 0;

	FLAC__StreamDecoderInitStatus ret;	

	ret = FLAC__stream_decoder_init_stream(hFLACDecoder, 
		read_callback, 
		seek_callback, 
		tell_callback, 
		length_callback, 
		eof_callback, 
		write_callback, 
		NULL, 
		error_callback, 
		client_Data);
	
/*
	ret = FLAC__stream_decoder_init_stream(hFLACDecoder, 
		read_callback, 
		NULL, 
		NULL, 
		NULL, 
		NULL, 
		write_callback, 
		NULL, 
		error_callback, 
		client_Data);
*/
		
	if (ret != FLAC__STREAM_DECODER_INIT_STATUS_OK)
	{		
		if (ret == FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR) lastError = ERR_CODEC_OUT_OF_MEMORY;
		else lastError = ERR_CODEC_FORMAT;
		//Serial.printf("Init: %s",FLAC__StreamDecoderInitStatusString[ret]);
		goto PlayErr;		
	}
	
	if (!FLAC__stream_decoder_process_until_end_of_metadata(hFLACDecoder))
	{
		lastError = ERR_CODEC_FORMAT;
		goto PlayErr;		
	}
	
	//Serial.println("Start OK");
	
#ifdef FLAC_USE_SWI
	decoding_block = 1;	
#endif	

	playing = 1;
	AudioStartUsingSPI();

    return lastError;

PlayErr:
	stop();
	return lastError;

}

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	if(*bytes > 0) {	
		*bytes = file.read(buffer, *bytes);
		if(*bytes == 0)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		else return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
	else return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}

/**
 * Seek callback. Called when decoder needs to seek the stream.
 *
 * @param decoder               Decoder instance
 * @param absolute_byte_offset  Offset from beginning of stream to seek to
 * @param client_data           Client data set at initialisation
 *
 * @return Seek status
 */
FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder* decoder, FLAC__uint64 absolute_byte_offset, void* client_data)
{	
	if (!file.seek(absolute_byte_offset)) {
        // seek failed
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    }
    else {
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
    }
}
/**
 * Tell callback. Called when decoder wants to know current position of stream.
 *
 * @param decoder               Decoder instance
 * @param absolute_byte_offset  Offset from beginning of stream to seek to
 * @param client_data           Client data set at initialisation
 *
 * @return Tell status
 */
FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder* decoder, FLAC__uint64* absolute_byte_offset, void* client_data)
{
    // update offset
    *absolute_byte_offset = (FLAC__uint64)file.position();
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

/**
 * Length callback. Called when decoder wants total length of stream.
 *
 * @param decoder        Decoder instance
 * @param stream_length  Total length of stream in bytes
 * @param client_data    Client data set at initialisation
 *
 * @return Length status
 */
FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder* decoder, FLAC__uint64* stream_length, void* client_data)
{

    *stream_length = (FLAC__uint64)file.size();
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;

}

/**
 * EOF callback. Called when decoder wants to know if end of stream is reached.
 *
 * @param decoder       Decoder instance
 * @param client_data   Client data set at initialisation
 *
 * @return True if end of stream
 */
FLAC__bool eof_callback(const FLAC__StreamDecoder* decoder, void* client_data)
{
    return (file.available()<=0)? true : false;
}
/**
 * Error callback. Called when error occured during decoding.
 *
 * @param decoder       Decoder instance
 * @param status        Error
 * @param client_data   Client data set at initialisation
 */
 
void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	//Serial.println(FLAC__StreamDecoderErrorStatusString[status]);
	((AudioPlaySdFlac*)client_data)->stop();
}

/**
 * Write callback. Called when decoder has decoded a single audio frame.
 *
 * @param decoder       Decoder instance
 * @param frame         Decoded frame
 * @param buffer        Array of pointers to decoded channels of data
 * @param client_data   Client data set at initialisation
 *
 * @return Read status
 */
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	//Serial.printf("WRITE BLOCKSIZE:%d CHANNELS:%d", frame->header.blocksize, frame->header.channels);

#ifndef FLAC_USE_SWI			
	AudioPlaySdFlac *audioobj;
	audio_block_t	*audioblock;
	unsigned i;
	
	audioobj = ((AudioPlaySdFlac*)client_data);
	
	if (frame->header.blocksize != AUDIO_BLOCK_SAMPLES)  
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;	
		
	for (i=0; i< frame->header.channels; i++)
	{
		audioblock = audioobj->allocate();
		if (audioblock == NULL) 
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;	
		for (int j=0; j<AUDIO_BLOCK_SAMPLES; j++)
			audioblock->data[j] = buffer[i][j];		
		audioobj->transmit(audioblock, i);
		audioobj->release(audioblock);	
	}
	
	audioobj->samples_played += AUDIO_BLOCK_SAMPLES;
#else
 todo
#endif

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



//runs in ISR
#ifndef FLAC_USE_SWI		
void AudioPlaySdFlac::update(void) 
{
	if (0==playing) return;
	if (2==playing) return;
	
	if (!FLAC__stream_decoder_process_single(hFLACDecoder)) 	
	{
		lastError = ERR_CODEC_FORMAT;		
		stop();
	}
}

#else
void AudioPlaySdFlac::update(void) 
{
	audio_block_t	*block_left;
	audio_block_t	*block_right;

	//paused or stopped ?
	if (0==playing or 2==playing) return;

	//chain decoder-interrupt.
	//to give the user-sketch some cpu-time, only chain
	//if the swi is not active currently.
	//In addition, check before if there waits work for it.
	int db = decoding_block;
	if (!NVIC_IS_ACTIVE(IRQ_AUDIOCODEC)) 
		if (decoded_length[db]==0)
			NVIC_TRIGGER_INTERRUPT(IRQ_AUDIOCODEC);

	//determine the block we're playing from
	int playing_block = 1 - decoding_block;
	if (decoded_length[playing_block] <= 0) return;
	
	uintptr_t pl = play_pos;
	uintptr_t bptr = buf[playing_block] + pl;

	// allocate the audio blocks to transmit
	block_left = allocate();
	if (block_left == NULL) return;

	if (channels == 2) {
		// if we're playing stereo, allocate another
		// block for the right channel output
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
			return;
		}
		
		memcpy_frominterleaved(block_left->data, block_right->data, bptr);

		pl += AUDIO_BLOCK_SAMPLES * 2;

		transmit(block_left, 0);
		transmit(block_right, 1);
		release(block_right);
		decoded_length[playing_block] -= AUDIO_BLOCK_SAMPLES * 2;

	} else
	{
		// if we're playing mono, no right-side block
		// let's do a (hopefully good optimized) simple memcpy
		memcpy(block_left->data, bptr, AUDIO_BLOCK_SAMPLES * sizeof(short));

		pl += AUDIO_BLOCK_SAMPLES;

		transmit(block_left, 0);
		transmit(block_left, 1);
		decoded_length[playing_block] -= AUDIO_BLOCK_SAMPLES;

	}

	samples_played += AUDIO_BLOCK_SAMPLES;

	release(block_left);
	

	//Switch to the next block if we have no data to play anymore:
	if (decoded_length[playing_block] == 0)
	{
		decoding_block = playing_block;
		play_pos = 0;
	} else 
	play_pos = pl;

}


//decoding-interrupt
/* todo (?) would be better for big block-sizes
void decode(void)
{

	if (decoded_length[decoding_block]) return; //this block is playing, do NOT fill it
		
	uint32_t cycles = ARM_DWT_CYCCNT;
	int eof = false;
	
	eof = !FLAC__stream_decoder_process_until_end_of_metadata(hFLACDecoder);
	
	cycles = (ARM_DWT_CYCCNT - cycles);
	if (cycles > decode_cycles_max ) decode_cycles_max = cycles;
	
	
flacend:
		
	if (eof) {
		//flacstop();
	} 

}
*/
#endif 

void flacstop(void)
{
	AudioStopUsingSPI();
	__disable_irq();	
	playing = 0;		
#ifdef FLAC_USE_SWI
	if (buf[1]) {free(buf[1]);buf[1] = NULL;}
	if (buf[0]) {free(buf[0]);buf[0] = NULL;}
#endif
	if (hFLACDecoder)
	{
		FLAC__stream_decoder_finish(hFLACDecoder);
		FLAC__stream_decoder_delete(hFLACDecoder);
		hFLACDecoder=NULL;
	};
	__enable_irq();
	file.close();
}