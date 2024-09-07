#include <SD.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <SPI.h>

const int chipSelect = 4; // chip select for SD module

static const int RXPin = 3, TXPin = 2; // pins for ATGM336H GPS device
static const uint32_t GPSBaud = 9600; // default baudrate of ATGM336H GPS device

TinyGPSPlus gps;
SoftwareSerial ss(TXPin, RXPin);

void setup() {
  Serial.begin(9600);
  ss.begin(GPSBaud);

  if (!SD.begin(chipSelect)) { // verify SD card and module are working
    Serial.println("SD Card not found");
    while (true);
  }

  Serial.println("Init complete");
}

void loop() {
  if (ss.available() > 0) {
    if (gps.encode(ss.read()) &&
        gps.location.isValid() &&
        gps.date.isValid() &&
        gps.time.isValid())
    {
      Serial.println("GPS detected!");

      String fileName = String(gps.date.year()) + String(gps.date.month()) + String(gps.date.day()) + ".csv";

      File dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        String line = String(gps.date.year()) + "." +
                      String(gps.date.month()) + "." +
                      String(gps.date.day()) + "," +
                      String(gps.time.hour()) + ":" +
                      String(gps.time.minute()) + ":" +
                      String(gps.time.second()) + "," +
                      String(gps.location.lat(), 8) + "," +
                      String(gps.location.lng(), 8);

        dataFile.println(line);
        dataFile.close();

        Serial.println(line);

        delay(1000);
      }
    }
  }
}
