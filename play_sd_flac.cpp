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

 */


#include "play_sd_flac.h"

FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data);
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data);
FLAC__StreamDecoderSeekStatus seek_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data);
FLAC__StreamDecoderTellStatus tell_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data);
FLAC__StreamDecoderLengthStatus length_callback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data);
FLAC__bool eof_callback(const FLAC__StreamDecoder *decoder, void *client_data);
void error_callback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC__StreamDecoder	*AudioPlaySdFlac::hFLACDecoder ;

unsigned	AudioCodec::decode_cycles;
unsigned	AudioCodec::decode_cycles_max;
unsigned	AudioCodec::decode_cycles_read;
unsigned	AudioCodec::decode_cycles_max_read;
short		AudioCodec::lastError;

void decodeFlac(void);

void AudioPlaySdFlac::stop(void)
{

	NVIC_DISABLE_IRQ(IRQ_AUDIOCODEC);
	
	playing = codec_stopped;

	delete audiobuffer;
	audiobuffer = NULL;
	if (hFLACDecoder != NULL)
	{
		FLAC__stream_decoder_finish(hFLACDecoder);
		FLAC__stream_decoder_delete(hFLACDecoder);
		hFLACDecoder=NULL;
	};

	fclose();
}


uint32_t AudioPlaySdFlac::lengthMillis(void)
{
	if (hFLACDecoder != NULL)
		return (AUDIO_SAMPLE_RATE_EXACT / 1000) * FLAC__stream_decoder_get_total_samples(hFLACDecoder);
	else
		return 0;
}

unsigned AudioPlaySdFlac::sampleRate(void){
	if (hFLACDecoder != NULL)
		return  FLAC__stream_decoder_get_total_samples(hFLACDecoder);
	else
		return 0;
}
/*
int AudioPlaySdFlac::play(const char *filename){
	stop();
	if (!fopen(filename))
		return ERR_CODEC_FILE_NOT_FOUND;
	return initPlay();
}

int AudioPlaySdFlac::play(const size_t p, const size_t size){
	stop();
	if (!fopen(p,size))
		return ERR_CODEC_FILE_NOT_FOUND;
	return initPlay();
}
*/
int AudioPlaySdFlac::play(void)
{
	initVars();
	lastError = ERR_CODEC_NONE;
	hFLACDecoder =  FLAC__stream_decoder_new();
	if (!hFLACDecoder)
	{
		lastError = ERR_CODEC_OUT_OF_MEMORY;
		goto PlayErr;
	}

	audiobuffer = new AudioBuffer();

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
		this);

//	ret = FLAC__stream_decoder_init_stream(hFLACDecoder,read_callback,NULL,NULL,NULL,NULL,write_callback,NULL,error_callback,this);

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

#ifdef FLAC_USE_SWI
	_VectorsRam[IRQ_AUDIOCODEC + 16] = &decodeFlac;
	initSwi();
#endif

	playing = codec_playing;

    return lastError;

PlayErr:
	stop();
	return lastError;
}

