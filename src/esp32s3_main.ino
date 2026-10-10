#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <TinyGPS++.h>
//#include <MicroNMEA.h>
#include <Preferences.h>
#include <time.h>

//EspSoftwareSerial::UART Serial2;
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
TinyGPSPlus gps;
TaskHandle_t wifi_task;

float p = 3.1415926;
const unsigned int MAX_MSG_LEN = 4096;
const unsigned int MAX_TFT_SCROLL_LEN = 16;
// Threshold for declaring GPS "outdated" (e.g., 5000 milliseconds)
const unsigned long GPS_TIMEOUT_MS = 5000;
const int ESP32U_INT_PIN = 15;
int screen_no = 0;

unsigned int current_tft_scroll_index = 0;
char esp32c5_chars[MAX_MSG_LEN];
char esp32u_chars[MAX_MSG_LEN];
String tft_scroll_lines[MAX_TFT_SCROLL_LEN];
HardwareSerial MySerial(1); 
//HardwareSerial MySerial2(2); 
//HardwareSerial MySerial2(1); 

#define YEAR_2020 1577836800

int gps_baud_rate = 9600;
unsigned long lastgps = 0;
bool ledState = LOW;
bool write_file_issue = false;
volatile bool ppsTriggered = false;

unsigned long lastSyncMillis = 0;
time_t fallbackTime = 0; 
bool isTimeSynced = false;
unsigned long lastGpsTimeSync = 0; // Tracks the millis() of the last good GPS update
// Timezone offset in seconds (e.g., +28800 for UTC+8)
const long gmtOffset_sec = 0; 
bool isDateFallback = true;

int gps_allow_stale_time = 60000;

char gps_confirm_buffer[256];
char wardriver_filename_out[] = "/wardriver_out_%d.csv";
char wardriver_filename_final[64];

// TinyGPS++ global arguments
double force_lat = 0;
double force_lon = 0;
char gps_date[64];

unsigned int total_24ghz = 0;
unsigned int total_5ghz = 0;
bool valid_gps = false;

// Variables changeable from config.ini
bool wait_gps_lock = true;
String last_lats = "";
String last_lons = "";
int altitude;
double hdop;

String conn_ssid = "";
String conn_password = "";
// -----------------------------------

boolean set_sys_clock(unsigned long new_epoch){
  //Wrapper function to set sys clock to epoch value using standard POSIX functions

  struct timeval val;
  int ret;

  val.tv_sec = new_epoch;
  val.tv_usec = 0;
  ret = settimeofday(&val, NULL);

  if (ret == 0){
    return true;
  } else {
    return false;
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  //Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    //Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    //Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      //Serial.print("  DIR : ");
      //Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      //Serial.print("  FILE: ");
      //Serial.print(file.name());
      //Serial.print("  SIZE: ");
      //Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char *path) {
  //Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    //Serial.println("Dir created");
  } else {
    //Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path) {
  //Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    //Serial.println("Dir removed");
  } else {
    //Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char *path) {
  //Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    //Serial.println("Failed to open file for reading");
    return;
  }

  //Serial.print("Read from file: ");
  while (file.available()) {
    //Serial.write(file.read());
  }
  file.close();
}

bool fileExists (fs::FS &fs, const char *path) {
  //Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    //Serial.println("Failed to open file for reading");
    return false;
  }

  file.close();

  return true;
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  //Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    //Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    //Serial.println("File written");
  } else {
    //Serial.println("Write failed");
  }
  file.close();
}

bool appendFile(fs::FS &fs, const char *path, const char *message) {
  //Serial.printf("Appending to file: %s\n", path);
  bool ret_val = false;

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    //Serial.println("Failed to open file for appending");
    return ret_val;
  }
  if (file.print(message)) {
    //Serial.println("Message appended");
    ret_val = true;
  } 

  file.close();
  return ret_val;
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
  //Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    //Serial.println("File renamed");
  } else {
    //Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path) {
  //Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    //Serial.println("File deleted");
  } else {
    //Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path) {
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file) {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len) {
      size_t toRead = len;
      if (toRead > 512) {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    //Serial.printf("%lu bytes read for %" PRIu32 " ms\n", (unsigned long)flen, end);
    file.close();
  } else {
    //Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file) {
    //Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++) {
    file.write(buf, 512);
  }
  end = millis() - start;
  //Serial.printf("%u bytes written for %" PRIu32 " ms\n", 2048 * 512, end);
  file.close();
}

