/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on NodeMCU.

  Note: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right NodeMCU module
  in the Tools -> Board menu!

  For advanced settings please follow ESP examples :
   - ESP8266_Standalone_Manual_IP.ino
   - ESP8266_Standalone_SmartConfig.ino
   - ESP8266_Standalone_SSL.ino

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */

// This project involves feedback from TDS meter and water levels to keep hydroponic 
// fluids for a hydoponic garden at the correct values.
// needed for this project a tds meter,  a quad relay board, 3 fluid float sensors.
// also we selected the ESP8266 NodeMCU 1.0 (ESP-12E Module) for this project
// it is also setup to allow OTA Over The Air updates
// a Blynk account is also needed or you need to setup a blynk server like I did for this project.

#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>

#define TdsSensorPin A0
#define pumpA D0
#define pumpB D1

// pump A and B calibration numbers
// pump calibration is in mili seconds per mili Liter
// this calibration method was used due to delay in c is in mili seconds and can be an int
int pumpA_ms_per_mL = 668;
int pumpB_ms_per_mL = 750;
// to calibrate I manually control each pump to fill 100ml container and measure that time in seconds
// example it takes pump B about 133 seconds to fill 100mL container so 1/(133sec/100mL*1000ms) = 751ms/mL

float temperature=30;

// for voltage accuracy at low end less than 1.7v set to 3.15 
#define VREF 3.15
// for accuracy on high over 2.0V end set vref to 3.295
//#define VREF 3.295
// adc in this esp8266 looks to be not perfectly linear  

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
//char auth[] = "WXJ4a-IN4I3jDSfhez7nsn8JzTupn_NL";
// new version hydroponic_control blynk token
char auth[] = "MuYphIVACLxc67wbfY1ZQoQ36BcpNGAd";

// Your WiFi credentials for Blynk.
// Set password to "" for open networks.
char ssid[] = "FreeNet";
char pass[] = "aaaabbbb";

// your WiFi credentials for OTA, I had them set to the same network as blynk but maybe for security you want a different one for OTA.
#ifndef STASSID
#define STASSID "FreeNet"
#define STAPSK  "aaaabbbb"
#endif

const char* ssidota = STASSID;
const char* password = STAPSK;
BlynkTimer timer;
WidgetLED led1(V8);
WidgetLED led2(V9);
WidgetLED led3(V10);
WidgetLED led4(V11);

int ledstate = 1;
//int pulse_time = 0;
int mL_dose = 0;
int min_tds = 0;
int auto_mode = 0;
int max_dose = 0;
int dose_count = 0;

void myTimerEvent(){
  float volt = 0;
  float tds = 0;
  ledstate = !ledstate;
  //digitalWrite(D1, ledstate); // togle the LED hooked to D1 on and off
  digitalWrite(LED_BUILTIN, ledstate);
  //digitalWrite(D9, ledstate);
  //Blynk.virtualWrite(V5, ledstate);
  volt = get_voltage(TdsSensorPin);
  Blynk.virtualWrite(V0, volt);       
  tds = volt_to_tds(volt);
  Blynk.virtualWrite(V1, tds);
 
  read_liquid_level();
  auto_feedback(tds);
  
  // led 1 - 3 active low led turns on
  //if (digitalRead(D5) == 1){
  //  led1.off();
  //}else{
  //  led1.on();
  //}
  led_set(1,digitalRead(D5));
  led_set(2, digitalRead(D6)); 
  led_set(3, digitalRead(D7));
  
  if (ledstate == 0){
    led4.off();
  }else{
    led4.on();
  }
}

void auto_feedback( float tds){   
   
   if (auto_mode == 1){
     // auto tds feedback active
     if (dose_count >= max_dose){
       Serial.println("max dose count reached, nothing done");
       return;
     } 
     Serial.println("auto feedback mode active");
     if (tds < min_tds){
       Serial.println("min_tds < tds pulse pump to correct");
       dose_count++;
       Serial.println(dose_count);
       Blynk.virtualWrite(V16, dose_count);
       pulse_pump();
     }   
   }
}

