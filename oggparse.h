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

#include <stdint.h>
#include "codecs.h"

// Thanks to OGG being little endian, we can just read the header into a struct
typedef struct __attribute__ ((packed)) ogg_page_header {
  char magic[4];
  uint8_t version; // = 0
  uint8_t flag_continued : 1;
  uint8_t flag_bos : 1;
  uint8_t flag_eos : 1;
  uint64_t granuleposition; // # of samples from beginning of file to last packet in page
  uint32_t serialno; // unique stream id
  uint32_t pageno; // page serial no
  uint32_t checksum;
  uint8_t nsegments;
  uint8_t segment_sizes[0];
} ogg_page_header;

enum ogg_codec {
  OGG_CODEC_VORBIS,
  OGG_CODEC_OPUS,
  OGG_CODECS,
};

enum read_packet_result {
  READ_PACKET_COMPLETE = 0,
  READ_PACKET_INCOMPLETE = 1,
  READ_PACKET_FAIL = 2,
};

enum read_packet_outbuf_full_action {
  READ_PACKET_REWIND = 0, // return READ_PACKET_FAIL and rewind back to beginning of packet
  READ_PACKET_HOLDPOS = 1, // return READ_PACKET_INCOMPLETE and advance read cursor so the next read continues this read
};

// low-memory optimized single-stream ogg demuxer. Uses <512B stack + heap memory
class OggStreamReader : public AudioCodec {
public:
  // for both vorbis and opus, the granuleposition is the same as PCM sample number
  uint32_t lengthMillis(void) {return max(samples_played, maxgranulepos) * 1000 / (samplerate ? samplerate : AUDIOCODECS_SAMPLE_RATE);}
protected:
  // free allocated memory
  void ogg_reader_clear();
  
  // allocate buffers and parse the beginning of file to identify streams
  // return true for success, false for failure
  bool ogg_reader_init();

  uint32_t verify_page_checksum();

  // return true for success, false for failure
  bool read_next_page(bool verifycrc);

  bool seekToOffset(uint64_t filepos){
    pagesync = false;
    pagepos = filepos;
    // do checksum verification in the first page read after seek to be certain we are back in sync
    return read_next_page(true);
  }
  bool seekToGranulePos(uint64_t granulePos, uint64_t *landedGranulePos);
    
  // return true for success, false for failure
  read_packet_result read_next_packet(uint8_t *outbuf, uint32_t outbufsize, uint32_t *packetsize, read_packet_outbuf_full_action outbuf_full_action = READ_PACKET_REWIND);

  uint16_t current_page_bytes();

  uint8_t *pagehdrbuf = 0;;
  ogg_page_header *pagehdr = 0;
  uint64_t pagepos = 0; // offset of the current page in the file.
  // segmentidx and packetpos are advanced with each packet read and reset on each page read
  uint8_t segmentidx = 0; // current index in the segment sizes table.
  uint64_t packetpos = 0; // offset of the next packet in the file
  bool pagesync = false; // pagepos is the beginnging of a page
  bool streamserialvalid = false; // when this is true, we ignore all other streams
  uint32_t streamserial = 0;
  ogg_codec codectype = OGG_CODECS; // codec corresponding to streamserial
  uint64_t maxgranulepos = 0;
  bool eof = false;

  uint32_t samplerate;
};