void drawTftScrollLines(const char *text){

  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);

  //Serial.println(text);

  tft_scroll_lines[current_tft_scroll_index % MAX_TFT_SCROLL_LEN] = text;

  current_tft_scroll_index = (current_tft_scroll_index + 1) % MAX_TFT_SCROLL_LEN;

  for ( int i = 0; i < MAX_TFT_SCROLL_LEN; i++){
    tft.println(tft_scroll_lines[(i + current_tft_scroll_index ) % MAX_TFT_SCROLL_LEN].c_str());
  }

}


void obtain_gps(){

  while (Serial2.available()){
    //Serial.println("Available from GPS!");

    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid() && 
          gps.altitude.isValid() && 
          gps.altitude.isValid() &&
          gps.hdop.isValid() &&
          gps.date.isValid() && 
          gps.time.isValid() &&
          gps.date.year() > 2000 &&
          (gps.date.isUpdated() || gps.time.isUpdated())
        ) {

        force_lat = gps.location.lat();
        force_lon = gps.location.lng();
        last_lats = String(force_lat, 6);
        last_lons = String(force_lon, 6);

  
        altitude = round(gps.altitude.meters());
        //altitude = 0;
        hdop = gps.hdop.hdop();
        //hdop = 0.;
        //strcpy(gps_date, "---------");
        struct tm gpsTime;

        valid_gps = true;

        gpsTime.tm_year = gps.date.year();
        gpsTime.tm_mon  = gps.date.month();
        gpsTime.tm_mday = gps.date.day();
        gpsTime.tm_hour = gps.time.hour();
        gpsTime.tm_min  = gps.time.minute();
        gpsTime.tm_sec  = gps.time.second();
        gpsTime.tm_isdst = 0;

        sprintf(gps_date, "%04d-%02d-%02d %02d:%02d:%02d", gpsTime.tm_year, gpsTime.tm_mon, gpsTime.tm_mday, gpsTime.tm_hour, gpsTime.tm_min, gpsTime.tm_sec);
        lastSyncMillis = millis();
        isTimeSynced = true;
        
        // Convert to UTC epoch time, then apply time zone offset
        gpsTime.tm_year -= 1900;
        gpsTime.tm_mon -= 1;

        time_t utcEpoch = mktime(&gpsTime);
        fallbackTime = utcEpoch + gmtOffset_sec;

        struct timeval now = { .tv_sec = fallbackTime, .tv_usec = 0 };
        settimeofday(&now, NULL);
        isDateFallback = false;
        lastGpsTimeSync = millis();
      }
    }
  }

  if (millis() - lastGpsTimeSync >= GPS_TIMEOUT_MS && valid_gps) {
    // GPS is out of date. Go to the local time sync  
    isDateFallback = true; 
  }

  if (isDateFallback && valid_gps){ // Fallback only when there's actually a valid time previously from GPS
    // fallback to sync time
    // Calculate exactly how many milliseconds elapsed since the last update
    unsigned long elapsedMillis = millis() - lastSyncMillis;
    
    if (elapsedMillis >= 1000) {
        // 1. Advance fallbackTime by the number of whole seconds passed
        fallbackTime += (elapsedMillis / 1000);
        
        // 2. Reset the marker, but KEEP the leftover milliseconds (remainder)
        // so you don't lose fractional timing precision
        lastSyncMillis = millis() - (elapsedMillis % 1000); 
        
        // 3. Push the advanced time into the system clock registers
        struct timeval now = { .tv_sec = fallbackTime, .tv_usec = 0 };
        settimeofday(&now, NULL);

        // 4. Read it back out to update your display string
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)){
          sprintf(gps_date, "%04d-%02d-%02d %02d:%02d:%02d", 
                  timeinfo.tm_year + 1900, 
                  timeinfo.tm_mon + 1, 
                  timeinfo.tm_mday, 
                  timeinfo.tm_hour, 
                  timeinfo.tm_min, 
                  timeinfo.tm_sec);
        }
        else {
          tft.println("BAAAAAAAAAAAAD");
          delay(1000);
        }
    }

  }

}

