
#include <MTP_Teensy.h>
#include <Audio.h>
#include <play_sd_mp3.h>
#include <play_sd_aac.h>
#include <play_sd_flac.h>

// GUItool: begin automatically generated code
AudioPlaySdMp3           playMp31;       //xy=154,78
AudioPlaySdWav           playWav; //xy=154,422
AudioPlaySdRaw           playRaw; //xy=154,422
AudioPlaySdAac           playAac; //xy=154,422
AudioPlaySdFlac          playFlac;
AudioOutputI2S           i2s1;           //xy=334,89
AudioConnection          patchCord1(playMp31, 0, i2s1, 0);
AudioConnection          patchCord2(playMp31, 1, i2s1, 1);
AudioConnection          patchCord3(playWav, 0, i2s1, 0);
AudioConnection          patchCord4(playWav, 1, i2s1, 1);
AudioConnection          patchCord5(playAac, 0, i2s1, 0);
AudioConnection          patchCord6(playAac, 1, i2s1, 1);
AudioConnection          patchCord7(playRaw, 0, i2s1, 0);
AudioConnection          patchCord8(playFlac, 0, i2s1, 0);
AudioConnection          patchCord9(playFlac, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
// GUItool: end automatically generated code
float volume = 0.4f;
char filename[256] = "SDTEST1.mp3";

//---------------------------------------------------
// Select drives you want to create
//---------------------------------------------------
#define USE_SD  1         // SDFAT based SDIO and SPI
#ifdef ARDUINO_TEENSY41
#define USE_LFS_RAM 1     // T4.1 PSRAM (or RAM)
#else
#define USE_LFS_RAM 0     // T4.1 PSRAM (or RAM)
#endif
#ifdef ARDUINO_TEENSY_MICROMOD
#define USE_LFS_QSPI 0    // T4.1 QSPI
#define USE_LFS_PROGM 1   // T4.4 Progam Flash
#define USE_LFS_SPI 0     // SPI Flash
#define USE_LFS_NAND 0
#define USE_LFS_QSPI_NAND 0
#define USE_LFS_FRAM 0
#else
#define USE_LFS_QSPI 1    // T4.1 QSPI
#define USE_LFS_PROGM 1   // T4.4 Progam Flash
#define USE_LFS_SPI 1     // SPI Flash
#define USE_LFS_NAND 1
#define USE_LFS_QSPI_NAND 0
#define USE_LFS_FRAM 0
#endif
#define USE_MSC 1    // set to > 0 experiment with MTP (USBHost.t36 + mscFS)
#define USE_SW_PU  1 //set to 1 if SPI devices does not have PUs,
                     // https://www.pjrc.com/better-spi-bus-design-in-3-steps/


#define DBGSerial Serial
FS *myfs;
File dataFile, myFile;  // Specifes that dataFile is of File type
int record_count = 0;
bool write_data = false;
uint8_t current_store = 0;

#define BUFFER_SIZE_INDEX 128
uint8_t write_buffer[BUFFER_SIZE_INDEX];
#define buffer_mult 4
uint8_t buffer_temp[buffer_mult*BUFFER_SIZE_INDEX];

int bytesRead;
uint32_t drive_index = 0;


//=============================================================================
// Global defines
//=============================================================================

MTPStorage storage;
MTPD    mtpd(&storage);

//=============================================================================
// MSC & SD classes
//=============================================================================
#if USE_SD==1
#include <SD.h>
#define USE_BUILTIN_SDCARD
#if defined(USE_BUILTIN_SDCARD) && defined(BUILTIN_SDCARD)
#define CS_SD  BUILTIN_SDCARD
#else
#define CS_SD 10
#endif
#endif


// SDClasses 
#if USE_SD==1
  // edit SPI to reflect your configuration (following is for T4.1)
  //#define SD_MOSI 11
  //#define SD_MISO 12
  //#define SD_SCK  13
  #define SD_MOSI  7
  #define SD_SCK   14
  #define SD_MISO   12
  
  #define SPI_SPEED SD_SCK_MHZ(16)  // adjust to sd card 

  #if defined (BUILTIN_SDCARD)
    const char *sd_str[]={"sdio","sd1"}; // edit to reflect your configuration
    const int cs[] = {BUILTIN_SDCARD,10}; // edit to reflect your configuration
  #else
    const char *sd_str[]={"sd1"}; // edit to reflect your configuration
    const int cs[] = {10}; // edit to reflect your configuration
  #endif
  const int nsd = sizeof(sd_str)/sizeof(const char *);
  
SDClass sdx[nsd];
#endif

// =======================================================================
// Set up MSC Drive file systems on different storage media
// =======================================================================
#if USE_MSC == 1
#include <USBHost_t36.h>
#include <USBHost_ms.h>

// Add USBHost objectsUsbFs
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
USBHub hub(myusb);

// MSC objects.
msController drive1(myusb);
msController drive2(myusb);
msController drive3(myusb);
msController drive4(myusb);

msFilesystem msFS1(myusb);
msFilesystem msFS2(myusb);
msFilesystem msFS3(myusb);
msFilesystem msFS4(myusb);
msFilesystem msFS5(myusb);

// Quick and dirty
msFilesystem *pmsFS[] = {&msFS1, &msFS2, &msFS3, &msFS4, &msFS5};
#define CNT_MSC  (sizeof(pmsFS)/sizeof(pmsFS[0]))
uint32_t pmsfs_store_ids[CNT_MSC] = {0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL};
char  pmsFS_display_name[CNT_MSC][20];

msController *pdrives[] {&drive1, &drive2, &drive3, &drive4};
#define CNT_DRIVES  (sizeof(pdrives)/sizeof(pdrives[0]))
bool drive_previous_connected[CNT_DRIVES] = {false, false, false, false};
#endif


// =======================================================================
// Set up LittleFS file systems on different storage media
// =======================================================================

#if USE_LFS_FRAM == 1 || USE_LFS_NAND == 1 || USE_LFS_PROGM == 1 || USE_LFS_QSPI == 1 || USE_LFS_QSPI_NAND == 1 || \
  USE_LFS_RAM == 1 || USE_LFS_SPI == 1
#include <LittleFS.h>
#endif

#if USE_LFS_RAM==1
const char *lfs_ram_str[] = {"RAM1"};  // edit to reflect your configuration
const int lfs_ram_size[] = {4000000}; // edit to reflect your configuration
const int nfs_ram = sizeof(lfs_ram_str)/sizeof(const char *);
LittleFS_RAM ramfs[nfs_ram];
#endif

#if USE_LFS_QSPI==1
const char *lfs_qspi_str[]={"QSPI"};     // edit to reflect your configuration
const int nfs_qspi = sizeof(lfs_qspi_str)/sizeof(const char *);
LittleFS_QSPIFlash qspifs[nfs_qspi];
#endif

#if USE_LFS_PROGM==1
const char *lfs_progm_str[]={"PROGM"};     // edit to reflect your configuration
const int lfs_progm_size[] = {1'000'000}; // edit to reflect your configuration
const int nfs_progm = sizeof(lfs_progm_str)/sizeof(const char *);
LittleFS_Program progmfs[nfs_progm];
#endif

#if USE_LFS_SPI==1
const char *lfs_spi_str[]={"sflash5", "sflash6"}; // edit to reflect your configuration
const int lfs_cs[] = {5, 6}; // edit to reflect your configuration
const int nfs_spi = sizeof(lfs_spi_str)/sizeof(const char *);
LittleFS_SPIFlash spifs[nfs_spi];
#endif

#if USE_LFS_NAND == 1
const char *nspi_str[]={"WINBOND1G", "WINBOND2G"};     // edit to reflect your configuration
const int nspi_cs[] = {3, 4}; // edit to reflect your configuration
const int nspi_nsd = sizeof(nspi_cs)/sizeof(int);
LittleFS_SPINAND nspifs[nspi_nsd]; // needs to be declared if LittleFS is used in storage.h
#endif

#if USE_LFS_QSPI_NAND == 1
const char *qnspi_str[]={"WIN-M02"};     // edit to reflect your configuration
const int qnspi_nsd = sizeof(qnspi_str)/sizeof(const char *);
LittleFS_QPINAND qnspifs[qnspi_nsd]; // needs to be declared if LittleFS is used in storage.h
#endif

void storage_configure()
{
  DateTimeFields date;
  breakTime(Teensy3Clock.get(), date);
  const char *monthname[12]={
    "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  DBGSerial.printf("Date: %u %s %u %u:%u:%u\n",
    date.mday, monthname[date.mon], date.year+1900, date.hour, date.min, date.sec);

    
  #if USE_SD==1
    #if defined SD_SCK
      SPI.setMOSI(SD_MOSI);
      SPI.setMISO(SD_MISO);
      SPI.setSCK(SD_SCK);
    #endif

    for(int ii=0; ii<nsd; ii++)
    { 
      #if defined(BUILTIN_SDCARD)
        if(cs[ii] == BUILTIN_SDCARD)
        {
          DBGSerial.printf("!! Try installing BUILTIN SD Card");
          if(!sdx[ii].begin(BUILTIN_SDCARD))
          { 
            Serial.printf("SDIO Storage %d %d %s failed or missing",ii,cs[ii],sd_str[ii]);  Serial.println();
          }
          else
          {
            storage.addFilesystem(sdx[ii], sd_str[ii]);
            uint64_t totalSize = sdx[ii].totalSize();
            uint64_t usedSize  = sdx[ii].usedSize();
            Serial.printf("SDIO Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
            Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
          }
        }
        else if(cs[ii]<BUILTIN_SDCARD)
      #endif
      {
        pinMode(cs[ii],OUTPUT); digitalWriteFast(cs[ii],HIGH);
        if(!sdx[ii].begin(cs[ii])) 
        { Serial.printf("SD Storage %d %d %s failed or missing",ii,cs[ii],sd_str[ii]);  Serial.println();
        }
        else
        {
          storage.addFilesystem(sdx[ii], sd_str[ii]);
          uint64_t totalSize = sdx[ii].totalSize();
          uint64_t usedSize  = sdx[ii].usedSize();
          Serial.printf("SD Storage %d %d %s ",ii,cs[ii],sd_str[ii]); 
          Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
        }
      }
    
    }
    #endif

#if USE_LFS_RAM==1
  for (int ii=0; ii<nfs_ram;ii++) {
    if (!ramfs[ii].begin(lfs_ram_size[ii])) {
      DBGSerial.printf("Ram Storage %d %s failed or missing",ii,lfs_ram_str[ii]);
      DBGSerial.println();
    } else {
      storage.addFilesystem(ramfs[ii], lfs_ram_str[ii]);
      uint64_t totalSize = ramfs[ii].totalSize();
      uint64_t usedSize  = ramfs[ii].usedSize();
      DBGSerial.printf("RAM Storage %d %s %llu %llu\n", ii, lfs_ram_str[ii],
        totalSize, usedSize);
    }
  }
#endif

#if USE_LFS_PROGM==1
  for (int ii=0; ii<nfs_progm;ii++) {
    if (!progmfs[ii].begin(lfs_progm_size[ii])) {
      DBGSerial.printf("Program Storage %d %s failed or missing",ii,lfs_progm_str[ii]);
      DBGSerial.println();
    } else {
      storage.addFilesystem(progmfs[ii], lfs_progm_str[ii]);
      uint64_t totalSize = progmfs[ii].totalSize();
      uint64_t usedSize  = progmfs[ii].usedSize();
      DBGSerial.printf("Program Storage %d %s %llu %llu\n", ii, lfs_progm_str[ii],
        totalSize, usedSize);
    }
  }
#endif

#if USE_LFS_QSPI==1
  for(int ii=0; ii<nfs_qspi;ii++) {
    if(!qspifs[ii].begin()) {
      DBGSerial.printf("QSPI Storage %d %s failed or missing",ii,lfs_qspi_str[ii]);
      DBGSerial.println();
    } else {
      storage.addFilesystem(qspifs[ii], lfs_qspi_str[ii]);
      uint64_t totalSize = qspifs[ii].totalSize();
      uint64_t usedSize  = qspifs[ii].usedSize();
      DBGSerial.printf("QSPI Storage %d %s %llu %llu\n", ii, lfs_qspi_str[ii], totalSize, usedSize);
    }
  }
#endif

#if USE_LFS_SPI==1
  for (int ii=0; ii<nfs_spi;ii++) {
    if (USE_SW_PU == 1) {
      pinMode(lfs_cs[ii],OUTPUT);
      digitalWriteFast(lfs_cs[ii],HIGH);
    }
    if (!spifs[ii].begin(lfs_cs[ii], SPI)) {
      DBGSerial.printf("SPIFlash Storage %d %d %s failed or missing",ii,lfs_cs[ii],lfs_spi_str[ii]);      DBGSerial.println();
    } else {
      storage.addFilesystem(spifs[ii], lfs_spi_str[ii]);
      uint64_t totalSize = spifs[ii].totalSize();
      uint64_t usedSize  = spifs[ii].usedSize();
      DBGSerial.printf("SPIFlash Storage %d %d %s %llu %llu\n", ii, lfs_cs[ii], lfs_spi_str[ii],
        totalSize, usedSize);
    }
  }
#endif
#if USE_LFS_NAND == 1
  for(int ii=0; ii<nspi_nsd;ii++) {
    if (USE_SW_PU == 1) {
      pinMode(nspi_cs[ii],OUTPUT);
      digitalWriteFast(nspi_cs[ii],HIGH);
    }
    if(!nspifs[ii].begin(nspi_cs[ii], SPI)) {
      DBGSerial.printf("SPIFlash NAND Storage %d %d %s failed or missing",ii,nspi_cs[ii],nspi_str[ii]);
      DBGSerial.println();
    } else {
      storage.addFilesystem(nspifs[ii], nspi_str[ii]);
      uint64_t totalSize = nspifs[ii].totalSize();
      uint64_t usedSize  = nspifs[ii].usedSize();
      DBGSerial.printf("Storage %d %d %s %llu %llu\n", ii, nspi_cs[ii], nspi_str[ii],
        totalSize, usedSize);
    }
  }
#endif

#if USE_LFS_QSPI_NAND == 1
  for(int ii=0; ii<qnspi_nsd;ii++) {
    if(!qnspifs[ii].begin()) {
       DBGSerial.printf("QSPI NAND Storage %d %s failed or missing",ii,qnspi_str[ii]); DBGSerial.println();
    } else {
      storage.addFilesystem(qnspifs[ii], qnspi_str[ii]);
      uint64_t totalSize = qnspifs[ii].totalSize();
      uint64_t usedSize  = qnspifs[ii].usedSize();
      DBGSerial.printf("Storage %d %s %llu %llu\n", ii, qnspi_str[ii], totalSize, usedSize);
    }
  }
#endif
 // Init it to first index storage 
 myfs = storage.getStoreFS(0);
}

void setup()
{

  // Open serial communications and wait for port to open
  while (!Serial && !DBGSerial.available() && millis() < 5000) 

  DBGSerial.print(CrashReport);
  DBGSerial.println("\n" __FILE__ " " __DATE__ " " __TIME__);
  delay(3000);
  
  //Setup Audio
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(40);

  sgtl5000_1.enable();
  sgtl5000_1.volume(volume);


  //This is mandatory to begin the mtpd session.
  mtpd.begin();
  
  storage_configure();

  #if USE_MSC == 1
  myusb.begin();
  DBGSerial.print("Initializing MSC Drives ...");
  DBGSerial.println("\nInitializing USB MSC drives...");
  DBGSerial.println("MSC and MTP initialized.");
  checkMSCChanges();
  #endif
  DBGSerial.println("\nSetup done");
  menu();
}

int ReadAndEchoSerialChar() {
  int ch = DBGSerial.read();
  if (ch >= ' ') DBGSerial.write(ch);
  return ch;
}

uint8_t storage_index = '0';
void loop()
{
  if ( DBGSerial.available() ) {
    uint8_t command = DBGSerial.read();
    int ch = DBGSerial.read();
    if ('2'==command) storage_index = CommandLineReadNextNumber(ch, 0);
    while (ch == ' ') ch = DBGSerial.read();

    uint32_t fsCount;
    switch (command) {
    case '1':
      // first dump list of storages:
      fsCount = storage.getFSCount();
      DBGSerial.printf("\nDump Storage list(%u)\n", fsCount);
      for (uint32_t ii = 0; ii < fsCount; ii++) {
        if ( ii == storage_index )
          DBGSerial.print("STORE");
        else
          DBGSerial.print("store");
        DBGSerial.printf(":%u storage:%x name:%s fs:%x\n", ii, mtpd.Store2Storage(ii),
          storage.getStoreName(ii), (uint32_t)storage.getStoreFS(ii));
      }
      //DBGSerial.println("\nDump Index List");
      //storage.dumpIndexList();
      break;
    case '2':
      if (storage_index < storage.getFSCount()) {
        DBGSerial.printf("Storage Index %u Name: %s Selected\n", storage_index,
          storage.getStoreName(storage_index));
        myfs = storage.getStoreFS(storage_index);
        current_store = storage_index;
      } else {
        DBGSerial.printf("Storage Index %u out of range\n", storage_index);
      }
      break;

    case 'l': listFiles(); break;
    case 'e': eraseFiles(); break;
    case 'p':
    {
      write_data = true;   // sets flag to continue to write data until new command is received
      int errorMp3 = 0;
      DBGSerial.print("Playing file: ");
      if (ch > ' ') {
        char *psz = filename;
        while (ch > ' ') {
          *psz++ = ch;
          ch = Serial.read();
        }
        *psz = '\0';
      }

      DBGSerial.println(filename);
      // Start playing the file.  This sketch continues to
      // run while the file plays.
      #if 1
      playFile(myfs, filename);
      #else

      errorMp3 = playMp31.play(myfs, filename);
      DBGSerial.printf("play() error = %d\r\n",errorMp3);
      // Simply wait for the file to finish playing.
      if(errorMp3 != 0) write_data = false;
      #endif      
      break;
    }
    case 'P': 
      playDir(myfs);
      DBGSerial.println("Finished Playlist");
      break;
    case'r':
      DBGSerial.println("Reset");
      mtpd.send_DeviceResetEvent();
      break;
    case 'R':
      DBGSerial.print(" RESTART Teensy ...");
      delay(100);
      SCB_AIRCR = 0x05FA0004;
      break;
    case '\r':
    case '\n':
    case 'h': menu(); break;
    }
    while (DBGSerial.read() != -1) ; // remove rest of characters.
  } else {
    #if USE_MSC == 1
    checkMSCChanges();
    #endif
    mtpd.loop();
  }

  if (write_data) {
    while (playMp31.isPlaying()) {
      // uncomment these lines if your audio shield
      // has the optional volume pot soldered
      //  float vol = analogRead(15);
      //  vol = vol / 1024;
      //  sgtl5000_1.volume(vol);
      //   delay(250);
    }
  }
}

#if USE_MSC == 1
void checkMSCChanges() {
  myusb.Task();

  USBMSCDevice mscDrive;
  PFsLib pfsLIB;
  for (uint8_t i=0; i < CNT_DRIVES; i++) {
    if (*pdrives[i]) {
      if (!drive_previous_connected[i]) {
        if (mscDrive.begin(pdrives[i])) {
          Serial.printf("\nUSB Drive: %u connected\n", i);
          pfsLIB.mbrDmp(&mscDrive, (uint32_t)-1, Serial);
          Serial.printf("\nTry Partition list");
          pfsLIB.listPartitions(&mscDrive, Serial);
          drive_previous_connected[i] = true;
        }
      }
    } else {
      drive_previous_connected[i] = false;
    }
  }
  bool send_device_reset = false;
  for (uint8_t i = 0; i < CNT_MSC; i++) {
    if (*pmsFS[i] && (pmsfs_store_ids[i] == 0xFFFFFFFFUL)) {
      Serial.printf("Found new Volume:%u\n", i); Serial.flush();
      // Lets see if we can get the volume label:
      char volName[20];
      if (pmsFS[i]->mscfs.getVolumeLabel(volName, sizeof(volName)))
        snprintf(pmsFS_display_name[i], sizeof(pmsFS_display_name[i]), "MSC%d-%s", i, volName);
      else
        snprintf(pmsFS_display_name[i], sizeof(pmsFS_display_name[i]), "MSC%d", i);
      pmsfs_store_ids[i] = storage.addFilesystem(*pmsFS[i], pmsFS_display_name[i]);

      // Try to send store added. if > 0 it went through = 0 stores have not been enumerated
      if (mtpd.send_StoreAddedEvent(pmsfs_store_ids[i]) < 0) send_device_reset = true;
    }
    // Or did volume go away?
    else if ((pmsfs_store_ids[i] != 0xFFFFFFFFUL) && !*pmsFS[i] ) {
      if (mtpd.send_StoreRemovedEvent(pmsfs_store_ids[i]) < 0) send_device_reset = true;
      storage.removeFilesystem(pmsfs_store_ids[i]);
      // Try to send store added. if > 0 it went through = 0 stores have not been enumerated
      pmsfs_store_ids[i] = 0xFFFFFFFFUL;
    }
  }
  if (send_device_reset) mtpd.send_DeviceResetEvent();
}
#endif


void menu()
{
  DBGSerial.println();
  DBGSerial.println("Menu Options:");
  DBGSerial.println("\t1 - List Drives (Step 1)");
  DBGSerial.println("\t2# - Select Drive # for Logging (Step 2)");
  DBGSerial.println("\tl - List files on disk");
  DBGSerial.println("\te - Erase files on disk with Format");
  DBGSerial.println("\tp - Play Hardcoded test file");
  DBGSerial.println("\tP - Play all music on selected device");
  DBGSerial.println("\tr - Reset MTP");
  DBGSerial.printf("\t%s","R - Restart Teensy");
  DBGSerial.println("\n\th - Menu");
  DBGSerial.println();
}

void listFiles()
{
  DBGSerial.print("\n Space Used = ");
  DBGSerial.println(myfs->usedSize());
  DBGSerial.print("Filesystem Size = ");
  DBGSerial.println(myfs->totalSize());

  printDirectory(myfs);
}

void eraseFiles()
{
  //DBGSerial.println("Formating not supported at this time");
  DBGSerial.println("\n*** Erase/Format started ***");
  myfs->format(1, '.', DBGSerial);
  DBGSerial.println("Completed, sending device reset event");
  mtpd.send_DeviceResetEvent();
}

void printDirectory(FS *pfs) {
  DBGSerial.println("Directory\n---------");
  printDirectory(pfs->open("/"), 0);
  DBGSerial.println();
}

void printDirectory(File dir, int numSpaces) {
  while (true) {
    File entry = dir.openNextFile();
    if (! entry) {
      //DBGSerial.println("** no more files **");
      break;
    }
    printSpaces(numSpaces);
    DBGSerial.print(entry.name());
    if (entry.isDirectory()) {
      DBGSerial.println("/");
      printDirectory(entry, numSpaces + 2);
    } else {
      // files have sizes, directories do not
      printSpaces(36 - numSpaces - strlen(entry.name()));
      DBGSerial.print("  ");
      DBGSerial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void printSpaces(int num) {
  for (int i = 0; i < num; i++) {
    DBGSerial.print(" ");
  }
}


uint32_t CommandLineReadNextNumber(int &ch, uint32_t default_num) {
  while (ch == ' ') ch = DBGSerial.read();
  if ((ch < '0') || (ch > '9')) return default_num;

  uint32_t return_value = 0;
  while ((ch >= '0') && (ch <= '9')) {
    return_value = return_value * 10 + ch - '0';
    ch = DBGSerial.read();
  }
  return return_value;
}


void playFile(FS* pfs, const char *filename)
{
  int filetype;
  int errAudio = 0;

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
    Serial.printf("Playing(%d) file: '%s'\n", filetype, filename);
    while (Serial.read() != -1) ; // clear out any keyboard data...
    
    switch (filetype) {
      case 1 :
        errAudio = playMp31.play(myfs, filename);
        if(errAudio != 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playMp31.isPlaying()) {
          if (Serial.available()){Serial.println("User Abort"); break;}
          delay(250);
        }
        playMp31.stop();
        break;
      case 2 :
        errAudio = playWav.play(pfs, filename);
        if(errAudio == 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playWav.isPlaying()) {
          if (Serial.available()){Serial.println("User Abort"); break;}
          delay(250);
        }
        playWav.stop();
        break;
      case 3 :
        errAudio = playAac.play(pfs, filename);
        if(errAudio != 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playAac.isPlaying()) {
          if (Serial.available()){Serial.println("User Abort"); break;}
          delay(250);
        }
        playAac.stop();
        break;
      case 4 :
        errAudio = playRaw.play(pfs, filename);
        if(errAudio == 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playRaw.isPlaying()) {
          if (Serial.available()){Serial.println("User Abort"); break;}
          delay(250);
        }
        playRaw.stop();
        break;
      case 5:
        //Serial.println("FLA Files not supported....");
        errAudio = playFlac.play(pfs, filename);
        if(errAudio != 0) {
          Serial.printf("Audio Error: %d\n", errAudio);
          break;
        }
        delay(5);
        while (playFlac.isPlaying()) {
          if (Serial.available()){Serial.println("User Abort"); break;}
          Serial.print(".");
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

void playDir(FS *pfs) {
  DBGSerial.println("Playing files on device");
  playAll(pfs, pfs->open("/"));
  DBGSerial.println();
}

void playAll(FS* pfs, File dir){
  char filename[64];
  char filnam[64];
   while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       // no more files
       // rewind to begining of directory and start over
       dir.rewindDirectory();
       break;
     }
     //DBGSerial.print(entry.name());
     if (entry.isDirectory()) {
       //DBGSerial.println("Directory/");
       //do nothing for now
       //DBGSerial.println(entry.name());
       playAll(pfs, entry);
     } else {
       // files have sizes, directories do not
       //DBGSerial.print("\t\t");
       //DBGSerial.println(entry.size(), DEC);
       // files have sizes, directories do not
       strcpy(filename, dir.name());
       if(strlen(dir.name()) > 0) strcat(filename, "/");
       strcat(filename, strcpy(filnam, entry.name()));
       playFile(pfs, filename);
     }
   entry.close();
 }
}
