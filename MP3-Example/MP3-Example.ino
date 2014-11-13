/*
	Helix library Arduino interface
	MP3-Player Example

	Copyright (c) 2014 Frank Bösing

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this library.  If not, see <http://www.gnu.org/licenses/>.

	The helix decoder itself as a different license, look at the subdirectories for more info.

	Dieses Programm ist freie Software: Sie können es unter den Bedingungen
	der GNU General Public License, wie von der Free Software Foundation,
	Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
	veröffentlichten Version, weiterverbreiten und/oder modifizieren.

	Dieses Programm wird in der Hoffnung, dass es nützlich sein wird, aber
	OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
	Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
	Siehe die GNU General Public License für weitere Details.

	Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
	Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.

	Der Helixdecoder selbst hat eine eigene Lizenz, bitte für mehr Informationen
	in den Unterverzeichnissen nachsehen.

 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#include <Helix.h>

// GUItool: begin automatically generated code
AudioPlayQueue           queue1;         //xy=267,183
AudioPlayQueue           queue2;         //xy=268,227
AudioOutputI2S           i2s1;           //xy=420,202
AudioConnection          patchCord1(queue1, 0, i2s1, 0);
AudioConnection          patchCord2(queue2, 0, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=263,109
// GUItool: end automatically generated code

HelixMp3 mp3;

#define PIN_SPI_SCK			14
#define PIN_SPI_MOSI		 7
#define PIN_SPI_SDCARD_CS	10			//SD-Card CS


void setup() {

	SPI.setMOSI(PIN_SPI_MOSI);
	SPI.setSCK(PIN_SPI_SCK);

	if (!SD.begin(PIN_SPI_SDCARD_CS)) {
		while(1) {
			Serial.println("Unable to access the SD card.");
			delay(500);
		}
	}

	// Audio connections require memory to work.
	// Should be >=18 for MP3, all buffers are you define here will be used.
	AudioMemory(18);

	sgtl5000_1.enable();
	//sgtl5000_1.enhanceBassEnable();
	sgtl5000_1.volume(0.5);
}

void loop() {
	// Filename is 8.3 only:
	mp3.play("whisper.mp3", &queue1, &queue2);
	
	delay(1500);
}
