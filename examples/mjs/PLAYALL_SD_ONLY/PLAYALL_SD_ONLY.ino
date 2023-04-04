// Requires the prop-shield
// This example code is in the public domain.

#include <string.h>

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <play_sd_mp3.h>
#include <play_sd_aac.h>
#include <play_sd_flac.h>

// GUItool: begin automatically generated code
AudioPlaySdMp3           playMp31;       //xy=154,78
AudioPlaySdWav           playWav; //xy=154,422
AudioPlaySdRaw           playRaw; //xy=154,422
AudioPlaySdAac           playAac; //xy=154,422
AudioPlaySdFlac          playFlac;

AudioMixer4              mixer1;         //xy=623.0000038146973,393.00000381469727
AudioMixer4              mixer2; //xy=626.0000038146973,490.00000381469727
AudioOutputI2S           i2s1;           //xy=831.0000057220459,452.00000381469727
AudioConnection          patchCord1(playMp31, 0, mixer1, 3);
AudioConnection          patchCord2(playMp31, 1, mixer2, 3);
AudioConnection          patchCord3(playWav, 0, mixer1, 1);
AudioConnection          patchCord4(playWav, 1, mixer2, 1);
AudioConnection          patchCord5(playAac, 0, mixer1, 0);
AudioConnection          patchCord6(playAac, 1, mixer2, 0);
AudioConnection          patchCord7(playFlac, 0, mixer1, 2);
AudioConnection          patchCord8(playFlac, 1, mixer2, 2);
AudioConnection          patchCord9(mixer1, 0, i2s1, 0);
AudioConnection          patchCord10(mixer2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=379.00000381469727,607.0000038146973
// GUItool: end automatically generated code

float volume = 0.4f;
File root, entry;

void setup() {
  // Open serial communications and wait for port to open
  while (!Serial && !Serial.available() && millis() < 5000){} 

  Serial.print(CrashReport);
  Serial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  
  AudioMemory(40);
  sgtl5000_1.enable();
  sgtl5000_1.volume(volume);
  
	mixer1.gain(0, volume);
	mixer1.gain(1, volume);
	mixer1.gain(2, volume);
	mixer1.gain(3, volume);
	
	mixer2.gain(0, volume);
	mixer2.gain(1, volume);
	mixer2.gain(2, volume);
	mixer2.gain(3, volume);
	
  if (!(SD.begin(10))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  Serial.println("initialization done.");
  root = SD.open("/");

}

void playFile(const char *filename)
{
  int filetype;
  int errAudio = 0;
  uint8_t command = 0;
  
  filetype = 0;
  if (strstr(filename, ".MP3") != NULL || strstr(filename, ".mp3") != NULL) {
      filetype = 1;
  } else if (strstr(filename, ".WAV") != NULL || strstr(filename, ".wav") != NULL ) {
      filetype = 2;
  } else if (strstr(filename, ".AAC") != NULL || strstr(filename, ".aac") != NULL) {
      filetype = 3;
  } else if (strstr(filename, ".RAW") != NULL || strstr(filename, ".raw") != NULL) {
      filetype = 4;
  } else if (strstr(filename, ".FLA") != NULL || strstr(filename, ".fla") != NULL ) {
      filetype = 5;    
  } else {
      filetype = 0;
  }
  if (filetype > 0) {
    Serial.print("Playing file: ");
    Serial.println(filename);

    switch (filetype) {
      case 1 :
        errAudio = playMp31.play( filename);

        if(errAudio != 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playMp31.isPlaying()) {
          if ( Serial.available() ) {
            command = Serial.read();
          }
          if(command == 'n') break;
          delay(250);
        }
        playMp31.stop();
        break;
      case 2 :
        errAudio = playWav.play( filename);
        if(errAudio == 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
        }
        delay(5);
        while (playWav.isPlaying()) {
          if ( Serial.available() ) {
            command = Serial.read();
          }
          if(command == 'n') break;
          delay(250);
        }
        playWav.stop();
        break;
      case 3 :
        errAudio = playAac.play( filename);
        if(errAudio != 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playAac.isPlaying()) {
          if ( Serial.available() ) {
            command = Serial.read();
          }
          if(command == 'n') break;
          delay(250);
        }
        playAac.stop();
        break;
      case 4 :
        errAudio = playRaw.play( filename);
        if(errAudio == 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playRaw.isPlaying()) {
          if ( Serial.available() ) {
            command = Serial.read();
          }
          if(command == 'n') break;
          delay(250);
        }
        playRaw.stop();
        break;
      case 5:
        //Serial.println("FLA Files not supported....");
          
        errAudio = playFlac.play( filename);
        if(errAudio != 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playFlac.isPlaying()) {
          if ( Serial.available() ) {
            command = Serial.read();
          }
          if(command == 'n') break;
          delay(250);
        }
        playFlac.stop();
        break;
      default:
        break;
    }
    delay(250);
  }
}

void loop() {
  playAll(root);
}

void playAll(File dir){
  char filename[64];
  char filnam[64];
  
   while(true) {
     entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       // rewind to begining of directory and start over
       dir.rewindDirectory();
       break;
     }
     //Serial.print(entry.name());
     if (entry.isDirectory()) {
       //Serial.println("Directory/");
       //do nothing for now
       Serial.println(entry.name());
       playAll(entry);
     } else {
       // files have sizes, directories do not
       //Serial.print("\t\t");
       //Serial.println(entry.name());
       //Serial.println(entry.size(), DEC);
       // files have sizes, directories do not
       strcpy(filename, dir.name());
       if(strlen(dir.name()) > 0) strcat(filename, "/");
       strcat(filename, strcpy(filnam, entry.name()));
       playFile(filename);
     }
   entry.close();
 }
}
