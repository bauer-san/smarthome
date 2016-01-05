// Adafruit IO REST API access with ESP8266
//

#define DEBUG

#include <ESP8266WiFi.h>
#include <max6675.h>
#include "Adafruit_IO_Client.h"

#ifndef mycommon
  #include <My_Common.h>
#endif

#define sleep_mins_long 10
#define sleep_mins_short 1
#define numSample 3
#define tempThr 80.0
#define delay_get_temp_ms 500

// Configure the max6675
#define pinVcc_MAX6755 15
#define pinCS_MAX6755 5

// Configure the SPI.
#define pinDO 4
#define pinCLK 14

#define sleep_us_long (sleep_mins_long * 60 * 1E6)
#define sleep_us_short (sleep_mins_short * 60 * 1E6)

// Create thermocouple instance of max6675
MAX6675 thermocouple(pinCLK, pinCS_MAX6755, pinDO);

// Create an ESP8266 WiFiClient class to connect to the AIO server.
WiFiClient client;

// Create an Adafruit IO Client instance.  Notice that this needs to take a
// WiFiClient object as the first parameter, and as the second parameter a
// default Adafruit IO key to use when accessing feeds (however each feed can
// override this default key value if required, see further below).
Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);

// Finally create instances of Adafruit_IO_Feed objects, one per feed.  Do this
// by calling the getFeed function on the Adafruit_IO_FONA object and passing
// it at least the name of the feed, and optionally a specific AIO key to use
// when accessing the feed (the default is to use the key set on the
// Adafruit_IO_Client class).
Adafruit_IO_Feed testFeed = aio.getFeed(AIO_FEEDNAME);

void setup() {
  // Setup serial port access.
  Serial.begin(115200);
  delay(10);
  Serial.println(); Serial.println();
  Serial.println(F("Read from MAX6675 and post to Adafruit IO using ESP8266!"));

  // Connect to WiFi access point.
  Serial.print(F("Connecting to "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  
  Serial.println();
  Serial.print(F("WiFi connected as IP address: ")); Serial.println(WiFi.localIP());
  
  // Initialize the Adafruit IO client class (not strictly necessary with the
  // client class, but good practice).
  aio.begin();

  // Set up max6675 power pin and turn it on
  pinMode(pinVcc_MAX6755, OUTPUT); digitalWrite(pinVcc_MAX6755, HIGH);
  
  Serial.println(F("Ready!"));  
}

void loop() {
  double tempF=0.0, feedtemp=0.0;
  char i, Nsamples = numSample;

  //Read samples
  for (i=0;i<Nsamples;i++) {
    delay(delay_get_temp_ms);
    // Sum the temperature readings
    tempF += thermocouple.readFahrenheit();
  }

  //Turn off the max6675
  digitalWrite(pinVcc_MAX6755, LOW);

  // Simple average
  feedtemp = (tempF/Nsamples);
  Serial.print(F("feedtmp: ")); Serial.println(feedtemp, DEC);

  if (testFeed.send(feedtemp)) {
    Serial.print(F("Wrote value to feed: ")); Serial.println(feedtemp, DEC);
  }
  else {
    Serial.println(F("Error writing value to feed!"));
  }

  if (feedtemp >= tempThr) {
    Serial.println(F("Short sleep"));
    ESP.deepSleep(sleep_us_short, WAKE_RF_DEFAULT);
  } else {
    Serial.println(F("Long sleep"));    
    ESP.deepSleep(sleep_us_long, WAKE_RF_DEFAULT);
  }
  
  delay(1000);

}
