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

#include "oggparse.h"
#include "oggcrctable.h"

void OggStreamReader::ogg_reader_clear(){
  if(pagehdrbuf){
    free(pagehdrbuf);
    pagehdrbuf = 0;
    pagehdr = 0;
  }
}

bool OggStreamReader::ogg_reader_init(){
  pagepos = 0;
  segmentidx = 0;
  packetpos = 0;
  pagesync = false;
  streamserialvalid = false;
  streamserial = 0;
  codectype = OGG_CODECS;
  maxgranulepos = 0;
  eof = false;
  if(!pagehdrbuf){
      pagehdrbuf = (uint8_t*)malloc(sizeof(ogg_page_header) + 255); // big enough for any header
      pagehdr = (ogg_page_header*)pagehdrbuf;
  }

  // go through the BOS (beginning of stream) pages to identify the audio stream
  while(!streamserialvalid){
    if(!read_next_page(true)){
      return false;
    }
    if(!pagehdr->flag_bos){
      break;
    }
    uint8_t streamhdr[64];
    uint32_t hdrsize = 0;
    if(read_next_packet(streamhdr, sizeof(streamhdr), &hdrsize) != READ_PACKET_COMPLETE){
      return false;
    }
    if(hdrsize < 8){
      continue;
    }
    if(strncmp("vorbis", (char*)streamhdr+1, 6) == 0){
      //Serial.print("found vorbis stream\n");
      streamserialvalid = true;
      streamserial = pagehdr->serialno;
      codectype = OGG_CODEC_VORBIS;
    }else if(strncmp("OpusHead", (char*)streamhdr, 8) == 0){
      //Serial.print("found opus stream\n");
      streamserialvalid = true;
      streamserial = pagehdr->serialno;
      codectype = OGG_CODEC_OPUS;
    }
  }
  
  if(!streamserialvalid){
    Serial.print("no supported stream found\n");
    return false;
  }
  
  // seek to end of file to find the duration. if it fails, just ignore the error. duration is not critical
  if(!seekToOffset(fsize() - 65536)){
    goto success;
  }
  
  maxgranulepos = pagehdr->granuleposition;
  while(read_next_page(!pagesync)){
    maxgranulepos = pagehdr->granuleposition;
  }
  //Serial.print("maxgranulepos ");
  //Serial.println((uint32_t)maxgranulepos);
  
success:
  // move packetpos back to return the first packet again to user
  eof = false;
  seekToOffset(0);
  return true;
}

uint32_t OggStreamReader::verify_page_checksum(){
  uint8_t buf[128];
  uint16_t remaining = current_page_bytes();
  uint32_t crc = 0, correctcrc = 0;
  bool firsttime = true;
  fseek(pagepos);
  while(remaining){
    uint16_t bytes_to_read = remaining > sizeof(buf) ? sizeof(buf) : remaining;
    if(fread(buf, bytes_to_read) < bytes_to_read){
      //Serial.print("verify_page_checksum: truncated file");
      return false;
    }
    if(firsttime){
      // zero out checksum during calculation
      ogg_page_header *hdr = (ogg_page_header*)buf;
      correctcrc = hdr->checksum;
      hdr->checksum = 0;
      firsttime = false;
    }
    crc = _os_update_crc(crc, buf, bytes_to_read);
    remaining -= bytes_to_read;
  }

  if(crc != correctcrc){
    //Serial.print("wrong crc ");
    //Serial.print(crc, HEX);
    //Serial.print(" (correct crc ");
    //Serial.print(correctcrc, HEX);
    //Serial.print(")\n");
    return false;
  }else{
    //Serial.print("crc ok\n");
    return true;
  }
}

