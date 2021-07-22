// Simple advanced MP3 file player example
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//   or dac / pwm / SD shield
//
// Example sketch by Dnstje.nl
// Act like a real MP3/AAC player up to 255 songs.
// Buttons at port 2 3 4, feel free to remap them.
// I don't have a Audioboard yet, just a SDcard breakout and using the onboard DAC and PWM outputs.
//
// This sketch will also using: EEPROM, Bounce library, Serial, SD.
//
// Sending char's to serial port to change tracks.
// P = previous track, N = Next track, S = Stop track, O = Play track I = Pause track, K = list tracklist, C = current track, R = Random track selection.
//
//
// 
// MP3/AAC library code by Frank Boesing.


#define AUDIO_BOARD // if using audioboard 
//#define PWMDAC // if using DAC and PWM as output.  (dac ~ pin 14 = left ch, PWM ~ pin 3 ~ 4 + resistor dac right ch)

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Bounce.h> //Buttons
#include <EEPROM.h> // store last track

#include <play_sd_mp3.h> //mp3 decoder
#include <play_sd_aac.h> // AAC decoder

#define BUTTON1 2  //NEXT
#define BUTTON2 3  //Play Pause
#define BUTTON3 4  //PREV 

Bounce bouncer1 = Bounce(BUTTON1, 50); 
Bounce bouncer2 = Bounce(BUTTON2, 50); 
Bounce bouncer3 = Bounce(BUTTON3, 50);

const int chipSelect = BUILTIN_SDCARD;  // inbuilt SD
//const int chipSelect = 10;  // if using another pin for SD card CS.

int track;
int tracknum;
int trackext[255]; // 0= nothing, 1= mp3, 2= aac, 3= wav.
String tracklist[255];
File root;

boolean trackchange;
boolean paused;

AudioPlaySdMp3           playMp31; 
AudioPlaySdAac           playAac1;  
#ifdef PWMDAC
AudioOutputAnalog        dac1;   
AudioOutputPWM           pwm1;   
#endif

#ifdef AUDIO_BOARD
AudioOutputI2S           i2s1;   
#endif

AudioMixer4              mixleft;
AudioMixer4              mixright;
//mp3
AudioConnection          patch1(playMp31,0,mixleft,0);
AudioConnection          patch2(playMp31,1,mixright,0);
//aac

AudioConnection          patch1a(playAac1, 0, mixleft, 1);
AudioConnection          patch2a(playAac1, 1, mixright, 1);

#ifdef PWMDAC
AudioConnection          patch3(mixleft,0,dac1,0);
AudioConnection          patch4(mixright,0,pwm1,0);
#endif

