#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "secrets.h"

const char* WLAN_SSID = SECRET_WLAN_SSID;
const char* WLAN_PASSWD = SECRET_WLAN_PASSWD;
const char* SERVER_URL = SECRET_SERVER_URL;

struct {
  uint32_t crc32;   // 4 bytes
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t ap_mac[6];// 6 bytes, 11 in total
  uint8_t padding;  // 1 byte,  12 in total
} rtcData;

IPAddress ip( 192, 168, 1, 35 );// pick your own suitable static IP address
IPAddress gateway( 192, 168, 1, 254 ); // may be different for your network
IPAddress subnet( 255, 255, 255, 0 ); // may be different for your network (but this one is pretty standard)
IPAddress dns(192,168,1,254);

#define LOADING_LED D4
#define SUCCESS_LED D3
#define FAILURE_LED D2

void connectWIFI() {
  Serial.printf("Connecting to %s...\n", WLAN_SSID);

  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );

  bool rtcValid = false;
  if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
    Serial.println("Read RTC memory");
    uint32_t crc = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
    if( crc == rtcData.crc32 ) {
      Serial.println("CRC matches");
      rtcValid = true;
    } else {
      Serial.println("CRC does not match");
    }
  } else {
    Serial.println("No RTC memory");
  }

  WiFi.forceSleepWake();
  delay( 1 );

  WiFi.persistent( false );

  WiFi.mode( WIFI_STA );
  WiFi.config(ip, dns, gateway, subnet );
  Serial.println("Configured WiFi");

  if( rtcValid ) {
    Serial.println("The RTC data was good, making a quick connection...");
    WiFi.begin( WLAN_SSID, WLAN_PASSWD, rtcData.channel, rtcData.ap_mac, true );
  }
  else {
    Serial.println("The RTC data was not valid, so making a regular connection...");
    WiFi.begin( WLAN_SSID, WLAN_PASSWD );
  }

  int retries = 0;
  int wifiStatus = WiFi.status();
  Serial.println("Waiting for connection...");
  while( wifiStatus != WL_CONNECTED ) {
    retries++;
    if( rtcValid && retries == 100 ) {
      Serial.println("Quick connect is not working, resetting WiFi and trying regular connection");
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin( WLAN_SSID, WLAN_PASSWD );
    }
    if( retries == 600 ) {
      Serial.println("Giving up after 30 seconds and going back to sleep");
      WiFi.disconnect( true );
      delay( 1 );
      WiFi.mode( WIFI_OFF );
      fail();
    }
    Serial.println("Connected!");
    delay( 50 );
    wifiStatus = WiFi.status();
  }
  
  rtcData.channel = WiFi.channel();
  memcpy( rtcData.ap_mac, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  rtcData.crc32 = calculateCRC32( ((uint8_t*)&rtcData) + 4, sizeof( rtcData ) - 4 );
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );
  Serial.println("Copied RTC memory");
}

void requestServer() {
  Serial.printf("Making HTTPS GET request to %s\n", SERVER_URL);
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  
  HTTPClient https;
  https.begin(*client, SERVER_URL);
  int httpCode = https.GET();
  if (httpCode>0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    if (httpCode < 200 || httpCode > 299) {
      fail();
    }
  }
  else {
    Serial.printf("HTTP GET Failed: (%d) %s\n", httpCode, https.errorToString(httpCode).c_str());
    fail();
  }
  https.end();
}

void setup() {
  pinMode(LOADING_LED, OUTPUT);
  pinMode(SUCCESS_LED, OUTPUT);
  pinMode(FAILURE_LED, OUTPUT);
  digitalWrite(LOADING_LED, LOW);
  digitalWrite(SUCCESS_LED, HIGH);
  digitalWrite(FAILURE_LED, HIGH);
  
  Serial.begin(115200);
  while(!Serial) {}
  Serial.println("\nWoken from deep sleep");
  
  connectWIFI();
  requestServer();
  digitalWrite(LOADING_LED, HIGH);
  digitalWrite(SUCCESS_LED, LOW);

  delay(1000);
  digitalWrite(SUCCESS_LED, HIGH);

  WiFi.disconnect( true );
  delay( 1 );

  ESP.deepSleep(0, WAKE_RF_DISABLED );
}

uint32_t calculateCRC32( const uint8_t *data, size_t length ) {
  uint32_t crc = 0xffffffff;
  while( length-- ) {
    uint8_t c = *data++;
    for( uint32_t i = 0x80; i > 0; i >>= 1 ) {
      bool bit = crc & 0x80000000;
      if( c & i ) {
        bit = !bit;
      }

      crc <<= 1;
      if( bit ) {
        crc ^= 0x04c11db7;
      }
    }
  }

  return crc;
}

void fail() {
  digitalWrite(LOADING_LED, HIGH);
  digitalWrite(SUCCESS_LED, HIGH);
  digitalWrite(FAILURE_LED, LOW);
  delay(1000);
  ESP.deepSleep(0, WAKE_RF_DISABLED );
}

void loop() {
}
