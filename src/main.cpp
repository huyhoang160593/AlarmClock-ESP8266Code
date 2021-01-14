#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS1307.h>
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

RtcDS1307<TwoWire> Rtc(Wire);

const unsigned char Active_buzzer = 14;
const char* serverSetupTime = "https://keepurlshort.herokuapp.com/api/alarmMessages/setup";
const char* getNearestAlarm = "https://keepurlshort.herokuapp.com/api/alarmMessages/nearest";

unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay5s = 5000;

bool clockFlag = true;

//Get the time from wifi - fix the issue if the RTCDS broken
const long utcOffsetInSeconds = 25200;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Make a document to hold the JSON and can be access later
StaticJsonDocument<200> doc;

String sensorReadings;

int alarmHour = -1, alarmMinutes = -1;
char* codeMessage;
char* displayCode;

LiquidCrystal_I2C lcd(0x27, 16, 2);

//GET Request from internet(HTTPS, no fingerprint, less secure)
String httpGETRequest(const char* serverName) {
  HTTPClient http;
  WiFiClientSecure client;

  //where the magic happen, use with caution
  client.setInsecure();
  client.connect(serverName,443);
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  int httpResponseCode = 0;
  String payload = "{}"; 
  // Send HTTP POST request
  while (true){

    httpResponseCode = http.GET();

    if (httpResponseCode == 200 || httpResponseCode == 204) {
      Serial.println("HTTP Response code: ");
      Serial.println(httpResponseCode);
      payload = http.getString();
      // Free resources
      http.end();
      return payload;

    }else {
      Serial.println("Error code: ");
      Serial.println(httpResponseCode);
      lcd.clear();
      lcd.print("Reset GET...");
    }
  } 
}

//convert const char* to char* and store is in the value
char* copy(const char* origin) {
    char *res = new char[strlen(origin)+1];
    strcpy(res, origin);
    return res;
}

//malloc the memory to address a char*, after using this you should free the buffer with free(char*)
char* tranformString(const char* url){
  String sensorReadings = httpGETRequest(url);
  Serial.println("The payload you get are:");
  Serial.println(sensorReadings);

  //Config the string into char array to make it readable with the ArduinoJson
  int jsonLength = sensorReadings.length() + 1;

  char* json = (char*) malloc(jsonLength);

  sensorReadings.toCharArray(json, jsonLength);
  return json;
}

bool setupTime(){

  lcd.print("Get Time");
  Rtc.Begin();
  Rtc.SetIsRunning(true);
  lcd.print(".");
  if(!Rtc.GetIsRunning()){
    Serial.println("This RTC DS1307 is broken");
    lcd.clear();
    lcd.print(".");
    lcd.print(".");
    return false;
  }
  //Change the JSON get from internet into a char*
  char* timeJSON = tranformString(serverSetupTime);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, timeJSON);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return false;
  }
  lcd.print(".");
  //access the JSON document and save the value in it
  int year = doc["year"];
  int month = doc["month"];
  int day = doc["day"];
  int hours = doc["hours"];
  int minutes = doc["minutes"];
  int seconds = doc["seconds"];

  lcd.clear();
  lcd.print("Setup time...");

  RtcDateTime datetime(year,month,day,hours,minutes,seconds);

  Rtc.SetDateTime(datetime);
  lcd.print(".");
  lcd.clear();
  return true;
}

bool setupAlarm(){
  //Change the JSON get from internet into a char*
  char* alarmJSON = tranformString(getNearestAlarm);
  // Deserialize the JSON document
  DeserializationError errorAlarm = deserializeJson(doc, alarmJSON);
  // Test if parsing succeeds.
  if (errorAlarm) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(errorAlarm.c_str());
    return false;
  }
  lcd.print(".");
  //set the value for alarm
  alarmHour = doc["hours"];
  alarmMinutes = doc["minutes"];

  lcd.print(".");
  //ArduinoJson returns keys and values as const char* so what we do here is try to convert is to char* for our purpose
  const char* constCodeMessage = doc["code"];
  codeMessage = copy(constCodeMessage);

  lcd.print(".");
  return true;
}

void morseCodeDot(){
  digitalWrite(Active_buzzer,HIGH) ; //Turn on active buzzer
  delay(100);
  digitalWrite(Active_buzzer,LOW) ; //Turn off active buzzer
  delay(100); 
}

void morseCodeDash(){
  digitalWrite(Active_buzzer,HIGH) ; //Turn on active buzzer
  delay(200);
  digitalWrite(Active_buzzer,LOW) ; //Turn off active buzzer
  delay(100); 
}

