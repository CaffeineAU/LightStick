#include <TouchScreen.h>
#include <ILI9341_t3.h>
#include <font_ArialBold.h>
#include <font_Arial.h>
#include <SysCall.h>
#include <SdFatConfig.h>
#include <SdFat.h>
#include <MinimumSerial.h>
#include <FreeStack.h>
#include <BlockDriver.h>
#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>


#define STATE_MAIN 0
#define STATE_IMAGE 1
#define STATE_DELAY 2
#define STATE_DISPLAY 3

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A0  // must be an analog pin, use "An" notation!
#define YM A6   // can be a digital pin
#define XP A5   // can be a digital pin

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

#define NUM_LEDS_PER_STRIP 144
CRGB leds[NUM_LEDS_PER_STRIP];

#define TFT_RST 8
#define TFT_DC  9
#define TFT_CS 10

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST);
#define RGB(r,g,b) (r<<11|g<<5|b)  //  R = 5 bits 0 to 31, G = 6 bits 0 to 63, B = 5 bits 0 to 31

byte colourbyte = 0;

SdFatSdioEX sd;
SdFile file;

#define ROWSPERDRAW 80
uint16_t awColors[240 * ROWSPERDRAW];  // hold colors for one row at a time...

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A0  // must be an analog pin, use "An" notation!
#define YM A6   // can be a digital pin
#define XP A5   // can be a digital pin
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 281);
#define BACKCOLOUR ILI9341_BLACK
#define FORECOLOUR ILI9341_WHITE

#define TS_MINX 169
#define TS_MAXX 916
#define TS_MINY 95
#define TS_MAXY 913

int MenuState = 0;

bool touching = false;
bool wastouching = false;
TSPoint touchup = TSPoint(-1, -1, -1);

void setup() {
	delay(200);

	tft.begin();
	tft.fillScreen(BACKCOLOUR);

	tft.setFont(Arial_10_Bold);

	Serial.print(F("Initializing SD card..."));
	sd.begin();

	Serial.println("OK!");

	//bmpDrawScale("triangle.bmp", 0, 0,2);
	int border = 5;
	//tft.fillRect((tft.width() / 4) - border, (tft.height() / 4) - border, 120 + (border * 2), 160 + (border * 2), ILI9341_BLACK);
	bmpDrawScale("Menu/MainMenu.bmp", 0, 0, 1, 320);

	LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);
	LEDS.setBrightness(64);

}

uint32_t FreeRam() { // for Teensy 3.0
	uint32_t stackTop;
	uint32_t heapTop;

	// current position of the stack.
	stackTop = (uint32_t)&stackTop;

	// current position of heap.
	void* hTop = malloc(1);
	heapTop = (uint32_t)hTop;
	free(hTop);

	// The difference is the free, available ram.
	return stackTop - heapTop;

}

char filename[255];
int xoff = 1;
int yoff = 0;
int drawscale = 4;

char prevfilename[255] = "";
String prevTouchString = "";
String touchString = "";
String prevFilenameString = "";
String filenameString = "";

