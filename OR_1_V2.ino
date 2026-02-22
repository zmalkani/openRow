// openRow 1 V2 Software for ESP32 - ISP.long
// AUTHOR  : Zach Malkani RSGC ACES '27
// COURSE  : TEJ3M (ACES 11)
// PURPOSE : Full telemetry system for a one seat of a sweep rowing boat - Force, Speed, SPM
// MCU     : ESP32
// REF     :
/*
ILI9341 Library repo. - https://github.com/adafruit/Adafruit_ILI9341
HX711 Tutorial - https://www.youtube.com/watch?v=sxzoAGf1kOo
SD Module info - https://docs.arduino.cc/libraries/sd/
Claude.ai taught me a lot too.
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <HX711.h>

//font libraries
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

//tft defs
#define TFT_cs 12
#define TFT_dc 27
#define TFT_rst 14
#define TFT_mosi 26
#define TFT_sck 25
#define TFT_miso 32

//hx711 defs
#define HX_data 23
#define HX_sck 22
const uint32_t HX_cb = 837031.06;  //calibration factor
#define HX_tare 33                  //tare button

#define baud 115200

float force = 0;
long raw = 0;
float kg = 0;

// Graph settings
#define GRAPH_X 5
#define GRAPH_Y 5
#define GRAPH_HEIGHT 100
#define GRAPH_WIDTH 310
#define MAX_FORCE 100.0
#define WIPER_WIDTH 25  // Wider wiper to clear faster when wrapping
#define X_SCALE 7  // Change this to 2 or 3 to stretch horizontally

int xPos = 0;
int lastY = GRAPH_HEIGHT / 2;

//voltage defs
float Vbatt;
#define battPin 0
unsigned long lastBattUpdate = 0;
#define BATT_UPDATE_INTERVAL 180000  // 3 minutes in milliseconds

//init tft
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_cs, TFT_dc, TFT_mosi, TFT_sck, TFT_rst, TFT_miso);
//init hx711 as sensor 1 (s1)
HX711 s1;

void setup() {
  Serial.begin(baud);

  //hx711 setup
  s1.begin(HX_data, HX_sck);
  s1.set_scale(HX_cb);
  s1.tare();

  //tft setup
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  // Draw graph border
  tft.drawRect(GRAPH_X, GRAPH_Y, GRAPH_WIDTH, GRAPH_HEIGHT, ILI9341_WHITE);
  
  // Display initial battery percentage
  Vbatt = (((analogRead(battPin) / 4095.0) * 3.3 * 2.0) - 3.0) / 1.2 * 100.0;
  Vbatt = constrain(Vbatt, 0, 100);
  tft.setCursor(5, 230);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.print("Batt: ");
  tft.print((int)Vbatt);
  tft.print("%");
}

void loop() {
  // Update battery percentage every 3 minutes
  if (millis() - lastBattUpdate >= BATT_UPDATE_INTERVAL) {
    Vbatt = (((analogRead(battPin) / 4095.0) * 3.3 * 2.0) - 3.0) / 1.2 * 100.0;
    Vbatt = constrain(Vbatt, 0, 100);
    
    // Clear old text
    tft.fillRect(5, 220, 100, 20, ILI9341_BLACK);
    
    // Display new percentage
    tft.setCursor(5, 230);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.print("Batt: ");
    tft.print((int)Vbatt);
    tft.print("%");
    
    lastBattUpdate = millis();
  }

  raw = s1.read();
  kg = s1.get_units(1);
  force = kg * 9.81;

  // Map force to y position
  int y = map((int)(force * 10), 0, (int)(MAX_FORCE * 10), GRAPH_HEIGHT - 1, 0);
  
  // Draw line from last point to current point
  tft.drawLine(GRAPH_X + xPos, GRAPH_Y + lastY, 
               GRAPH_X + xPos + X_SCALE, GRAPH_Y + y, ILI9341_CYAN);
  
  // Clear ahead of cursor (wiper effect) - starts just after the line we drew
  for (int i = 0; i < WIPER_WIDTH; i++) {
    int clearX = xPos + X_SCALE + 1 + i;
    if (clearX >= GRAPH_WIDTH - 1) {
      clearX = clearX - (GRAPH_WIDTH - 1);  // Wrap the clearing around
    }
    tft.drawFastVLine(GRAPH_X + clearX, GRAPH_Y + 1, GRAPH_HEIGHT - 2, ILI9341_BLACK);
  }
  
  // Move to next position
  xPos += X_SCALE;
  
  // Wrap around at edge
  if (xPos >= GRAPH_WIDTH - X_SCALE) {
    xPos = 0;
  }
  
  lastY = y;

  Serial.println(force, 2);

  if (digitalRead(HX_tare)) {
    s1.tare();
  }
}