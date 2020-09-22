#include <Arduino.h>

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#include <WiFiManager.h>
#include <LiquidCrystal_I2C.h>
#include <RtcDS1307.h>
#include <Wire.h>

//Tomorrow will be finish

RtcDS1307<TwoWire> Rtc(Wire);

const unsigned char Active_buzzer = 14;
const char* serverSetupTime = "https://alarmmessagex.herokuapp.com/api/alarmMessages/setup";
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

String sensorReadings;
float sensorReadingsArr[6];

String hr,minut ;
int hr1, minut1;
String new_time;

LiquidCrystal_I2C lcd(0x27, 16, 2);

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  WiFiClientSecure client;

  //where the magic happen, use with caution
  client.setInsecure();
  client.connect(serverName,443);

  Serial.print(client);
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.println("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
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
  morseCodeDot();
  morseCodeDot();
  morseCodeDot();
  delay(300);
  morseCodeDot();
  morseCodeDot();
  morseCodeDash();
  delay(300);
  morseCodeDash();
  morseCodeDot();
  morseCodeDash();
  morseCodeDot();
  delay(300);
  morseCodeDash();
  morseCodeDot();
  morseCodeDash();
  morseCodeDot();
  delay(300);
  morseCodeDot();
  delay(300);
  morseCodeDot();
  morseCodeDot();
  morseCodeDot();
  morseCodeDot();
  delay(300);
  morseCodeDot();
  morseCodeDot();
  morseCodeDot();
  morseCodeDot();
}


void setup() {
  // put your setup code here, to run once:
  //Start the Serial and Pin
  Serial.begin(9600);
  //Start the pin(GPIO)
  Wire.begin(D2,D1);
  pinMode (Active_buzzer,OUTPUT);

  //cheking I2C device to connect to LCD
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

  //Success Buzzer
  successBuzzer();

  //Start the timer
  lcd.clear();
  lcd.print("Get Time...");
  sensorReadings = httpGETRequest(serverSetupTime);

  Serial.println(sensorReadings);

  //Config the string into char array to make it readable with the ArduinoJson
  int jsonLength = sensorReadings.length() + 1;

  char json[jsonLength];

  sensorReadings.toCharArray(json, jsonLength);

  StaticJsonDocument<200> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  int year = doc["year"];
  int month = doc["month"];
  int day = doc["day"];
  int hours = doc["hours"];
  int minutes = doc["minutes"];
  int seconds = doc["seconds"];

  lcd.clear();
  lcd.print("Setup time...");
  Rtc.Begin();

  RtcDateTime datetime(year,month,day,hours,minutes,seconds);

  Rtc.SetDateTime(datetime);
  Rtc.SetIsRunning(true);

  lcd.clear();
  lcd.print("All done");
  delay(3000);
  Serial.println("Done setting!");
}  // end of setup


void loop() {
  RtcDateTime now = Rtc.GetDateTime();

  lcd.setCursor(0,0);
  lcd.print("Time: ");
  lcd.print(now.Hour(), DEC);
  lcd.print(':');
  lcd.print(now.Minute(), DEC);
  lcd.print(':');
  lcd.print(now.Second(), DEC);

  if(now.Second() == 0){
    delay(1000);
    lcd.clear();
  }
}
