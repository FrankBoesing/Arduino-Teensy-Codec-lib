// Simple AAC/MP4/M4A file player example
// Plays Audio from Teensy Flash
//
// Tip: Use https://code.google.com/p/bin2h/
// to convert files to *.h or equivalent tool.
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <play_sd_aac.h> 
#include "enterauthorizationcode.h"


// GUItool: begin automatically generated code
AudioPlaySdAac           playAac1;       //xy=154,78
AudioOutputI2S           i2s1;           //xy=334,89
AudioConnection          patchCord1(playAac1, 0, i2s1, 0);
AudioConnection          patchCord2(playAac1, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
// GUItool: end automatically generated code

void setup() {
  Serial.begin(9600);

  delay(1000);
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(2);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(7);
  SPI.setSCK(14);
}
 

void loop() {
  Serial.println("Enter authorization code.\r\n");
  playAac1.play((uint8_t*)&enterauthorizationcode_data, enterauthorizationcode_data_len);
  while (playAac1.isPlaying()) {;}
  delay(2000);
}