void successBuzzer(){
  // ... ..- -.-. -.-. . ... ...
  lcd.clear();
  lcd.print("S");
  morseCodeDot(); morseCodeDot(); morseCodeDot(); //S
  delay(300);
  lcd.print("U");
  morseCodeDot(); morseCodeDot(); morseCodeDash(); //U
  delay(300);
  lcd.print("C");
  morseCodeDash(); morseCodeDot(); morseCodeDash(); morseCodeDot(); //C
  delay(300);
  lcd.print("C");
  morseCodeDash(); morseCodeDot(); morseCodeDash(); morseCodeDot(); //C
  delay(300);
  lcd.print("E");
  morseCodeDot(); //E
  delay(300);
  lcd.print("S");
  morseCodeDot(); morseCodeDot(); morseCodeDot(); morseCodeDot(); //S
  delay(300);
  lcd.print("S");
  morseCodeDot(); morseCodeDot(); morseCodeDot(); morseCodeDot(); //S
}

void messageBuzzer(){
  // -- . ... ... .- --. .
  lcd.clear();
  lcd.print("M");
  morseCodeDash(); morseCodeDash(); //M
  delay(300);
  lcd.print("E");
  morseCodeDot(); // E
  delay(300);
  lcd.print("S");  
  morseCodeDot(); morseCodeDot(); morseCodeDot(); // S
  delay(300);
  lcd.print("S");
  morseCodeDot(); morseCodeDot(); morseCodeDot(); // S
  delay(300);
  lcd.print("A");
  morseCodeDot(); morseCodeDash(); // A
  delay(300);
  lcd.print("G");
  morseCodeDash(); morseCodeDash(); morseCodeDot(); // G
  delay(300);
  lcd.print("E");
  morseCodeDot(); // E
}


void setup() {
  // put your setup code here, to run once:
  //Start the Serial and Pin
  Serial.begin(9600);
  //Start the pin(GPIO)
  Wire.begin(D2,D1);
  pinMode (Active_buzzer,OUTPUT);

  //initial the LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Setup wifi...");

  //Wifi setting up
  WiFiManager wifiManager;
  wifiManager.resetSettings();  //For testing purpose, now everytime you reset the module, you need to config the wifi again
  wifiManager.autoConnect("NodeMCU-Arduino-PlatformIO");

  lcd.clear();
  lcd.print("Connected!");
  Serial.println("Done connect to wifi!");

  //Success Buzzer
  successBuzzer();

  //Start the timer
  lcd.clear();
  
  if(setupTime()){
    lcd.print("Time done...");
    lcd.clear();

  } else{
    lcd.println("failed, useNTPClient");
    timeClient.begin();
    Serial.println("Use NTPClient instead");
    clockFlag = false;
    delay(1000);
    lcd.clear();
  }
  //Get the nearest alarm in setup
  lcd.print("Set the alarm");
  if(setupAlarm()){
    lcd.clear();
    lcd.print("Alarm done...");
  }

  delay(3000);
}  // end of setup


void loop() {
  int hour,minutes,second;
  if(clockFlag){
    RtcDateTime now = Rtc.GetDateTime();
    hour = now.Hour();
    minutes = now.Minute();
    second = now.Second();
  } else {
    timeClient.update();
    hour = timeClient.getHours();
    minutes = timeClient.getMinutes();
    second = timeClient.getSeconds();
  }

  //Display the first row
    lcd.setCursor(0,0); 
    lcd.print("Time: "); 
    lcd.print(hour, DEC); 
    lcd.print(':'); 
    lcd.print(minutes, DEC); 
    lcd.print(':'); 
    lcd.print(second, DEC);  
    lcd.print("   ");

    //Display the second row
    lcd.setCursor(0,1);
    lcd.print("Next: ");
    if(alarmHour!=-1){
      lcd.print(alarmHour, DEC); lcd.print(":"); lcd.print(alarmMinutes, DEC); lcd.print("  ");
    }else{
      lcd.print("None ");
    }
    lcd.print(displayCode);

    //When the alarm come, display the code and reset the counter for update alarm(defect)
    if(hour == alarmHour && minutes == alarmMinutes){
      messageBuzzer();
      displayCode = codeMessage;
      lastTime = millis();
      alarmHour = -1;
      alarmMinutes = -1;
    }

  // Update alarm every 10 minutes
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      setupAlarm();
    }
    else {
      Serial.println("WiFi Disconnected, Please restart the wifi");
    }
    lastTime = millis();
  }
}


  //cheking I2C device to connect to LCD, put in setup when you need
  
  // while(!Serial) // Waiting for serial connection
  //   {
  //   };

  // Serial.println();
  // Serial.println("I2C scanner. Scanning ...");
  // byte count = 0;
  
  // Wire.begin();
  // for (byte i = 8; i < 120; i++)
  // {
  //   Wire.beginTransmission (i);
  //   if (Wire.endTransmission () == 0)
  //     {
  //       Serial.print("Found address: ");
  //       Serial.print(i, DEC);
  //       Serial.print(" (0x");
  //       Serial.print(i, HEX);
  //       Serial.println(")");
  //       count++;
  //       delay (1);  // maybe unneeded?
  //     }; // end of good response
  // }; // end of for loop

  // Serial.println("Done.");
  // Serial.print("Found ");
  // Serial.print(count, DEC);
  // Serial.println(" device(s).");

