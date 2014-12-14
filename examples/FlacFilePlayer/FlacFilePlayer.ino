// Simple Flac file player example
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <play_sd_flac.h>


// GUItool: begin automatically generated code
AudioPlaySdFlac           playFlac1;       //xy=154,78
AudioOutputI2S           i2s1;           //xy=334,89
AudioConnection          patchCord1(playFlac1, 0, i2s1, 0);
AudioConnection          patchCord2(playFlac1, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
// GUItool: end automatically generated code

void setup() {
  Serial.begin(9600);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(6);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(7);
  SPI.setSCK(14);
  if (!(SD.begin(10))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
}

void playFile(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.println(filename);

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playFlac1.play(filename);

  // Simply wait for the file to finish playing.
  while (playFlac1.isPlaying()) {
    // uncomment these lines if your audio shield
    // has the optional volume pot soldered
    //float vol = analogRead(15);
    //vol = vol / 1024;
    // sgtl5000_1.volume(vol);

#if 1	 
	 Serial.print(AudioProcessorUsageMax());
	 Serial.println("% Audio");
	 AudioProcessorUsageMaxReset();
#endif	
	 delay(200); 
  }
  
}


void loop() {
  playFile("T1_128.FLA");

  delay(500);
}
