#include <HardwareSerial.h>
#include "WiFi.h"
#include <vector>

// Define a new serial port instance (UART1)
// Parameters: (UART_NUM, RX_PIN, TX_PIN)
HardwareSerial MySerial(1); // UART1 instance

struct DetectedWifi {

  String detectedwifi;
  unsigned long lastmillisdetected;
  bool sent;

};

std::vector<String> detected_wifi;
std::unordered_map<uint64_t, DetectedWifi> detected_wifi_map;
const int send_data_pin = 21;


uint64_t bssidToKey(const uint8_t* bssid) {
    if (!bssid) return 0;
    uint64_t key = 0;
    for (int i = 0; i < 6; i++) {
        key = (key << 8) | bssid[i];
    }
    return key;
}

void setup() {

  Serial.begin(115200); // For debugging via USB
  MySerial.begin(9600, SERIAL_8N1, -1, 25); // 9600 baud, 8 data bits, no parity, 1 stop bit, RX=GPIO14, TX=GPIO12
  //WiFi.STA.begin();
  //WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  WiFi.setBandMode(WIFI_BAND_MODE_5G_ONLY);

  pinMode(2, INPUT_PULLUP);

  pinMode(3, OUTPUT_OPEN_DRAIN);
  digitalWrite(3, LOW);

  Serial.println("Esp32C5 Init");

}

void loop() {
  // Send a simple message
  Serial.println("start of loop 1");
  
  
  //MySerial.println("HardwareSerial (MySerial) Example Started");
  
  Serial.println("Hello from ESP32 C5 Hardware Serial!");
  
   //Serial.println("Hello!");
  delay(2000);
  Serial.println("Starting Wi-Fi scan...");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks(false,true,false,300);
  //int n = WiFi.scanNetworks(false, true);
  Serial.println("Scan done");

  //int n = 0;
  //Serial.println("Scan done");

  
  if (n == 0) {
    //digitalWrite(GPIO, HIGH);
    Serial.println("No networks found");
  } else {
    
    Serial.print(n);
    Serial.println(" networks found");
    Serial.println("-------------------------------------");

    for (int i = 0; i < n; ++i) {
      // Print network details

      const uint8_t* rawBssid = WiFi.BSSID(i);
      uint64_t keyBssid = bssidToKey(rawBssid);

      auto foundBssid = detected_wifi_map.find(keyBssid); 

      if (foundBssid != detected_wifi_map.end()) {
        detected_wifi_map[keyBssid].lastmillisdetected = millis();
        continue;
      }


      String bssid = WiFi.BSSIDstr(i);
      //Serial.print(" | ");
      String ssid = WiFi.SSID(i); // Network name

      String authtype = "";
      switch (WiFi.encryptionType(i)) {
        case WIFI_AUTH_OPEN:
          authtype = "[OPEN]";
          break;
      
        case WIFI_AUTH_WEP:
          authtype = "[WEP]";
          break;
      
        case WIFI_AUTH_WPA_PSK:
          authtype = "[WPA_PSK]";
          break;
      
        case WIFI_AUTH_WPA2_PSK:
          authtype = "[WPA2_PSK]";
          break;
      
        case WIFI_AUTH_WPA_WPA2_PSK:
          authtype = "[WPA_WPA2_PSK]";
          break;
      
        case WIFI_AUTH_WPA2_ENTERPRISE:
          authtype = "[WPA2]";
          break;

        //Requires at least v2.0.0 of https://github.com/espressif/arduino-esp32/
        case WIFI_AUTH_WPA3_PSK:
          authtype = "[WPA3_PSK]";
          break;

        case WIFI_AUTH_WPA2_WPA3_PSK:
          authtype = "[WPA2_WPA3_PSK]";
          break;

        case WIFI_AUTH_WAPI_PSK:
          authtype = "[WAPI_PSK]";
          break;
            
        default:
          authtype = "[UNDEFINED]";
      }

      //Serial.print(" | ");
      //Serial.print(authtype);

      //Serial.print(" (");
      String signal_strength = String(WiFi.RSSI(i)); // Signal strength
      //Serial.print(" dBm), Ch: ");
      int channel = WiFi.channel(i); // Channel number
      
      //                                                          DATE
      String result = bssid + "," + ssid + "," + authtype + "," + "%s" + "," + channel + "," + String(5000 + (channel * 5)).c_str() + "," + signal_strength;;


      detected_wifi_map[keyBssid].detectedwifi = result;
      detected_wifi_map[keyBssid].lastmillisdetected = millis();
      detected_wifi_map[keyBssid].sent = false;
      //Serial.println(result.c_str());
      //detected_wifi.push_back(result);
      //Serial.println(result.c_str());
    }

      // Check if the channel is in the 5GHz range (channels > 14 are generally 5GHz)
  }

  if (digitalRead(2) == LOW) {
    Serial.println("C5 Will now be sending data...");
    //MySerial.end();
    //MySerial.begin(9600, SERIAL_8N1, -1, 14);
    /*
    while (!detected_wifi.empty()) {
      Serial.println(detected_wifi.back().c_str());
      MySerial.println(detected_wifi.back().c_str());
      MySerial.flush();
      detected_wifi.pop_back();
    }
    Serial.println("C5 Done sending Data");
    */
    //MySerial.end();
    //MySerial.begin(9600, SERIAL_8N1, 14, -1);
    for (auto curr_wifi = detected_wifi_map.begin(); curr_wifi != detected_wifi_map.end(); ) {
        
      // Send the wifi AP if it was not yet sent
      if (!curr_wifi->second.sent) {
        
        Serial.println(curr_wifi->second.detectedwifi.c_str());
        MySerial.println(curr_wifi->second.detectedwifi.c_str());
        MySerial.flush();

        curr_wifi->second.sent = true;
      }

      // If an entry is already old (older than 1 minute) and already sent, remove the entry
      if (millis() - curr_wifi->second.lastmillisdetected >= 60000 && curr_wifi->second.sent){
        curr_wifi = detected_wifi_map.erase(curr_wifi);
      }
      else {
        ++curr_wifi;  
      }
    }

  }
  else {

    Serial.println("C5 Will not be sending data.");
  }

  Serial.println("++++++++++++++++++++++++++");
}