// the loop function runs over and over again until power down or reset
void loop() {
	//CHSV colour = CHSV(colourbyte++, 255, 255);
	//CRGB colourRGB = CRGB(colour);

	//CHSV BGcolour = CHSV(colourbyte+128, 255, 255);
	//CRGB BGcolourRGB = CRGB(BGcolour);

	//tft.setTextColor(CL(colourRGB.red, colourRGB.green, colourRGB.blue));
	//tft.fillRoundRect(20, 20, 200, 200, 15, CL(BGcolourRGB.red, BGcolourRGB.green, BGcolourRGB.blue));
	//tft.
	//delay(20);

	TSPoint p = ts.getPoint();

	p.x = map(p.x, TS_MAXX, TS_MINX, 240, 0); p.y = map(p.y, TS_MAXY, TS_MINY, 320, 0); // map to screen coordinates

	touching = (p.z > ts.pressureThreshhold);
	if (!touching && wastouching) // 'mouse' up
	{
		touchup = p; // valid touch up event
		Serial.print("Touch up at "); Serial.print(touchup.x); Serial.print(":"); Serial.println(touchup.y);
	}
	if (touching)
	{
		touchup = TSPoint(-1, -1, -1); // clear last touch
		Serial.println("Touching...");
	}
	// we have some minimum pressure we consider 'valid'
	// pressure of 0 means no pressing!
	//delay(10);


	switch (MenuState)
	{
		Serial.print("State is "); Serial.println(MenuState);
	case STATE_MAIN:
		if (touchup.x > 0 && touchup.x < 120) // image menu
		{
			touchup = TSPoint(-1, -1, -1); // clear last touch
			Serial.println("Going to Image Menu");
			MenuState = STATE_IMAGE;
			bmpDrawScale("Menu/ImageMenu.bmp", 0, 0, 1, 320);
			sd.vwd()->rewind();
			DrawCurrentImage();
			delay(250);

		}
		break;
	case STATE_IMAGE:
		if (touchup.x > 76 && touchup.x < 228 && touchup.y > 132 && touchup.y < 300) // next button
		{
			touchup = TSPoint(-1, -1, -1); // clear last touch
			Serial.println("Going to Delay Menu");
			MenuState = STATE_DELAY;
			bmpDrawScale("Menu/DelayMenu.bmp", 0, 0, 1, 320);
			delay(250);
		}
		else if (touchup.x > 127 && touchup.x < 177 && touchup.y > 45 && touchup.y < 95) // show image
		{
			touchup = TSPoint(-1, -1, -1); // clear last touch
			DrawCurrentImage();
			delay(250);
		}

		break;
	case STATE_DELAY:
		if (touchup.x > 10 && touchup.x < 230 && touchup.y > 10 && touchup.y < 310)
		{
			MenuState = STATE_DISPLAY;

		}
		break;
	case STATE_DISPLAY:
			touchup = TSPoint(-1, -1, -1); // clear last touch
		tft.fillScreen(ILI9341_BLACK);
		tft.setTextColor(ILI9341_YELLOW);
		tft.setFont(Arial_72_Bold);
		tft.setRotation(3);
		LCDClearAndDrawString("5", prevTouchString, 90, 160);
		prevTouchString = "5";
		delay(1000);
		LCDClearAndDrawString("4", prevTouchString, 90, 160);
		prevTouchString = "4";
		delay(1000);
		LCDClearAndDrawString("3", prevTouchString, 90, 160);
		prevTouchString = "3";
		delay(1000);
		LCDClearAndDrawString("2", prevTouchString, 90, 160);
		prevTouchString = "2";
		delay(1000);
		LCDClearAndDrawString("1", prevTouchString, 90, 160);
		prevTouchString = "1";
		delay(1000);
		tft.fillScreen(ILI9341_BLACK);
		bmpDrawLEDs(filename);
		MenuState = STATE_MAIN;
		delay(5000);
		tft.setRotation(0);
		bmpDrawScale("Menu/MainMenu.bmp", 0, 0, 1, 320);

		break;
	default:
		break;
	}

	//while (file.openNext(sd.vwd(), O_READ)) {
	//	file.getName(filename, 255);
	//	//Serial.println(filename);
	//	//Serial.print("x:");
	//	//Serial.print(xoff);
	//	//Serial.print(" y:");
	//	//Serial.println(yoff);
	//	if (bmpDrawScale(filename, 60, 80,2))
	//	{
	//		//if (bmpDrawScale(filename, xoff % 240 - 1, yoff, drawscale))
	//		//{
	//		//	//delay(1500);
	//		//	//file.printName(&tft);
	//		//	xoff += (240 / drawscale);
	//		//	//if (xoff >= 240)
	//		//	//{
	//		//	//	xoff = 0;
	//		//	//}
	//		//	yoff = (xoff / 240) * (320 / drawscale);
	//		//	if (yoff >= 320)
	//		//	{
	//		//		yoff = 0;
	//		//		xoff = 1;
	//		//	}
	//		//}
	//		char filenametoprint[255];

	//		SubStringBeforeChar(filename, filenametoprint, '.');

	//		filenameString = filenametoprint;
	//		LCDClearAndDrawString(filenameString, prevFilenameString, 60, 250);
	//		prevFilenameString = filenameString;

	//		int count = 500;
	//		while (count-- > 0)
	//		{
	//			p = ts.getPoint();
	//			//Serial.println("Checking touch");
	//			if (p.z > ts.pressureThreshhold) {
	//				Serial.print("X = "); Serial.print(p.x);
	//				Serial.print("\tY = "); Serial.print(p.y);
	//				Serial.print("\tPressure = "); Serial.println(p.z);

	//				touchString = "";
	//				touchString.concat("X = "); touchString.concat(p.x);
	//				touchString.concat(" Y = "); touchString.concat(p.y);
	//				touchString.concat(" Pressure = "); touchString.concat(p.z);
	//				LCDClearAndDrawString(touchString, prevTouchString, 20, 20);
	//				prevTouchString = touchString;


	//				p.x = map(p.x, TS_MAXX, TS_MINX, 240, 0); p.y = map(p.y, TS_MAXY, TS_MINY, 320, 0);

	//				tft.fillCircle(p.x, p.y, 3, ILI9341_YELLOW); // need to calibrate
	//			}
	//			delay(1);

	//		}
	//	}
	//	file.close();
	//}
	delay(10);

	wastouching = touching;

}
void DrawCurrentImage()
{
	tft.fillRect(76, 132, 152, 168, ILI9341_WHITE);
	bool done = false;
	while (!done) {
		if (!file.openNext(sd.vwd(), O_READ))
		{
			sd.vwd()->rewind();

		}
		file.getName(filename, 255);
		done = bmpDrawScale(filename, 80, 136, 1, 160);
		file.close();
	}
}