void await_wifi_data(void * p_task_params){

  while (true){
    
    Serial.println("S3 Start of loop");

    int count_new_wifi = 0;

    unsigned int msg_pos = 0;
    String wifi_results = "";

    int start_millis = millis();

    // Await for data to be sent by 32U or C5 as long as they're sending "Alive" = LOW signal on pulse pin.
    //int await_pulse = digitalRead(15) == LOW ? digitalRead(16) : digitalRead(18);

    //tft.setTextWrap(true);
    while ((MySerial.available() < 10) && (millis() - start_millis) < 30000){
      //tft.print(".");
      delay(1000);
      //await_pulse = digitalRead(15) == LOW ? digitalRead(16) : digitalRead(18);
    }
    //tft.setTextWrap(false);

    while (MySerial.available() > 0) {

      char rcv = (char)MySerial.read();
      if (rcv == '\n') {
        //Serial.println("MySerial Entered");
        esp32c5_chars[msg_pos] = '\0';

        char esp32c5_chars_w_date[256];
        sprintf(esp32c5_chars_w_date, esp32c5_chars, gps_date);

        wifi_results = wifi_results + String(esp32c5_chars_w_date) + "," + last_lats + "," + last_lons + "," + String(altitude) + "," + String(hdop) + ",,,WIFI" +String("\n");
        
        Serial.println((const char *)esp32c5_chars_w_date);

        //drawTftScrollLines((const char *)esp32c5_chars_w_date);
        delay(50);
        msg_pos = 0;
        ++count_new_wifi;

      } else {
        esp32c5_chars[msg_pos] = rcv;
        msg_pos += 1;
      }
    }


    MySerial.flush();
    MySerial.end();
    //Serial.println(strlen((const char *)esp32c5_chars));

    if (digitalRead(15) == HIGH) { //

      MySerial.begin(9600, SERIAL_8N1, 14, -1);
      delay(100);
      digitalWrite(17, HIGH);
      digitalWrite(15, LOW);
    }
    else {

      MySerial.begin(9600, SERIAL_8N1, 9, -1);
      delay(100);
      digitalWrite(15, HIGH);
      digitalWrite(17, LOW);
    }

      // Write result to file:

    if (wifi_results.length() > 0) {

      //tft.println((String("Writing to SD Card. DO NOT POWEROFF! ") + String(total_5ghz)).c_str());
      wifi_results.replace("\r","");
      Serial.println("||||Writing to file||||");
      //Serial.println(wifi_results.c_str());
      write_file_issue = !appendFile(SD, wardriver_filename_final, wifi_results.c_str());
    }

    if (digitalRead(15) == HIGH){
      total_24ghz += count_new_wifi;
    }
    else {
      total_5ghz += count_new_wifi;
    }

    delay(5000);
  }

}

void parse_config() {


}

