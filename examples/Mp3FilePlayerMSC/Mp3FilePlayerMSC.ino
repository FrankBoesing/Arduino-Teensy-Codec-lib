// Simple MP3 player example for USB drives.
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <USBHost_t36.h>
#include <USBHost_ms.h>

#include <play_sd_mp3.h>
// Setup USBHost_t36 and as many HUB ports as needed.
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub3(myusb);
USBHub hub4(myusb);

msFilesystem msFS1(myusb);
msController drive1(myusb);
USBMSCDevice mscDrive;

// GUItool: begin automatically generated code
AudioPlaySdMp3           playMp31;       //xy=154,78
AudioOutputI2S           i2s1;           //xy=334,89
AudioConnection          patchCord1(playMp31, 0, i2s1, 0);
AudioConnection          patchCord2(playMp31, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
// GUItool: end automatically generated code

void setup() {
  Serial.begin(9600);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(5);

  sgtl5000_1.enable();
  sgtl5000_1.volume(0.7);

  // Initialize USBHost_t36
  myusb.begin();
  delay(100);
  
  if (!mscDrive.begin(&drive1)) {
  // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the drive...");
      delay(500);
    }
  }
  myusb.Task();
}

void playFile(const char *filename)
{
  Serial.printf("Playing file: %s\r\n",filename);
  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playMp31.play(&msFS1, filename);
  // Simply wait for the file to finish playing.
    while (playMp31.isPlaying()) {
  // uncomment these lines if your audio shield
  // has the optional volume pot soldered
  //float vol = analogRead(15);
  //vol = vol / 1024;
  //sgtl5000_1.volume(vol);
   delay(250);
 }
}


void loop() {
//  playFile("ForTag.mp3");	
//  playFile("Tom.mp3");
//  playFile("Foreverm.mp3");
  delay(500);
}