#ifdef AUDIO_BOARD
AudioConnection          patch5(mixleft, 0, i2s1, 0);
AudioConnection          patch6(mixright, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
#endif


void setup() {
  Serial.begin(115200);

#ifdef AUDIO_BOARD
  sgtl5000_1.enable();
  sgtl5000_1.volume(0.5);

  SPI.setMOSI(7);
  SPI.setSCK(14);

#endif
  //setup pins with pullups
  pinMode(BUTTON1,INPUT_PULLUP);
  pinMode(BUTTON3,INPUT_PULLUP);
  pinMode(BUTTON2,INPUT_PULLUP);  
  // reads the last track what was playing.
  track = EEPROM.read(0); 

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(16);
  //put the gain a bit lower, some MP3 files will clip otherwise.
  mixleft.gain(0,0.9);
  mixright.gain(0,0.9);

  //Start SD card
  if (!(SD.begin(chipSelect))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  //Starting to index the SD card for MP3/AAC.
  root = SD.open("/");

  while(true) {

    File files =  root.openNextFile();
    if (!files) {
      //If no more files, break out.
      break;
    }
    String curfile = files.name(); //put file in string
    String curfileUpper = curfile;
    curfileUpper.toUpperCase();
   // Serial.println(curfile);
    //look for MP3 or AAC files
    int m = curfileUpper.lastIndexOf(".MP3");
    int a = curfileUpper.lastIndexOf(".AAC");
    int a1 = curfileUpper.lastIndexOf(".MP4");
    int a2 = curfileUpper.lastIndexOf(".M4A");
    //int w = curfileUpper.lastIndexOf(".WAV");

    // if returned results is more then 0 add them to the list.
    if(m > 0 || a > 0 || a1 > 0 || a2 > 0 ){  
      Serial.println(curfile);
      tracklist[tracknum] = files.name();
      if(m > 0) trackext[tracknum] = 1;
      if(a > 0) trackext[tracknum] = 2;  
      if(a1 > 0) trackext[tracknum] = 2;
      if(a2 > 0) trackext[tracknum] = 2;
      //  if(w > 0) trackext[tracknum] = 3;
      //Serial.print(m);
      tracknum++;  
    }
    // close 
    files.close();
  }
  //check if tracknum exist in tracklist from eeprom, like if you deleted some files or added.
  if(track > tracknum){
    //if it is too big, reset to 0
    EEPROM.write(0,0);
    track = 0;
  }
  
}

void playFileMP3(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.print(track);
  Serial.print(" - ");
  Serial.println(filename);
  trackchange = true; //auto track change is allowed.
  // Start playing the file.  This sketch continues to
  // run while the file plays.
  EEPROM.write(0,track); //meanwhile write the track position to eeprom address 0
  playMp31.play(filename);


  // Simply wait for the file to finish playing.
  while (playMp31.isPlaying()) {
    // update controls!
    controls();
    serialcontrols();
  }
}

void playFileAAC(const char *filename)
{
  Serial.print("Playing file: ");
  Serial.print(track);
  Serial.print(" - ");
  Serial.println(filename);
  trackchange = true; //auto track change is allowed.
  // Start playing the file.  This sketch continues to
  // run while the file plays.
  EEPROM.write(0,track); //meanwhile write the track position to eeprom address 0
  playAac1.play(filename);


  // Simply wait for the file to finish playing.
  while (playAac1.isPlaying()) {
    // update controls!
    controls();
    serialcontrols();
  }
}

void controls() {
  bouncer1.update();
  bouncer2.update();
  bouncer3.update();
  if ( bouncer1.fallingEdge()) { 
    nexttrack();
  }
  if ( bouncer2.fallingEdge()) {  
    pausetrack();
  }
  if ( bouncer3.fallingEdge()) { 
    prevtrack();
  }  
}

void serialcontrols(){
  if(Serial.available()){
    byte incomingByte = Serial.read();

    switch(incomingByte){

    case 78:     //Next track  N
      nexttrack();
      break;

    case 80:     //Previous track P
      prevtrack();
      break;

    case 73:     //pause track I
      pausetrack();
      break;               

    case 75:     //List tracklist K
      //list our tracklist over serial
      for(int i =0; i < tracknum;i++){
        Serial.print(i);
        Serial.print("\t");
        Serial.print(tracklist[i]);
        Serial.print("\t");
        Serial.println(trackext[i]);
      }        
      break;  

    case 67: //current track C
      Serial.print("current track = ");      
      Serial.print(track);  
      Serial.print(" - ");  
      Serial.println(tracklist[track]);    
      break;  
    case 82: //random track
      randomtrack();
      break;
    }         
    incomingByte = 0;

  }

}



void loop() {


  Serial.println();
  Serial.println();
  Serial.println(tracknum);
  Serial.println(track);
  Serial.println(trackext[track]);
  Serial.println(tracklist[track]);


  if(trackext[track] == 1){
    Serial.println("MP3" );
    playFileMP3((const char*)tracklist[track].c_str());
  }else if(trackext[track] == 2){
    Serial.println("aac");
    playFileAAC((const char*)tracklist[track].c_str());
  }


  if(trackchange == true){ //when your track is finished, go to the next one. when using buttons, it will not skip twice.
    nexttrack();
  }
  delay(100);
}

void nexttrack(){
  Serial.println("Next track!");
  trackchange=false; // we are doing a track change here, so the auto trackchange will not skip another one.
  playMp31.stop();
  playAac1.stop();
  track++;
  if(track >= tracknum){ // keeps in tracklist.
    track = 0;
  }    
}

void prevtrack(){
  Serial.println("Previous track! ");
  trackchange=false; // we are doing a track change here, so the auto trackchange will not skip another one.
  playMp31.stop();
  playAac1.stop();
  track--;
  if(track <0){ // keeps in tracklist.
    track = tracknum-1;
  }  
  
}

void pausetrack(){
  if(paused){
    Serial.println("Playing!");
  }
  else{
    Serial.println("Paused!");
  }
  paused = playMp31.pause(!paused);
}


void randomtrack(){
  Serial.println("Random track!");
  trackchange=false; // we are doing a track change here, so the auto trackchange will not skip another one.
  if(trackext[track] == 1) playMp31.stop();
  if(trackext[track] == 2) playAac1.stop();

  track= random(tracknum);
  
}