void SubStringBeforeChar(char *in, char *out, char delimiter)
{
	int i = 0;
	while (in[i] != delimiter)
	{
		out[i] = in[i];
		i++;
	}

	out[i] = 0;

}

#define BUFFPIXEL 240

void LCDClearAndDrawString(String input, String prevInput, int x, int y)
{
	tft.setCursor(x, y);
	tft.setTextColor(BACKCOLOUR);
	tft.println(prevInput);
	tft.setTextColor(FORECOLOUR);
	tft.setCursor(x, y);
	tft.println(input);

}


bool bmpDrawScale(const char *filename, uint8_t x, uint16_t y, int scale, int maxheight) {

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * BUFFPIXEL*ROWSPERDRAW]; // pixel buffer (R+G+B per pixel)
	uint32_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip = true;        // BMP is stored bottom-to-top
	int      row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();

	if ((x >= tft.width()) || (y >= tft.height())) return false;

	Serial.println();
	Serial.print("Filename: ");
	Serial.println(filename);
	//Serial.println('\'');

	//Serial.print("x:");
	//Serial.print(x);
	//Serial.print(" y:");
	//Serial.println(y);


	// Open requested file on SD card
	if (!(bmpFile.open(filename))) {
		Serial.print(F("File not found"));
		return false;
	}

	// Parse BMP header
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (bmpHeight < maxheight)
		{
			maxheight = bmpHeight;
		}
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
										//Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
								//Serial.print(F("Image size: "));
								//Serial.print(bmpWidth);
								//Serial.print('x');
								//Serial.println(bmpHeight);

								// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				//w = bmpWidth;
				//h = bmpHeight;
				//if ((x + w - 1) >= tft.width())  w = tft.width() - x;
				//if ((y + h - 1) >= tft.height()) h = tft.height() - y;


				//for (row = 0; row < bmpHeight; row += ROWSPERDRAW) { // For each scanline...
				for (row = 0; row < maxheight; row += ROWSPERDRAW) { // For each scanline...

																	 // Seek to start of scan line.  It might seem labor-
																	 // intensive to be doing this on every line, but this
																	 // method covers a lot of gritty details like cropping
																	 // and scanline padding.  Also, the seek only takes
																	 // place if the file position actually needs to change
																	 // (avoids a lot of cluster math in SD library).
					if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - ROWSPERDRAW - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if (bmpFile.position() != pos) { // Need seek?
						bmpFile.seek(pos);
						//Serial.print("Seeking to ");
						//Serial.println(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}
					for (int i = ROWSPERDRAW; i > 0; i--)
					{
						int index = 0;

						for (col = 0; col < (bmpWidth); col++) { // For each pixel...
																 // Time to read more pixel data?
							if (buffidx >= sizeof(sdbuffer)) { // Indeed
								bmpFile.read(sdbuffer, sizeof(sdbuffer));
								buffidx = 0; // Set index to beginning
							}

							b = sdbuffer[buffidx++];
							g = sdbuffer[buffidx++];
							r = sdbuffer[buffidx++];
							// Convert pixel from BMP to TFT format, push to display
							if (i%scale == 0)
							{
								if (index++%scale == 0)
								{
									awColors[(index / scale) + (((i / scale) - 1)*bmpWidth / scale)] = tft.color565(r, g, b);
								}
								//Serial.print("col : ");
								//Serial.print(col);
								//Serial.print(" buffidx : ");
								//Serial.print(buffidx);
								//Serial.print(" row : ");
								//Serial.print(i);
								//Serial.print(" output bufferindex : ");
								//Serial.println((index / scale) + (((i / scale) - 1)*(w/scale)));
							} // end pixel
						}
					}
					tft.writeRect(x, y + (row / scale), bmpWidth / scale, ROWSPERDRAW / scale, awColors);
				} // end scanline
				Serial.print(F("Loaded in ")); Serial.print(millis() - startTime); Serial.println(" ms");
				//Serial.print(F("Free Stack :"));
				//Serial.println(FreeRam());
			} // end goodBmp
		}
	}
	else
	{
		return false;
	}
	bmpFile.close();
	if (!goodBmp) Serial.println(F("BMP format not recognized."));
	return true;
}