void setup() {
  // put your setup code here, to run onc
  configTime(0, 0, ""); 
  tft.setRotation(3);
  Serial.begin(115200);
  // Use this initializer if you're using a 1.8" TFT
  tft.init();   // initialize a ST7735S chip
  tft.fillScreen(TFT_BLACK);

  //MySerial.begin(9600, SERIAL_8N1, 14, -1); // RX Only, Receiving data from ESP32-32u. Start MySerial (GPS/Module): Baud 9600, 8 data bits, No parity, 1 stop bit, using GPIO 14 (RX) & 12 (TX) [5].
  //MySerial2.begin(9600, SERIAL_8N1, 9, -1); // RX Only, Receiving data from ESP32-c5. Start MySerial (GPS/Module): Baud 9600, 8 data bits, No parity, 1 stop bit, using GPIO 14 (RX) & 12 (TX) [5].
  
  tft.setTextSize(0.5);
  tft.setTextWrap(false);

#ifdef REASSIGN_PINS
  SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(cs)) {
#else
  if (!SD.begin()) {
#endif
    tft.println("Card Mount Failed. Please reload board or inspect SD card");
    while (true){

    }
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    tft.println("No SD card attached. Please load SD card and reload");
    while (true){

    }
    return;
  }

  //Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    tft.println("MMC");
  } else if (cardType == CARD_SD) {
    tft.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    tft.println("SDHC");
  } else {
    tft.println("UNKNOWN");
  }

  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE);
  tft.fillScreen(TFT_BLACK);
  tft.println("Writing to hello.txt...");
  delay(1000);
  writeFile(SD, "/hello.txt", "Hello ");
  write_file_issue = appendFile(SD, "/hello.txt", "World!\n");
  tft.println();

  tft.println("Total space: ");

  String myString = String(SD.totalBytes() / (1024 * 1024)); 
  tft.print((char*) myString.c_str());
  tft.println("MB");
  

  tft.print("Used space: ");
  myString = String(SD.usedBytes() / (1024 * 1024));
  tft.print((char*) myString.c_str());
  tft.println("MB");

  delay(500);

  Serial2.begin(gps_baud_rate,SERIAL_8N1,19,20); // 19 -> ATGM336H TX, 20 -> ATGM366h RX

  Serial.print("Date/time is now: ");
  //Serial.println(dt_string_from_gps());

  Serial.begin(115200);
  MySerial.setRxBufferSize(8192); 

  //Setting up signal pins for 32U and C5 to S3:

  pinMode(15, OUTPUT_OPEN_DRAIN); // ESP32U send data signal PIN
  pinMode(16, INPUT_PULLUP); // ESP32U "Alive" signal. Alive = LOW


  pinMode(17, OUTPUT_OPEN_DRAIN); // C5 send data signal PIN
  pinMode(18, INPUT_PULLUP); // ESP32c5 "Alive" signal. Alive = LOW

  // Both do not send data yet!
  digitalWrite(15, HIGH); 
  digitalWrite(17, HIGH);

  // Setting up button signals:
  pinMode(3, INPUT); // IO3 is Move to another screen button
  pinMode(2, INPUT); // IO2 is Function button for current screen


  // Initialize wardriver save filename
  int file_counter = 0;

  while(true){

    sprintf(wardriver_filename_final, wardriver_filename_out, file_counter);

    tft.print("Attempting to write to: ");
    drawTftScrollLines(wardriver_filename_final);

    if (!fileExists(SD, wardriver_filename_final)) {
      //Set the CSV headers
      writeFile(SD, wardriver_filename_final, "WigleWifi-1.6,appRelease=1.0,model=1.0,release=1.0,device=friedtempura,display=ST7735,board=wireless_reap3r,brand=Shinigami.ph,star=Sol,body=4,subBody=0\n");
      while(!appendFile(SD, wardriver_filename_final, "MAC,SSID,AuthMode,FirstSeen,Channel,Frequency,RSSI,CurrentLatitude,CurrentLongitude,AltitudeMeters,AccuracyMeters,RCOIs,MfgrId,Type\n")){
        tft.print(".");
        delay(100);
      }

      drawTftScrollLines("\nSUCCESS WRITE to WARDFILE");
      delay(500);
      break;
    }

    file_counter++;    
  }

  // if GPS must lock

  int gps_lock_wait_time = 0;
  while (wait_gps_lock && !valid_gps){

    tft.setCursor(0, 0);
    tft.fillScreen(TFT_BLACK);
    tft.println((String("Awaiting GPS Lock: ") + String(gps_lock_wait_time) + String("s elapsed.")).c_str());
    obtain_gps();
    delay(1000);
    ++gps_lock_wait_time;
  }

  xTaskCreatePinnedToCore(
    await_wifi_data,   /* Task function */
    "await_wifi_data",        /* Name of the task */
    50000,          /* Stack size of memory in bytes */
    NULL,           /* Parameter to pass */
    1,              /* Task priority */
    &wifi_task,         /* Task handle */
    0               /* Core ID (0 or 1) */
  );

}

void loop() {


  obtain_gps();
  tft.setCursor(0, 0);
  tft.fillScreen(TFT_BLACK);

  if (screen_no == 0) {
    if (digitalRead(15) == LOW) { //

      tft.println("[32U] =====================");
    }
    else {

      tft.println("[C5] =====================");
    }
    
    tft.setTextWrap(true);

    tft.println((String("Lat: ") + last_lats).c_str());
    tft.println((String("Lon: ") + last_lons).c_str());
    tft.println((String("Alti: ") + String(altitude)).c_str());
    tft.println((String("HDOP: ") + String(hdop)).c_str());
    tft.println((String("Date: ") + String(gps_date)).c_str());

    tft.println((String("Found 2.4Ghz: ") + String(total_24ghz)).c_str());
    tft.println((String("Found 5Ghz: ") + String(total_5ghz)).c_str());

    if (!write_file_issue){
      tft.setTextColor(TFT_GREEN);
      tft.println("SD Status: WRITING/GOOD");
    }
    else {
      tft.setTextColor(TFT_RED);
      tft.println("SD Status: NOT WRITING/BAD");
    }

    
    tft.setTextColor(TFT_WHITE);
    tft.print("Is Fallback date: ");
    tft.println(isDateFallback ? "Y" : "N");

    tft.setTextWrap(false);

    Serial.println("From the regular loop");
  }
  else if (screen_no == 1){

    tft.println("Recent Seen Wifi:");
  }
  else if (screen_no == 2){

    tft.println("Status Screen");
  }
  else if (screen_no == 3){

    tft.println("Uploads Screen");
  }
  else if (screen_no == 4){

    tft.println("File List screen");
  }
  else if (screen_no == 5){

    tft.println("Config Screen");
  }
  
  delay(500);

  if (digitalRead(3) == HIGH) {
    screen_no = ( screen_no + 1 ) % 6;
  }

}


