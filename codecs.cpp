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

#include "common/assembly.h"
#include "SD.h"

int lastError = ERR_CODEC_NONE;

//upgrade original audiointerrupt if needed (hackish...)
void init_interrupt()
{	
	if (NVIC_GET_PRIORITY(IRQ_AUDIO) == 240) {
		NVIC_SET_PRIORITY(IRQ_AUDIO, 224);
	}
}
	
// SD-buffer
size_t fillReadBuffer(File file, uint8_t *sd_buf, uint8_t *data, size_t dataLeft, size_t sd_bufsize)
{
	memmove(sd_buf, data, dataLeft);

	size_t spaceLeft = sd_bufsize - dataLeft;
	size_t read = dataLeft;
	size_t n;

	if (spaceLeft>0)
	{	
		
		n = file.read(sd_buf + dataLeft, spaceLeft);
		dataLeft += n;		
		read +=n;
		
		if(n < spaceLeft)
		{ //Rest mit 0 füllen (EOF)
			memset(sd_buf + dataLeft, 0, sd_bufsize - dataLeft);
		}
		
	}

	return read;
}

//read big endian 16-Bit from fileposition(position)
uint16_t fread16(File file, size_t position)
{
	uint16_t tmp16;

	file.seek(position);
	file.read((uint8_t *) &tmp16, sizeof(tmp16));
	return REV16(tmp16);

}

//read big endian 32-Bit from fileposition(position)
uint32_t fread32(File file, size_t position)
{
	uint32_t tmp32;

	file.seek(position);
	file.read((uint8_t *) &tmp32, sizeof(tmp32));
	return REV32(tmp32);

}

//Skip ID3-Tags at the beginning of the file.
//http://id3.org/id3v2.4.0-structure
size_t skipID3(uint8_t *sd_buf)
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

int	AudioCodec::getLastError(void)
{
	return lastError;
}

