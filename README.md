Arduino-Teensy-Codec-lib
======================

Audiolibrary plugin, plays up to 320 kbps *.MP3, *.MP4, *.M4A, or *.AAC in software without external hardware - 
with only 48MHz.

Optimized for ARM Thumb2.

planned:
 - *.flac
 - streaming audio

 HINT: -O2 gives much better performance
 
 TODO:
 - detect APE-header
 - parse ID3 / APE / MP4 for extended information
 - ogg vorbis (?)
 
 not possible: SBR (aac-HE (low bitrate)) - (not enough ram).
