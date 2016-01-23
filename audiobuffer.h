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

#ifndef audiobuffer_h_
#define audiobuffer_h_

#include <stdlib.h>
#include <stdint.h>
#include <Audio.h>

class AudioBuffer
{
public:
	AudioBuffer() { init(); }
	~AudioBuffer(){ if (data) free(data); init(); }
	void allocMem(size_t size){
		__disable_irq();
		data = (int16_t*) malloc(size *	AUDIO_BLOCK_SAMPLES * sizeof(int16_t));
		if (data) bufsize=freespace=size; else bufsize=freespace=0;
		read=write=0;
		__enable_irq();
	}
	size_t available(){ return freespace; }
	size_t used(){ return bufsize-freespace; }
	size_t getBufsize(void) {return bufsize;}
	int16_t *alloc(void)
	{
		int16_t *ret = NULL;
		__disable_irq();
		if (freespace > 0)
		{
			freespace--;
			ret = data + write * AUDIO_BLOCK_SAMPLES;
			write++;
			if (write >= bufsize) write=0;
		}
		__enable_irq();
		return ret;
	}
	int16_t *get(void)
	{
		int16_t *ret = NULL;
		__disable_irq();
		if (freespace < bufsize) {
			freespace++;
			ret = data + read * AUDIO_BLOCK_SAMPLES;
			read++;
			if (read >= bufsize) read=0;
		}
		__enable_irq();
		return ret;
	}
protected:
	void init(void) {bufsize=freespace=read=write=0;data=NULL;}
	int16_t *data;
	size_t bufsize, freespace, read, write;

};

#endif