__attribute__ ((optimize("O2")))
FLAC__StreamDecoderReadStatus read_callback(const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	uint32_t cycles = ARM_DWT_CYCCNT;
	FLAC__StreamDecoderReadStatus ret = FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	if(*bytes > 0)
	{
		int num = ((AudioPlaySdFlac*)client_data)->fread(buffer, *bytes);
		if (num > 0)
		{
			ret = FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
			*bytes = num;
		}  else
		if(num == 0)
		{
			ret = FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
			*bytes = 0;
		}
		else *bytes = 0;
	}

	((AudioPlaySdFlac*)client_data)->decode_cycles_read += (ARM_DWT_CYCCNT - cycles);
	return ret;
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
	if (!((AudioPlaySdFlac*)client_data)->fseek(absolute_byte_offset)) {
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
    *absolute_byte_offset = (FLAC__uint64)((AudioPlaySdFlac*)client_data)->fposition();
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

    *stream_length = (FLAC__uint64)((AudioPlaySdFlac*)client_data)->fsize();
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
    return ((AudioPlaySdFlac*)client_data)->f_eof();
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
	//((AudioPlaySdFlac*)client_data)->stop();
	//Serial.print("ERROR ");
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
__attribute__ ((optimize("O3")))
FLAC__StreamDecoderWriteStatus write_callback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const buffer[], void *client_data)
{
	AudioPlaySdFlac *obj = (AudioPlaySdFlac*) client_data;
	int blocksize = frame->header.blocksize  & ~(AUDIO_BLOCK_SAMPLES-1) ;
	int chan = frame->header.channels;
	int bps = frame->header.bits_per_sample;
	size_t numbuffers = (blocksize * chan) / AUDIO_BLOCK_SAMPLES;

	if (obj->audiobuffer->getBufsize() == 0)
	{ //It is our very first frame.
		obj->_channels = chan;
		obj->audiobuffer->allocMem(FLAC_BUFFERS(numbuffers));
		obj->minbuffers	= numbuffers;
	}

	if ( frame->header.sample_rate != AUDIOCODECS_SAMPLE_RATE ||
		chan==0 || chan> 2 ||
		blocksize < AUDIO_BLOCK_SAMPLES ||
		obj->audiobuffer->available() < numbuffers
		)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	//Copy all the data to the fifo. Decoded buffer is 32 bit, fifo is 16 bit
	//int16_t *abufPtr = obj->audiobuffer->alloc(numbuffers);
	//Serial.printf("Free:%d Req:%d\r\n", obj->audiobuffer->available(), numbuffers);


	const FLAC__int32 *sbuf;
	const FLAC__int32 *k;
	if (bps==16) //16 BITS / SAMPLE
	{
		int j = 0;
		do
		{
			int i = 0;
			do
			{
				sbuf = &buffer[i][j];
				k = sbuf + AUDIO_BLOCK_SAMPLES;
				int16_t *abufPtr = obj->audiobuffer->alloc();
				do
				{
					*abufPtr++ = (*sbuf++);
				} while (sbuf < k);

			} while (++i < chan);

			j+=	AUDIO_BLOCK_SAMPLES;
		} while (j < blocksize);
	}
	else if (bps < 16) //2..15 BITS /SAMPLE
	{
		int shift = 16-bps;
		int j = 0;
		do
		{
			int i = 0;
			do
			{
				sbuf = &buffer[i][j];
				k = sbuf + AUDIO_BLOCK_SAMPLES;
				int16_t *abufPtr = obj->audiobuffer->alloc();
				do
				{
					*abufPtr++ = (*sbuf++)<<shift;
				} while (sbuf < k);

			} while (++i < chan);

			j+=	AUDIO_BLOCK_SAMPLES;
		} while (j < blocksize);
	}
	else  //17..32 BITS /SAMPLE
	{
		int shift = bps-16;
		int j = 0;
		do
		{
			int i = 0;
			do
			{
				sbuf = &buffer[i][j];
				k = sbuf + AUDIO_BLOCK_SAMPLES;
				int16_t *abufPtr = obj->audiobuffer->alloc();
				do
				{
					*abufPtr++ = (*sbuf++)>>shift;
				} while (sbuf < k);

			} while (++i < chan);

			j+=	AUDIO_BLOCK_SAMPLES;
		} while (j < blocksize);
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}



//runs in ISR
__attribute__ ((optimize("O3")))
void AudioPlaySdFlac::update(void)
{
	audio_block_t	*audioblockL;
	audio_block_t	*audioblockR;

	//paused or stopped ?

	if (playing != codec_playing) return;

	if (_channels > 0 && audiobuffer->used() >= _channels)
	{
		audioblockL = allocate();
		if (!audioblockL) return;
		if (_channels == 2)
		{
			audioblockR = allocate();
			if (!audioblockR) {
				release(audioblockL);
				return;
			}
			int16_t *abufptrL = audiobuffer->get();
			int16_t *abufptrR = audiobuffer->get();
			for (int j=0; j < AUDIO_BLOCK_SAMPLES; j++)
			{
				audioblockL->data[j] = *abufptrL++;
				audioblockR->data[j] = *abufptrR++;
			}
			transmit(audioblockL, 0);
			transmit(audioblockR, 1);
			release(audioblockL);
			release(audioblockR);
		} else
		{
			int16_t *abufptrL = audiobuffer->get();
			for (int j=0; j < AUDIO_BLOCK_SAMPLES; j++)
			{
				audioblockL->data[j] = *abufptrL++;
			}
			transmit(audioblockL, 0);
			release(audioblockL);
		}

		samples_played += AUDIO_BLOCK_SAMPLES;
	} else
	{ //Stop playing
#ifdef FLAC_USE_SWI
		if (!NVIC_IS_ACTIVE(IRQ_AUDIOCODEC))
#endif
		{
			FLAC__StreamDecoderState state;
			state = FLAC__stream_decoder_get_state(hFLACDecoder);
			if (state == FLAC__STREAM_DECODER_END_OF_STREAM ||
				state == FLAC__STREAM_DECODER_ABORTED ||
				state == FLAC__STREAM_DECODER_MEMORY_ALLOCATION_ERROR)
				{
					stop();
					return;
				}
		}
	}

	if (audiobuffer->getBufsize() > 0 && audiobuffer->available() < minbuffers) return;

#ifndef FLAC_USE_SWI

	decode_cycles_sd = 0;
	FLAC__stream_decoder_process_single(hFLACDecoder);
	if (decode_cycles_sd > decode_cycles_max_sd ) decode_cycles_max_sd = decode_cycles_sd;

#else
	//to give the user-sketch some cpu-time, only chain
	//if the swi is not active currently.
	if (!NVIC_IS_ACTIVE(IRQ_AUDIOCODEC))
			NVIC_TRIGGER_INTERRUPT(IRQ_AUDIOCODEC);
#endif
}

void decodeFlac(void)
{
	//if (!audiobuffer.available()) return;
	AudioPlaySdFlac::decode_cycles_read = 0;

	uint32_t cycles = ARM_DWT_CYCCNT;
	if (AudioPlaySdFlac::hFLACDecoder == NULL) return;
	FLAC__stream_decoder_process_single(AudioPlaySdFlac::hFLACDecoder);
	cycles = (ARM_DWT_CYCCNT - cycles);
	if (cycles - AudioPlaySdFlac::decode_cycles > AudioPlaySdFlac::decode_cycles_max )
			AudioPlaySdFlac::decode_cycles_max = cycles - AudioPlaySdFlac::decode_cycles;
	if (AudioPlaySdFlac::decode_cycles_read > AudioPlaySdFlac::decode_cycles_max_read )
			AudioPlaySdFlac::decode_cycles_max_read = AudioPlaySdFlac::decode_cycles_read;

}
