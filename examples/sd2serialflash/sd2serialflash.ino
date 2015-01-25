/*

   W25Q128FV Serial Flasher
   
   (c) Frank Boesing, 2014,Dec
   GNU License Version 3.

   Teensy Audio Shield (c) Paul Stoffregen 
   W25Q128FV - Library  (c) Pete (El Supremo) 
   Thank you both!!!!
      
   Reads directory "/SERFLASH" on SD and burns 
   all files to the optional serial flash.
  
   Version 20141227
      
*/

#include <Audio.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <flash_spi.h>

#define FLASHSIZE 0x1000000
#define PAGE      256
#define DIRECTORY "SERFLASH"



File dir;
File entry;
unsigned char id_tab[32];
unsigned pos;
unsigned page;
int fsize = 0;
int fcnt = 0;
unsigned char buf[PAGE];

String filename[500];
unsigned int position[500];

bool verify(void)
{

	unsigned char buf2[PAGE];

    fcnt = 0;
    pos = 0; 
    page = 0;     
    Serial.println("Verify.");
    dir = SD.open(DIRECTORY);
    while(1) {
      entry = dir.openNextFile();
      if (!entry) break;
      pos = page * PAGE;
      Serial.printf("%d. Verifying \"%s\" at position: 0x%07X...", fcnt+1, entry.name(), pos);
	  filename[fcnt] = entry.name();
	  position[fcnt] = pos;
      int rd =0;
      do {
        memset(buf, 0xff, PAGE);
        rd = entry.read(buf, PAGE);
        flash_read_pages(buf2, page, 1);
        int v = 0;
        for (int i=0; i<PAGE; i++) v+= buf[i]-buf2[i];
        if (v) {Serial.println("is not ok."); return false;}
        pos += rd;         
        page++;
      } while (rd==PAGE);          
      Serial.printf("ok.\r\n");
      entry.close(); 
      fcnt++;
    }
    return true;
}

void flash(void)
{
	unsigned char buf[PAGE];
    fcnt = 0;
    pos = 0; 
    page = 0;     
    dir = SD.open(DIRECTORY);
    while(1) {
      entry = dir.openNextFile();
      if (!entry) break;
      pos = page * PAGE;
      Serial.printf("%d. Flashing \"%s\" at position: 0x%07X...", fcnt+1, entry.name(), pos);
      int rd =0;
      do {
        memset(buf, 0xff, PAGE);          
        rd = entry.read(buf, PAGE);
        pos += rd;         
        flash_page_program(buf,page);
        page++;
      } while (rd==PAGE);          
      Serial.printf("done.\r\n");
      entry.close(); 
      fcnt++;
    }
}

void erase(void) {
    Serial.println("Erasing chip....");
    flash_chip_erase(true);
    Serial.println("done.");
}

void setup()
{
  SPI.setMOSI(7);
  SPI.setSCK(14);
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
   while (!Serial) {;}  
  delay(1000); 
  Serial.print("\r\n\r\nW25Q128FV Serial Flasher \r\nInitializing SD card...");
  if (!SD.begin(10)) {
    Serial.println("failed!");
    return;
  }  
  Serial.println("done.\r\n");
  dir = SD.open(DIRECTORY);
  fsize = 0;
  fcnt = 0;
  while(1) 
  {
    entry = dir.openNextFile();
    if (!entry) break;    
    int s = entry.size();   
    if ( (s & 0xff) > 0) s |= 0xff;
    Serial.printf("%s\t\t\t%d Bytes\r\n", entry.name(), s);
    fsize += s;   
    entry.close();  
    fcnt++;
  } 
  dir.close();
  
  Serial.printf("\t\t\t%d File(s), %d Bytes\r\n", fcnt, fsize);

  if (!fsize) goto ready;
  if (fsize < FLASHSIZE) {
      flash_init();
      int flashstatus = flash_read_status();
      flash_read_id(id_tab);
      Serial.printf("Flash Status: 0x%X, ID:0x%X,0x%X,0x%X,0x%X ", flashstatus , id_tab[0], id_tab[1], id_tab[2], id_tab[3]);   
      if (id_tab[0]!=0xef || id_tab[1]!=0x40 || id_tab[2]!=0x18 || id_tab[3]!=0x00) {Serial.println(" is not ok."); goto end;}
      Serial.printf(" is ok.\r\nFile(s) fit(s) in serial flash, %d Bytes remaining.\r\n\r\n", FLASHSIZE - fsize);

      Serial.print("Check flash content: ");
      if (verify()) { Serial.println("Flash content ok. Nothing to do.");goto end; }

      erase();
      flash();      
      verify();      

end:            
      dir.close();
  }
  else Serial.println("Does not fit.");  
  
ready:  
  Serial.println("Ready.");
  if (fsize) { 
	Serial.printf("Copy'n paste:\r\n\r\nconst int SPIFlash[%d] = {\r\n",fcnt);
	for (int i = 0; i<fcnt; i++) 
		Serial.printf("\t\t0x%07X%s //\"%s\"\r\n",position[i], ((i<fcnt-1)?", ":"};"), filename[i].c_str());
	
	
	Serial.printf("\r\nconst char SPIFlashFilename[%d][13] = {\r\n",fcnt);
	for (int i = 0; i<fcnt; i++) 
		Serial.printf("\t\t\"%s\"%s\r\n", filename[i].c_str(),((i<fcnt-1)?",":"};"));	
  }
  
}



const int SPIFlash[7] = {
                0x0000000,  //"201159~1.RAW"
                0x002C800,  //"82583_~1.RAW"
                0x0053300,  //"86334_~1.RAW"
                0x0065B00,  //"86773_~1.RAW"
                0x01F1D00,  //"102790~1.RAW"
                0x020A100,  //"171104~1.RAW"
                0x0214900}; //"P2.RAW"

const char SPIFlashFilename[7][13] = {
                "201159~1.RAW",
                "82583_~1.RAW",
                "86334_~1.RAW",
                "86773_~1.RAW",
                "102790~1.RAW",
                "171104~1.RAW",
                "P2.RAW"};



void loop()
{}