void pulse_pump(){  
   Serial.println("mL_dose");
   Serial.println(mL_dose);
   if (mL_dose == 0){
     delay(200);
     Blynk.virtualWrite(V5,0);  
     return;
   }   
   if (pumpA_ms_per_mL<pumpB_ms_per_mL){
     turn_on_dosage_pump_A();
     turn_on_dosage_pump_B();
     Serial.println("turn on pumps A and B for ms");
     Serial.println(pumpA_ms_per_mL*mL_dose);
     delay(pumpA_ms_per_mL*mL_dose);
     turn_off_dosage_pump_A();
     Serial.println("pump A turned off continue for ms");
     Serial.println((pumpB_ms_per_mL*mL_dose)-(pumpA_ms_per_mL*mL_dose));
     delay((pumpB_ms_per_mL*mL_dose)-(pumpA_ms_per_mL*mL_dose));
     turn_off_dosage_pump_B();
     Serial.println("turned off pump B");
   }else{
     turn_on_dosage_pump_A();
     turn_on_dosage_pump_B();
     Serial.println("turn on pumps A B for ms");
     Serial.println(pumpB_ms_per_mL*mL_dose);
     delay(pumpB_ms_per_mL*mL_dose);
     turn_off_dosage_pump_B();
     Serial.println("pump B turned off continue ms");
     Serial.println((pumpA_ms_per_mL*mL_dose)-(pumpB_ms_per_mL*mL_dose));
     delay((pumpA_ms_per_mL*mL_dose)-(pumpB_ms_per_mL*mL_dose));
     turn_off_dosage_pump_A();
     Serial.println("pump A turned off");
   }
}

BLYNK_WRITE(V5){
  // V5 is manual activate dose botton to pulse_pump()
  int value = param.asInt();
  pulse_pump();
  // after pulse_pump() time completes turn off V5 button
  Blynk.virtualWrite(V5,0);  
}

BLYNK_WRITE(V6){
  // v6 returns mili Liter dosage entry to apply when pulse_pump is run
  // pumps puts out about 1.33 mili Liter per second
  // so pulse_time is about 750 mili seconds per mili Liter
  // pulse_time in mili seconds
  // we found that the pumps are not an exact rates.  We need to calibrate each one separately
  // so now this just sets mL_dose instead, calibration values are added at pulse_pump()
  mL_dose = param.asInt();
}

BLYNK_WRITE(V13){
 //V13 is auto_mode activate botton 
  auto_mode = param.asInt();
  Serial.println("auto mode entry");
  Serial.println(auto_mode);
}

BLYNK_WRITE(V14){
 //V14 is min_tds value entry from app
  min_tds = param.asInt();
  Serial.println("min_tds entry");
  Serial.println(min_tds);
}

BLYNK_WRITE(V15){
 //V15 is max_dose value entry from app
  max_dose = param.asInt();
  Serial.println("max_dose_entry");
  Serial.println(max_dose);
}

BLYNK_WRITE(V17){
 //V17 is pump A calibration factor ms/ml
  pumpA_ms_per_mL = param.asInt();
  Serial.println("pump A calibration override");
  Serial.println(pumpA_ms_per_mL);
}

BLYNK_WRITE(V18){
 //V18 is pump B calibration factor ms/ml
  pumpB_ms_per_mL = param.asInt();
  Serial.println("pump B calibration override");
  Serial.println(pumpB_ms_per_mL);
}

void led_set(int led, int onoff){
  //Serial.println("onoff");
  //Serial.println(onoff);
  //Serial.println("led");
  //Serial.println(led);
  // led on active 0
  onoff = !onoff;
  switch( led){
    case 1:
      if (onoff == 0){
        led1.on();
      }else{
        led1.off();
      }
      break;
    case 2:
      if (onoff == 0){
        led2.on();
      }else{
        led2.off();
      }
      break;
    case 3:
      if (onoff == 0){
        led3.on();
      }else{
        led3.off();
      }
      break;
   } 
}

void read_liquid_level(){
  // note the liquid level switches will hook D5,D6,D7 to GND when float levels lift
  // d5 is lowest level in bucket, d6 is midle level, d7 is high level when water bucket full
  //int level = 3 - (digitalRead(D5) + digitalRead(D6) + digitalRead(D7));
  // seems in my case the level switches are reversed so taking out the 3 - here
  // I also only ended up only using 2 of the level switches in the shallow reservoir pool (12 cm) so later I will remove D7
  int level = (digitalRead(D5) + digitalRead(D6) + digitalRead(D7));
  Blynk.virtualWrite(V2, level);
}

