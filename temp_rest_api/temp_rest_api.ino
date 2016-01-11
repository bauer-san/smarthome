// Adafruit IO REST API access with ESP8266
//

#define DEBUG
#define SUPER_SAVER   1
#define BENCH_DEBUG   0

#include <ESP8266WiFi.h>
#include <max6675.h>
#include "Adafruit_IO_Client.h"

#ifndef mycommon
  #include <My_Common.h>
#endif

#define sleep_mins 1
#define sleep_us (sleep_mins * 60 * 1E6)
#define avgSAMPLES 3

#if (BENCH_DEBUG)
  #define tempRADIO_ON       50.0      //150.0
  #define tempSAUNA_COOL     70.0      //160.0
  #define tempSAUNA_HOT      75.0      //185.0
#else
  #define tempRADIO_ON      150.0
  #define tempSAUNA_COOL    160.0
  #define tempSAUNA_HOT     185.0
#endif
#define delay_get_temp_ms 500
#define READY 1
#define UNREADY 0

// Configure the max6675
#define pinVcc_MAX6755 15
#define pinCS_MAX6755 5

// Configure the SPI.
#define pinDO 4
#define pinCLK 14

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
Adafruit_IO_Feed tempFeed = aio.getFeed(AIO_FEEDNAME_SAUNA_TEMP);
Adafruit_IO_Feed readyFeed = aio.getFeed(AIO_FEEDNAME_SAUNA_READY);

void setup() {
  // Setup serial port access.
  Serial.begin(115200);
  delay(10);
  
  Serial.println(); Serial.println();
  Serial.println(F("**********************************************************************"));
  Serial.println(F("Read from MAX6675 and conditionally post to Adafruit IO using ESP8266!"));
  Serial.println(F("**********************************************************************"));

  // Set up max6675 power pin and turn it on
  pinMode(pinVcc_MAX6755, OUTPUT); digitalWrite(pinVcc_MAX6755, HIGH);
  
  Serial.println(F("Ready!"));  
}

void loop() {
  double tempF=0.0, AvgtempF=0.0, LBO_tempF;
  char i, Nsamples = avgSAMPLES;
  int LBO_ready;
  
  //Read samples
  for (i=0;i<Nsamples;i++) {
    delay(delay_get_temp_ms);
    // Sum the temperature readings
    tempF += thermocouple.readFahrenheit();
  }

  //Turn off the max6675
  digitalWrite(pinVcc_MAX6755, LOW);

  // Simple average
  AvgtempF = (tempF/Nsamples);
  Serial.print(F("feedtmp: ")); Serial.println(AvgtempF, DEC);

  /*******************************************************************************************************/
  /* Extend the battery life by only turning on the wifi radio when the temperature is above a threshold */
  /*******************************************************************************************************/
  if (AvgtempF >= tempRADIO_ON) {

    // Turn on the WiFi radio and assume that it worked (like in the examples)
    WiFi.mode(WIFI_STA);
    delay(500);

    // Connect to WiFi access point.
    Serial.print(F("Connecting to "));
    Serial.println(WLAN_SSID);
  
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    delay(2000);  // Increasing this seems to improve the reliability of the connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(F("."));
    }
  
    Serial.println();
    Serial.print(F("WiFi connected as IP address: ")); Serial.println(WiFi.localIP());
  
    // Initialize the Adafruit IO client class (not strictly necessary with the
    // client class, but good practice).
    aio.begin();

#if SUPER_SAVER
    // Get the previous TEMPerature
    FeedData lastTemp = tempFeed.receive();
    if (lastTemp.isValid()) {
      Serial.print(F("Received value from tempFeed: ")); Serial.println(lastTemp);
      if (lastTemp.doubleValue(&LBO_tempF)) {
        Serial.print(F("Same value as an double: ")); Serial.println(LBO_tempF, DEC);
      }
    }

    // Write the new TEMP
    if (tempFeed.send(AvgtempF)) {
      Serial.print(F("Wrote value to tempFeed: ")); Serial.println(AvgtempF, DEC);
    } else {
      Serial.println(F("Error writing value to tempFeed!"));
    }
#endif

    // Get the previous state
    FeedData lastReady = readyFeed.receive();    
    if (lastReady.isValid()) {
      Serial.print(F("Received value from readyFeed: ")); Serial.println(lastReady);
      if (lastReady.intValue(&LBO_ready)) {
        Serial.print(F("Same value as an int: ")); Serial.println(LBO_ready, DEC);
      }
    }
    
    // Decide if the readyFeed should be updated
    if (   (LBO_ready == UNREADY)
        && (AvgtempF > tempSAUNA_HOT))   {// Is hot enough
        if (readyFeed.send(READY)) {        // report READY!
          Serial.print(F("Wrote value to readyFeed: ")); Serial.println(READY, DEC);
        } else {
          Serial.println(F("Error writing READY value to readyFeed!"));
        }
    } else { //(LBO_ready == READY)
      if  (AvgtempF <= tempSAUNA_COOL)  {   // cooling off or warming up
        if (readyFeed.send(UNREADY)) {      // report UNREADY
          Serial.print(F("Wrote value to readyFeed: ")); Serial.println(UNREADY, DEC);
        } else {
          Serial.println(F("Error writing UNREADY value to readyFeed!"));
        }
      }
    }
  }
  
  Serial.println(F("Going to sleep"));
  ESP.deepSleep(sleep_us, WAKE_RF_DISABLED);
  delay(1000);

}