bool OggStreamReader::read_next_page(bool verifycrc){
  while(1){
    if(pagesync){
      if(pagehdr->flag_eos){
        //Serial.print("eof\n");
        eof = true;
        return false;
      }
      // calculate where the next page is
      pagepos += current_page_bytes();
    }else{
      //Serial.print("searching for sync from ");
      //Serial.println((uint32_t)pagepos);
      // look for OggS sync
      fseek(pagepos);
      int bytes_in_buf = 0;
      const int bufsize = 256; // here we repurpose the first 256 bytes of pagehdrbuf
      pagehdrbuf[bufsize] = 0;
      const uint8_t *bufend = pagehdrbuf + bufsize;
      bool found = false;
      while(!found){
        int bytes_read = fread(pagehdrbuf + bytes_in_buf, 256 - bytes_in_buf);
        // BUG: if a page begins in the last 256 bytes, this won't find it
        if(bytes_read < 256 - bytes_in_buf){
          eof = true;
          return false;
        }
        uint8_t *oPos = pagehdrbuf;
        while(1){
          oPos = (uint8_t*)memchr(oPos, 'O', bufend - oPos);
          if(oPos == NULL){
            //Serial.print("no more O's in buf\n");
            break;
          }
          if(oPos <= bufend - 4){
            if(!strncmp("OggS", (char*)oPos, 4)){
              //Serial.print("found OggS\n");
              found = true;
              break;
            }else{
              //Serial.print("found O but not OggS\n");
              oPos++;
            }
          }else{
            //Serial.print("found O\n");
            break;
          }
        }
        if(oPos != NULL){
          bytes_in_buf = bufend - oPos;
          memmove(pagehdrbuf, oPos, bytes_in_buf);
          pagepos += oPos - pagehdrbuf;
        }else{
          bytes_in_buf = 0;
          pagepos += bufsize;
        }
        //Serial.print("advancing ");
        //Serial.print(bufsize - bytes_in_buf);
        //Serial.print(" bytes\n");
      }
    }
    //Serial.print("seeking to ");
    //Serial.println((uint32_t)pagepos);
    fseek(pagepos);
    uint bytes_read = fread(pagehdrbuf, sizeof(ogg_page_header));
    if(bytes_read < sizeof(ogg_page_header)){
      // truncated file
      //Serial.print("truncated\n");
      eof = true;
      return false;
    }
    if(strncmp("OggS", pagehdr->magic, 4)){
      // lost sync
      //Serial.print("lost sync\n");
      pagesync = false;
      pagepos++;
      continue;
    }
    bytes_read = fread(pagehdrbuf + sizeof(ogg_page_header), pagehdr->nsegments);
    if(bytes_read < pagehdr->nsegments){
      //Serial.print("truncated\n");
      eof = true;
      return false;
    }
    
    if(verifycrc && !verify_page_checksum()){
      // crc mismatch
      pagesync = false;
      pagepos += 4;
      continue;
    }

    /*
    Serial.print("cont=");
    Serial.print(pagehdr->flag_continued);
    Serial.print(" bos=");
    Serial.print(pagehdr->flag_bos);
    Serial.print(" eos=");
    Serial.print(pagehdr->flag_eos);
    Serial.print(" granule=");
    Serial.print((uint32_t)pagehdr->granuleposition);
    Serial.print(" serialno=");
    Serial.print(pagehdr->serialno);
    Serial.print(" pageno=");
    Serial.print(pagehdr->pageno);
    Serial.print(" checksum=");
    Serial.print(pagehdr->checksum, HEX);
    Serial.print(" nseg=");
    Serial.println(pagehdr->nsegments);
    */
    if(!pagesync){
      //Serial.println("re-acquired sync");
    }
    pagesync = true;

    if(!streamserialvalid || pagehdr->serialno == streamserial){
      break;
    }
  }
  segmentidx = 0;
  packetpos = pagepos + sizeof(ogg_page_header) + pagehdr->nsegments;
  return true;
}

