#include "codecs.h"
#include <SD.h>

class CodecFile : public CodecFileBase
{
public:

	bool fopen(const char *filename) {ftype=codec_file; AudioStartUsingSPI(); fptr=NULL; file=SD.open(filename); _fsize=file.size(); _fposition=0; return file != 0;} //FILE
	bool fopen(const uint8_t*p, const size_t size) {ftype=codec_flash; fptr=(uint8_t*)p; _fsize=size; _fposition=0; return true;} //FLASH
	bool fopen(const size_t p, const size_t size) {ftype=codec_serflash; offset=p; _fsize=size; _fposition=0; AudioStartUsingSPI(); serflashinit(); return true;} //SERIAL FLASH
	void fclose(void)
	{
		_fsize=_fposition=0; fptr=NULL;
		if (ftype==codec_file) {file.close(); AudioStopUsingSPI();}
		else
		if (ftype==codec_serflash) {AudioStopUsingSPI();}
		ftype=codec_none;
	}
	bool f_eof(void) {return _fposition >= _fsize;}
	bool fseek(const size_t position) {_fposition=position;if (ftype==codec_file) return file.seek(_fposition)!=0; else return _fposition <= _fsize;}
	size_t fposition(void) {return _fposition;}
	size_t fsize(void) {return _fsize;}
	size_t fread(uint8_t buffer[],size_t bytes){
		if (_fposition + bytes > _fsize) bytes = _fsize - _fposition;
		switch (ftype) {
			case codec_none : bytes = 0; break;
			case codec_file : bytes = file.read(buffer, bytes); break;
			case codec_flash: memcpy(buffer, _fposition + fptr, bytes); break;
			case codec_serflash: readserflash(buffer, _fposition + offset, bytes); break;
		}
		_fposition += bytes;
		return bytes;
	}

protected:
//private:

	void serflashinit(void) {
#if 0
		pinMode(10,OUTPUT);
		digitalWrite(10, HIGH);
		pinMode(SERFLASH_CS,OUTPUT);
		digitalWrite(SERFLASH_CS, HIGH);

		SPI.setMOSI(7);
		SPI.setMISO(12);
		SPI.setSCK(14);
#endif		
		SPI.begin();
		spisettings = SPISettings(SPICLOCK , MSBFIRST, SPI_MODE0);
	}
	void readserflash(uint8_t* buffer, const size_t position, const size_t bytes) {
		SPI.beginTransaction(spisettings);
		digitalWriteFast(SERFLASH_CS, LOW);
		SPI.transfer(0x0b);//CMD_READ_HIGH_SPEED
		SPI.transfer((position >> 16) & 0xff);
		SPI.transfer((position >> 8) & 0xff);
		SPI.transfer(position & 0xff);
		SPI.transfer(0);
		for(unsigned i = 0;i < bytes;i++) {
			*buffer++ = SPI.transfer(0);
		}
		digitalWriteFast(SERFLASH_CS, HIGH);
		SPI.endTransaction();
	}

	SPISettings spisettings;

	codec_filetype ftype;

	File file;
	union {
		uint8_t* fptr;
		size_t offset;
	};

	size_t _fsize;
	size_t _fposition;
}; 