void turn_on_dosage_pump_A(){
  // turn on both dose pumps at the same time to apply same amount of both A and B hydro solutions at same time
   Serial.println("turn on pump A");
   // relay module is active on low
   digitalWrite(pumpA, 0);
   //digitalWrite(pumpB, 0);
}

void turn_on_dosage_pump_B(){
  // turn on both dose pumps at the same time to apply same amount of both A and B hydro solutions at same time
   Serial.println("turn on pump B");
   // relay module is active on low
   //digitalWrite(pumpA, 0);
   digitalWrite(pumpB, 0);
}

void turn_off_dosage_pump_A(){
  // turn off both dose pumps
   Serial.println("turn off pump A");
   digitalWrite(pumpA, 1);
   //digitalWrite(pumpB, 1);
}

void turn_off_dosage_pump_B(){
  // turn off both dose pumps
   Serial.println("turn off pump B");
   //digitalWrite(pumpA, 1);
   digitalWrite(pumpB, 1);
}

float volt_to_tds(float voltage){
  float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compensationVolatge=voltage/compensationCoefficient;  //temperature compensation
  float tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
  return tdsValue; 
}



float get_average_adc(int pin){
   int measurings=0;
   int samples = 30;
    for (int i = 0; i < samples; i++)
    {
        measurings += analogRead(pin);
        delay(10);
    }
    float av =  measurings/samples;  
    return av;   
}

float adc_to_volt( float av){
   float voltage = VREF / 1024.0 * av;  
   return voltage; 
}

float get_voltage(int pin){
  float av = get_average_adc(pin);
  return adc_to_volt(av);
}

float get_voltage2(int pin){
   int measurings=0;
   int samples = 30;
    for (int i = 0; i < samples; i++)
    {
        measurings += analogRead(pin);
        delay(10);
    }
    float voltage = VREF / 1024.0 * measurings/samples;  
    return voltage;   
}


void setup()
{
  // Debug console
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); // pin2 also = D4 aka LED_BUILTIN  that is built in led on esp8266
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);

  //
  
  //pinMode(GPIO1, OUTPUT);
  // D8 boot will fail if pulled high, is pulled to gnd
  //pinMode(D8, OUTPUT); // this can't drive an led without some problems
  // These digital inputs are used to sense the liquid level switches to monitor reservoir fluid levels
  pinMode(D7, INPUT_PULLUP);
  pinMode(D6, INPUT_PULLUP);
  pinMode(D5, INPUT_PULLUP);
  pinMode(A0, INPUT);

// these digital outputs are setup to drive the relay board
// at power up we defalt them all to be turned off, HIGH is off on the relay board
// this is good if we have a brown out and system resets it will auto shut off pump and cam and has to be manualy corrected on the app to turn them back on
  digitalWrite(D0, HIGH); // 12v pump A 
  digitalWrite(D1, HIGH); // 12v pump B
  digitalWrite(D2, HIGH); // ac power control to monitor cam and air pump to shut down at night to save power due to solar power limitations
  digitalWrite(D3, HIGH); // presently not used but could use to separate air pump and cam monitor at some point or other

  
  //turn_off_dosage_pumps();
  
 // Blynk.begin(auth, ssid, pass);
  Blynk.begin(auth, ssid, pass, "www.funtracker.site", 8080);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidota, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    //ESP.restart();
  }
  Blynk.virtualWrite(V6,0);
  Blynk.virtualWrite(V14,0);
  Blynk.virtualWrite(V13,0); 
  Blynk.virtualWrite(V15,0);  
  Blynk.virtualWrite(V16,0);
  Blynk.virtualWrite(V17,pumpA_ms_per_mL);
  Blynk.virtualWrite(V18,pumpB_ms_per_mL);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //Serial.println("updated with ota");
  // You can also specify server:
  //Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  //Blynk.begin(auth, ssid, pass, IPAddress(192,168,1,100), 8080);

   timer.setInterval(5000L, myTimerEvent);
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  ArduinoOTA.handle();
}