bool OggStreamReader::seekToGranulePos(uint64_t granulePos, uint64_t *landedGranulePos){
  if(granulePos >= maxgranulepos){
    return false;
  }
  
  if(!pagesync){
    if(!read_next_page(true)){
      return false;
    }
  }
  
  if(granulePos == pagehdr->granuleposition){
    *landedGranulePos = pagehdr->granuleposition;
    read_next_page(false);
    return true;
  }
  
  uint64_t highPos, highGran, lowPos, lowGran; // bisection search
  if(granulePos < pagehdr->granuleposition){
    highPos = pagepos + current_page_bytes();
    highGran = pagehdr->granuleposition;
    lowPos = 0;
    lowGran = 0;
  }else{
    highPos = fsize();
    highGran = maxgranulepos;
    lowPos = pagepos + current_page_bytes();
    lowGran = pagehdr->granuleposition;
  }
  
  while(1){
    // possible integer overflow on files >24 hours
    //Serial.print(" lowPos=");
    //Serial.print((uint32_t)lowPos);
    //Serial.print(" highPos=");
    //Serial.print((uint32_t)highPos);
    //Serial.print(" lowGran=");
    //Serial.print((uint32_t)lowGran);
    //Serial.print(" highGran=");
    //Serial.println((uint32_t)highGran);
    uint64_t tryOffset = lowPos + (highPos - lowPos) * (granulePos - lowGran) / (highGran - lowGran);
    //Serial.print("tryOffset=");
    //Serial.println((uint32_t)tryOffset);
    // if tryOffset is close enough, just read ahead and return success
    if(tryOffset >= pagepos + current_page_bytes() && tryOffset <= pagepos + 1024*1024){
      uint64_t lastGran;
      do{
        lastGran = pagehdr->granuleposition;
        if(!read_next_page(false)){
          return false;
        }
      }while((int64_t)pagehdr->granuleposition == -1 || pagehdr->granuleposition <= granulePos);
      *landedGranulePos = lastGran;
      return true;
    }
    
    if(tryOffset > lowPos + 128 * 1024){
      tryOffset -= 128* 1024;
    }else{
      tryOffset = lowPos;
    }
    seekToOffset(tryOffset);
    
    if(granulePos < pagehdr->granuleposition){
      highPos = pagepos + current_page_bytes();
      highGran = pagehdr->granuleposition;
    }else{
      lowPos = pagepos + current_page_bytes();
      lowGran = pagehdr->granuleposition;
    }
  }
}

read_packet_result OggStreamReader::read_next_packet(uint8_t *outbuf, uint32_t outbufsize, uint32_t *packetsize, read_packet_outbuf_full_action outbuf_full_action){
  if(!pagesync || segmentidx >= pagehdr->nsegments){
    // skip checksum if we already have pagesync to save cpu cycles and avoid the backward seek.
    if(!read_next_page(!pagesync)){
      return READ_PACKET_FAIL;
    }
  }
  uint64_t oldpagepos = pagepos;
  uint8_t oldsegmentidx = segmentidx;
  uint64_t oldpacketpos = packetpos;
  *packetsize = 0;
  uint32_t bytes_to_read = 0;
  bool lastpart = false;
  bool incomplete = false;
  while(1){
    while(segmentidx < pagehdr->nsegments){
      uint8_t segsize = pagehdr->segment_sizes[segmentidx];
      if(*packetsize + bytes_to_read + segsize <= outbufsize){
        // segment fits
        segmentidx++;
        bytes_to_read += segsize;
        if(segsize < 255){
          lastpart = true;
          break;
        }
      }else{
        // segment doesn't fit
        if(outbuf_full_action == READ_PACKET_REWIND){
          //Serial.print("buffer too small\n");
          goto restorepos;
        }else{
          incomplete = true;
          break;
        }
      }
    }
    if(bytes_to_read){
      if(!fseek(packetpos)){
        //Serial.print("fseek fail\n");
        eof = true;
        return READ_PACKET_FAIL;
      }
      if(fread(outbuf, bytes_to_read) < bytes_to_read){
        //Serial.print("fread read less than needed\n");
        eof = true;
        return READ_PACKET_FAIL;
      }
      outbuf += bytes_to_read;
      *packetsize += bytes_to_read;
      packetpos += bytes_to_read;
      bytes_to_read = 0;
    }
    if(lastpart || incomplete){
      break;
    }
    // packet spans pages. need to get next page and continue
    if(!read_next_page(false)){
      return READ_PACKET_FAIL;
    }
  }
  //Serial.print("packet is ");
  //Serial.print(*packetsize);
  //Serial.print(" bytes\n");
  if(incomplete){
    return READ_PACKET_INCOMPLETE;
  }else{
    return READ_PACKET_COMPLETE;
  }

restorepos:
  // restore previous state
  if(pagepos != oldpagepos){
    Serial.print("go back to last page\n");
    pagepos = oldpagepos;
    pagesync = false;
    read_next_page(false);
  }
  segmentidx = oldsegmentidx;
  packetpos = oldpacketpos;
  return READ_PACKET_FAIL;
}

uint16_t OggStreamReader::current_page_bytes(){
  if(pagesync){
    uint16_t total_packet_size = 0;
    for(int i = 0; i < pagehdr->nsegments; i++){
      total_packet_size += pagehdr->segment_sizes[i];
    }
    return sizeof(ogg_page_header) + pagehdr->nsegments + total_packet_size;
  }else{
    return 0;
  }
}
