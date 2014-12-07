/*
	Helix library Arduino Audio Library MP3/AAC objects

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


#include "codecs.h"
#include "SD.h"

//upgrade original audiointerrupt if needed (hackish...)
void init_interrupt( void (*decoder)(void) )
{

	int audioIntPrio = NVIC_GET_PRIORITY(IRQ_AUDIO);
	if (audioIntPrio == 240) {
		audioIntPrio = 224;
		NVIC_SET_PRIORITY(IRQ_AUDIO, audioIntPrio);
	}

	_VectorsRam[IRQ_AUDIO2 + 16] = decoder;

	NVIC_SET_PRIORITY(IRQ_AUDIO2, audioIntPrio + 0x10);
	NVIC_ENABLE_IRQ(IRQ_AUDIO2);
	
}
	
// SD-buffer
unsigned int fillReadBuffer(File file, uint8_t *sd_buf, uint8_t *data, uint32_t dataLeft, uint32_t sd_bufsize)
{
	memmove(sd_buf, data, dataLeft);

	unsigned int spaceLeft = sd_bufsize - dataLeft;
	unsigned int read = dataLeft;
	unsigned int n;

	//Read 512 - byte blocks (faster)
	if (spaceLeft>0)
	{	
		unsigned int num;
		do {
			num = min(512, spaceLeft);
			n = file.read(sd_buf + dataLeft, num);
			dataLeft += n;
			spaceLeft -= n;
			read +=n;
	
		} while (spaceLeft >= 512 && n == 512 );

		if( n<num)
		{ //Rest mit 0 füllen
			memset(sd_buf + dataLeft, sd_bufsize - dataLeft, 0);
		}

	}

	return read;
}

//Skip ID3-Tags at the beginning of the file.
//http://id3.org/id3v2.4.0-structure
unsigned int skipID3(uint8_t *sd_buf)
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