bool bmpDrawLEDs(const char *filename) {

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * 144]; // pixel buffer (R+G+B per pixel)
	uint32_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip = true;        // BMP is stored bottom-to-top
	int      row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();

	Serial.println();
	Serial.print("Filename: ");
	Serial.println(filename);
	//Serial.println('\'');

	//Serial.print("x:");
	//Serial.print(x);
	//Serial.print(" y:");
	//Serial.println(y);


	// Open requested file on SD card
	if (!(bmpFile.open(filename))) {
		Serial.print(F("File not found"));
		return false;
	}

	// Parse BMP header
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
										//Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
								//Serial.print(F("Image size: "));
								//Serial.print(bmpWidth);
								//Serial.print('x');
								//Serial.println(bmpHeight);

								// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				//w = bmpWidth;
				//h = bmpHeight;
				//if ((x + w - 1) >= tft.width())  w = tft.width() - x;
				//if ((y + h - 1) >= tft.height()) h = tft.height() - y;


				for (row = 0; row < bmpHeight; row++) { // For each scanline...

														// Seek to start of scan line.  It might seem labor-
														// intensive to be doing this on every line, but this
														// method covers a lot of gritty details like cropping
														// and scanline padding.  Also, the seek only takes
														// place if the file position actually needs to change
														// (avoids a lot of cluster math in SD library).
					if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if (bmpFile.position() != pos) { // Need seek?
						bmpFile.seek(pos);
						//Serial.print("Seeking to ");
						//Serial.println(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}

					for (col = 0; col < (bmpWidth); col++) { // For each pixel...
															 // Time to read more pixel data?
						if (buffidx >= sizeof(sdbuffer)) { // Indeed
							bmpFile.read(sdbuffer, sizeof(sdbuffer));
							buffidx = 0; // Set index to beginning
						}

						b = sdbuffer[buffidx++];
						g = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];
						leds[col].setRGB(r, g, b);
					} // end pixel
					LEDS.show();
					LEDS.delay(10);
					Serial.print("loading row "); Serial.println(row);
				}
			} // end scanline
			Serial.print(F("Loaded in ")); Serial.print(millis() - startTime); Serial.println(" ms");
			for (int black = 0; black < bmpWidth; black++)
			{
				leds[black].setRGB(0,0,0);
			}		
					LEDS.show();
					LEDS.delay(10);
			//Serial.print(F("Free Stack :"));
			//Serial.println(FreeRam());
		} // end goodBmp
	}
	else
	{
		return false;
	}
	bmpFile.close();
	if (!goodBmp) Serial.println(F("BMP format not recognized."));
	return true;
}



// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
	uint8_t readValues[2];
	f.read(readValues, 2);
	uint16_t *result = (uint16_t*)readValues;
	return result[0];
}

uint32_t read32(File &f) {
	uint8_t readValues[4];
	f.read(readValues, 4);
	uint32_t *result = (uint32_t*)readValues;
	return result[0];
}

