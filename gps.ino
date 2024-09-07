#include <SD.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int chipSelect = 4; // chip select for SD module

static const int RXPin = 3, TXPin = 2; // pins for ATGM336H GPS device
static const uint32_t GPSBaud = 9600; // default baudrate of ATGM336H GPS device

TinyGPSPlus gps;
SoftwareSerial ss(TXPin, RXPin);

#define WIRE Wire
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &WIRE);

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1000);

  display.clearDisplay();
  display.display();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  ss.begin(GPSBaud);
}

void loop() {
  if (ss.available() > 0) {
    if (gps.encode(ss.read()) &&
        gps.location.isValid() &&
        gps.date.isValid() &&
        gps.time.isValid())
    {
      String fileName = String(gps.date.year()) + String(gps.date.month()) + String(gps.date.day()) + ".csv";

      File dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        dataFile.println(String(gps.date.year()) + "." +
                         String(gps.date.month()) + "." +
                         String(gps.date.day()) + "," +
                         String(gps.time.hour()) + ":" +
                         String(gps.time.minute()) + ":" +
                         String(gps.time.second()) + "," +
                         String(gps.location.lat(), 8) + "," +
                         String(gps.location.lng(), 8));
        dataFile.close();

        display.clearDisplay();
        display.println(String(gps.location.lat(), 8) + "; " + String(gps.location.lng(), 8));
        display.setCursor(0, 0);
        display.display();

        delay(1000);
      }
    }
  }
}
