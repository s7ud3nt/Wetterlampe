#include <FastLED.h>
#include <ArduinoJson.h>                  
#include <math.h>                         
#include <WiFiManager.h>
#include <time.h> 
#include <SPI.h> 
#include <BlynkSimpleEsp8266.h>

#define BLYNK_PRINT Serial
#define BRIGHTNESS 200
#define DATA_PIN D3 
#define NUM_LEDS 8   

CRGB leds[NUM_LEDS];

WiFiManager wifiManager;
WiFiClient client;

// ========================  hier deinen API-Key eintragen!!!  ==============================================================================================
const String city = "Oldenburg";
const String api_key = "XXX";    // dein Open-Weather-Map-API-Schluessel, kostenlos beziehbar ueber https://openweathermap.org/
// ==========================================================================================================================================================

//blynk auth code an wifi credentials
char auth[] = "XXX";
char ssid[] = "XXX";
char pass[] = "XXX";

//weather data
int weatherID = 0;
int weatherID_shortened = 0;
String weatherforecast_shortened = " ";
int temperature_Celsius_Int = 0;
int cloudiness = 0;
int windforce = 0;
int sun_color[3] = {50, 0, 255}; 
int cloud_color[3] = {0, 0, 0};
int fog_color[3] = {0, 0, 0};
int visibility = 0;
// hour from last update of current weather conditions
int last_hour = 0;
int sunrise_hour = 0;
int sunset_hour = 0;
unsigned long lastcheck = 0;


//LEDs
int windLED = 0; 
int snowLED = 1;
int thunderLED = 2;
int rainLED = 3;
int cloudLED = 4;
int sunLED = 5;
int fodLED = 6;
int tempLED = 7;

//testing attributes´
int anHeaviness = 0; //animation heaviness: 1 = light, 2 = moderate, 3 = heavy, 4 = very heavy, 5 = extreme
int numAn = 0; //animation number: 1 = clear Sky, 2 = cloudy, 3 = fog, 4 = rai, 5 = snow, 6 = sleet, 7 = thunderstorm
boolean testmode = false;

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void setup() {
  Serial.begin(115200);    
 
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 400);
  FastLED.clear();
  FastLED.show();
  
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB( 253, 184, 19);
  }
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.show();

  // connect to wifi aoutmatically
  wifiManager.autoConnect("deineWetterLampe");
  
  Blynk.begin(auth, ssid, pass);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB( 0, 0, 0);
  }
  FastLED.show();
  
  delay(2000);
  
  getCurrentWeatherConditions();
  
  
}
//.............................................
void loop() {
  Blynk.run();

  //testing
  if (testmode == true) {
    switch(numAn){
      case 1: Serial.println("Test Clear Sky Animation"); testClearSky(); testEnded(); break;
      case 2: Serial.println("Test Cloudy Animation"); testCloudy(); testEnded(); break;
      case 3:  Serial.println("Test Fog Animation"); testFog(); testEnded(); break;
      case 4: Serial.println("Test Rain Animation"); testRain(anHeaviness); testEnded(); break;
      case 5: Serial.println("Test Snow Animation"); testSnow(anHeaviness); testEnded(); break;
      case 6: Serial.println("Test Sleet Animation"); testSleet(anHeaviness); testEnded(); break;
      case 7: Serial.println("Test Thunderstorm Animation"); testThunderstorm(anHeaviness); testEnded(); break;
      default: break;
    }
  }
  
   
  //check every 30 min current weahter
  if (millis() - lastcheck >= 1800000) {         
    getCurrentWeatherConditions();
    lastcheck = millis();                         
  }

  //select animation depending on weather data
  if (weatherID == 800){
    //LED_effect_clearSky();
    clearSky();
  }
  else {
    switch (weatherID_shortened) {
      case 2: thunderstorm();  break;
      case 5: rain(); break;
      case 6: snow(); break;
      case 7: fog(); break;
      case 8:  cloudy(); break; 
    }
  }

}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void getCurrentWeatherConditions() {
  int WeatherData;
  Serial.print("connecting to "); Serial.println("api.openweathermap.org");
  if (client.connect("api.openweathermap.org", 80)) { //connect client to openwethermap
    //get the current weather from openwethermap for the epecified city
    client.println("GET /data/2.5/weather?q=" + city + ",DE&units=metric&lang=de&APPID=" + api_key);
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
  
  //capacity for the memory in bytes that beeing set by DynamicJsonDocument doc()                                             
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 360;
  //allocates memory in the heap
  DynamicJsonDocument doc(capacity);
  //(desserialize := converts byte stream to object), saves the json got from client above as object in doc
  deserializeJson(doc, client);
  //stop the client
  client.stop();

  //get weather id from saved json
  weatherID = doc["weather"][0]["id"];
  
  //get temperature from saved json
  int temperature_Celsius = doc["main"]["temp"];
  temperature_Celsius_Int = (int)temperature_Celsius;

  //get cloudiness
  cloudiness = doc["clouds"]["all"];

  //get windspeed and set windforce
  float windspeed = doc["wind"]["speed"];
  if (windspeed >= 0.0 && windspeed <= 5.4) {windforce = 0;}
  else if(windspeed >= 5.5 && windspeed <= 10.7) {windforce = 1;}
  else if(windspeed >= 10.8 && windspeed <= 17.1) {windforce = 2;}
  else if(windspeed >= 17.2 && windspeed <= 24.4) {windforce = 3;}
  else if(windspeed >= 24.5 && windspeed <= 32.6) {windforce = 4;}
  else if(windspeed >= 32.7) {windforce = 5;}

  //get visibility
  visibility = doc["visibility"];

  //get current hour from doc+
  int utc_time = doc["dt"];
  int  timezone_owm = doc["timezone"];
  int local_time = utc_time + timezone_owm;
  
  int utc_sunrise = doc["sys"]["sunrise"];
  int utc_sunrise_hour = utc_sunrise + timezone_owm;
  
  int utc_sunset = doc["sys"]["sunset"];
  int utc_sunset_hour = utc_sunset + timezone_owm;
  
  time_t t1 = local_time;
  tm* tim1 = gmtime(&t1);
  last_hour = tim1 -> tm_hour;

  time_t t2 = utc_sunrise_hour;
  tm* tim2 = gmtime(&t2);
  sunrise_hour = tim2 -> tm_hour;
  
  time_t t3 = utc_sunset_hour;
  tm* tim3 = gmtime(&t3);
  sunset_hour = tim3 -> tm_hour;
 
  Serial.print("hour: ");
  Serial.print(last_hour);
  Serial.print(" sunrise: ");
  Serial.print(sunrise_hour);
  Serial.print(" sunset: ");
  Serial.print(sunset_hour);
  Serial.println();
  
  //schorts the weather ID to the fisrt number: 8XX / 100 = 8
  weatherID_shortened = weatherID / 100;
  //set wetherforecast depending on weather_id_shortened
  switch (weatherID_shortened) {                                    
    //...
    case 8: weatherforecast_shortened = "Wolken"; break;
    default: weatherforecast_shortened = ""; break;
  } if (weatherID == 800) weatherforecast_shortened = "klar";            
}
//.............................................
void fade(int led_position, uint16_t duration, uint16_t delay_val, uint16_t startR, uint16_t startG, uint16_t startB, uint16_t endR, uint16_t endG, uint16_t endB) {
  int16_t redDiff = endR - startR;
  int16_t greenDiff = endG - startG;
  int16_t blueDiff = endB - startB;
  int16_t steps = duration * 1000 / delay_val;
  int16_t redValue, greenValue, blueValue;
  //fades first led from stripe depending on given parameters above
  for (int16_t i = 0 ; i < steps - 1 ; ++i) {
    redValue = (int16_t)startR + (redDiff * i / steps);
    greenValue = (int16_t)startG + (greenDiff * i / steps);
    blueValue = (int16_t)startB + (blueDiff * i / steps);
    leds[led_position] = CRGB(redValue, greenValue, blueValue);
    FastLED.show();
    delay(delay_val);
  }
  leds[led_position] = CRGB(endR, endG, endB);
}
//.............................................
void LED_effect_clearSky() { // Effekt, der angezeigt wird, wenn der Himmel klar ist
  //set brigthness to max., set first LED yellow then black (off)
  FastLED.setBrightness(255);
  leds[0] = CRGB::Yellow;
  FastLED.show();
  delay(500);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(500);
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//animations

void rain() { //animations for rain, depending on weatherID
  setLEDs();
 
  int heaviness = 0;
  boolean freezing = false;
//.............................................
  switch(weatherID) {
    case 501: heaviness = 2; freezing = false; break;
    case 502: heaviness = 3; freezing = false; break;
    case 503: heaviness = 4; freezing = false; break;
    case 504: heaviness = 5; freezing = false; break;
    case 511: heaviness = 3; freezing = true; break;
    case 520: heaviness = 2; freezing = false; break;
    case 521: heaviness = 3; freezing = false; break;
    case 522: heaviness = 4; freezing = false; break;
    case 531: heaviness = 5; freezing = false; break;
    default: break;
  }
  
  if(freezing == true) {
    freezingRain(heaviness);
  } else {
    rain(heaviness);
  }
}
//.............................................
void rain(int heaviness){
  for (int j = 255; j >= 0; j--) { // 255 * (10-heaviness*2) ms
    setLED(rainLED, 0, 0, j);
    delay(10-(heaviness*2));
  }
}
//.............................................
void freezingRain(int heaviness){
  for (int j = 255; j >= 0; j--) { // 255 * (10-heaviness*2) ms
    setLED(rainLED, 0, j, j);
    delay(10-(heaviness*2));
  }
}
//.............................................
void rain(int heaviness, int drops){
  for (int i = 0; i < drops; i++){
    rain(heaviness);
  }
}
//.............................................
void snow() { //animations for snow and sleet, depending on windspeed and weatherID
  setLEDs();

  int sleetHeaviness = 0;

  int heaviness = 0;

  switch(weatherID) {
    case 601: heaviness = 1; break;
    case 602: heaviness = 2; break;
    case 611: sleetHeaviness = 3; break;
    case 612: sleetHeaviness = 4; heaviness = 1; break;
    case 613: sleetHeaviness = 5; heaviness = 2;break;
    case 615: sleetHeaviness = 4; heaviness = 1;break;
    case 616: sleetHeaviness = 5; heaviness = 2;break;
    case 621: heaviness = 1; break;
    case 622: heaviness = 2; break;
    default: break;
  }

  for (int i = 255; i >= 0; i--) {
    setLED(snowLED, i, i, i);
    delay((20-(heaviness*5)) - windforce);
  }

  if (sleetHeaviness > 0) {
    for (int i = 255; i >= 0; i--) {
      setLED(snowLED, 0, 0, i);
      delay(10-(sleetHeaviness*2));
    }
  }
  
}
//.............................................
void thunderstorm() { //animations for thunderstorm, depending on weatherID
  setLEDs();
  setLED(thunderLED, 250, 255, 0);
  int heaviness = 0;
  
  switch(weatherID) {
    case 200: heaviness = 2; break;
    case 201: heaviness = 3; break;
    case 202: heaviness = 4; break;
    case 210: heaviness = 2; break;
    case 211: heaviness = 3; break;
    case 212: heaviness = 4; break;
    case 221: heaviness = 4; break;
    case 230: heaviness = 2; break;
    case 231: heaviness = 3; break;
    case 232: heaviness = 4; break;
    default: break;
  }
  
  int time_spent = 0;
  //lightnings and rain
  while (time_spent <= 15000) { //15 sec animation
    setLED(thunderLED, 250, 255, 0);
    int illumination_time = random(1000, 3000);
    time_spent += illumination_time;

    int num_raindrops = illumination_time / (255 * (10-heaviness*2));
    
    rain(heaviness, num_raindrops);
    
    setLED(thunderLED, 255, 255, 255);
    delay(25);
    time_spent += 20;
    setLED(thunderLED, 250, 255, 0);
    
    illumination_time = random(1000, 3000);
    time_spent += illumination_time;
    
    num_raindrops = illumination_time / (255 * (10-heaviness*2));

    rain(heaviness, num_raindrops);
    
    int led = random(cloudLED, sunLED);
    int ledrgb[3] = {0, 0, 0};
    switch(led) {
      case 4: ledrgb[0] = cloud_color[0]; ledrgb[1] = cloud_color[1]; ledrgb[2] = cloud_color[2]; break;
      case 5: ledrgb[0] = sun_color[0]; ledrgb[1] = sun_color[1]; ledrgb[2] = sun_color[2]; break;
      default: break;
    }
    setLED(led, 250, 255, 0);
    delay(25);
    time_spent += 20;
    setLED(led, ledrgb[0], ledrgb[1], ledrgb[2]);
  }
  
}
//.............................................
void sunColorFade(int color_shift) { //color fade of sun for cloudy and foggy weather
  for (int i = 0; i < color_shift; i++) {
    if(last_hour == sunrise_hour || last_hour == sunset_hour) {
      setLED(sunLED, sun_color[0], sun_color[1] + i, sun_color[2] + i);
    }
    else if(last_hour > sunrise_hour && last_hour < sunset_hour) {
      if (sun_color[1] + i < 255) {setLED(sunLED, sun_color[0], sun_color[1] + i, sun_color[2] + i);}
      else {setLED(sunLED, sun_color[0], 255, sun_color[2] + i);}
    }
    else {
      setLED(sunLED, sun_color[0] + i, sun_color[1] + i, sun_color[2]);
    }
    delay(40-(windforce*7));
  }

  for (int i = color_shift; i > 0; i--) {
    if(last_hour == sunrise_hour || last_hour == sunset_hour) {
      setLED(sunLED, sun_color[0], sun_color[1] + i, sun_color[2] + i);
    }
    else if(last_hour > sunrise_hour && last_hour < sunset_hour) {
      if (sun_color[1] + i <= 255) {setLED(sunLED, sun_color[0], sun_color[1] + i, sun_color[2] + i);}
      else {setLED(sunLED, sun_color[0], 255, sun_color[2] + i);}
    }
    else {
      setLED(sunLED, sun_color[0] + i, sun_color[1] + i, sun_color[2]);
    }
    delay(40-(windforce*7));
  }
}
//.............................................
void cloudy(){ //animations for cloudiness, depending on windspeed 
  setLEDs();
  
  //sun -rise/-set color = 255, 60, 0; 
  //day color = 255, 190, 30;
  //nigth color = 50, 0, 255;
  int color_shift = 0;
  
  switch(cloudiness) {
    case 51 ... 84: color_shift = 50; break;
    case 85 ... 100: color_shift = 100; break;
    default: break;
  }

  sunColorFade(color_shift);
  
}
//.............................................
void fog(){ //animations for foginess, depending on windspeed 
  setLEDs();

  //sun -rise/-set color = 255, 60, 0;  60 -> 255, 0->255
  //day color = 255, 190, 30;  
  //nigth color = 50, 0, 255; 

  int color_shift = 0;
  switch(visibility) {
    case 0 ... 199: color_shift = 100; break;
    case 200 ... 499: color_shift = 50; break;
    default: break;
  }
  
  sunColorFade(color_shift);
}
//.............................................
void clearSky() { //animation for clear sky
  setLEDs();  
}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

//setter

void setLED(int led,int r, int g, int b){
  setLED(led, BRIGHTNESS, r, g, b);
}
//.............................................
void setLED(int led, int brigthness, int r, int g, int b){
  leds[led] = CRGB(r, g, b);
  FastLED.setBrightness(brigthness);
  FastLED.show();
} 
//.............................................
void setLEDs(){ //sets LEDS for Fog, Temp, Cloud, Wind and Sun depending on weather conditions
  setFogLED();
  setTempLED();
  setCloudLED();
  setWindSpeedLED();
  setSunColor();
  
  setLED(sunLED, sun_color[0], sun_color[1], sun_color[2]);
}
//.............................................
void setSunColor(){ // color depends on time (hour)
  //sun -rise/-set color = 255, 60, 0; 
  //day color = 255, 190, 30;
  //nigth color = 50, 0, 255;
  if(last_hour == sunrise_hour || last_hour == sunset_hour) {
    sun_color[0] = 255; sun_color[1] = 60; sun_color[2] = 0;
    
  }
  else if(last_hour > sunrise_hour && last_hour < sunset_hour) {
    sun_color[0] = 255; sun_color[1] = 190; sun_color[2] = 30;
  }
  else {
    sun_color[0] = 50; sun_color[1] = 0; sun_color[2] = 255;
  }
}
//.............................................
void setFogLED() { // Temp LED is LED 6
  
  switch(visibility){
    case 0 ... 199: setLED(fodLED, 255, 255, 255); setFogColor(255, 255, 255); break;
    case 200 ... 499: setLED(fodLED, 100, 100, 200); setFogColor(100, 100, 200); break;
    case 500 ... 1000: setLED(fodLED, 50, 50, 150); setFogColor(50, 50, 150); break;
    default: setLED(fodLED, 0, 191, 255); setFogColor(0, 191, 255); break;
  }
}
//.............................................
void setFogColor(int r, int g, int b) {
  fog_color[0] = r;
  fog_color[1] = g;
  fog_color[2] = b;
}
//.............................................
void setCloudLED(){ // Temp LED is LED 4
  
  switch(cloudiness) {
    case 25 ... 50: setLED(cloudLED, 60, 255, 255); setCloudColor(60, 255, 255); break;
    case 51 ... 84: setLED(cloudLED, 125, 255, 255); setCloudColor(125, 255, 255); break;
    case 85 ... 100: setLED(cloudLED, 255, 255, 255); setCloudColor(255, 255, 255); break;
    default: setLED(cloudLED, 0, 191, 255); setCloudColor(0, 191, 255); break;
  }
}
//.............................................
void setCloudColor(int r, int g, int b) {
  cloud_color[0] = r;
  cloud_color[1] = g;
  cloud_color[2] = b;
}
//.............................................
void setWindSpeedLED() { // Temp LED is LED 0
  
  switch(windforce) {
    case 0: setLED(windLED, 0, 191, 255); break;
    case 1: setLED(windLED, 0, 255, 0); break;
    case 2: setLED(windLED, 205, 205, 0); break;
    case 3: setLED(windLED, 255, 215, 0); break;
    case 4: setLED(windLED, 238, 118, 50); break;
    case 5: setLED(windLED, 255, 0, 0); break;
    default: break;
  } 
}
//.............................................
void setTempLED() { // Temp LED is LED 7
  switch(temperature_Celsius_Int){
      case -50 ... -11: setLED(tempLED, 200, 200, 255); break;
      case -10 ... 0: setLED(tempLED, 0, 191, 255); break;
      case 1 ... 10: setLED(tempLED, 15, 255, 0); break;
      case 11 ... 19: setLED(tempLED, 255, 215, 0); break;
      case 20 ... 30: setLED(tempLED, 255, 45, 0); break;
      case 31 ... 50: setLED(tempLED, 255, 0, 0); break;
      default: break;
  }

  if (temperature_Celsius_Int < -10){setLED(0, 230, 245, 255);}
  else if(temperature_Celsius_Int > 33){setLED(0, 255, 0, 200);} 

}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

BLYNK_WRITE(V0){ //animation selector
  int an = param.asInt(); //selected animation
  numAn = an;
  if(an <= 3) { //1 = clear Sky, 2 = cloudy, 3 = fog 
    Blynk.setProperty(V2, "offLabel", "push to test");
    Blynk.setProperty(V2, "onLabel", "test");
  } else {
    Blynk.setProperty(V2, "onLabel", "select heaviness");
    Blynk.setProperty(V2, "offLabel", "select heaviness");
  }
}

BLYNK_WRITE(V1){ //heaviness selector
  int heaviness = param.asInt(); 
  switch(numAn) {
    case 1 ... 3: Blynk.setProperty(V2, "onLabel", "no heaviness needed"); 
                  Blynk.setProperty(V2, "offLabel", "no heaviness needed"); delay(2000);
                  Blynk.run();
                  Blynk.setProperty(V2, "onLabel", "push to test");
                  Blynk.setProperty(V2, "offLabel", "test");
                  break;
    case 4: anHeaviness = heaviness; 
            Blynk.setProperty(V2, "offLabel", "push to test");
            Blynk.setProperty(V2, "onLabel", "test");
            break;
    case 5 ... 7: if(heaviness > 3){
                    Blynk.setProperty(V2, "offLabel", "select lower heaviness"); delay(2000);
                    Blynk.run();
                    Blynk.setProperty(V2, "offLabel", "select heaviness");
                  } else {
                    anHeaviness = heaviness;
                    Blynk.setProperty(V2, "offLabel", "push to test");
                    Blynk.setProperty(V2, "onLabel", "test");
                  }
                  break;
    default: Blynk.setProperty(V2, "onLabel", "select animation");
             Blynk.setProperty(V2, "offLabel", "select animation");
             break;
  }
}

BLYNK_WRITE(V2){ //test button
  if(numAn <= 3) { //1 = clear Sky, 2 = cloudy, 3 = fog 
    testmode = true;  
  } else {
    if(anHeaviness <= 0) {
      Blynk.setProperty(V2, "onLabel", "select heaviness");
      Blynk.setProperty(V2, "offLabel", "select heaviness");
      delay(2000);
      Blynk.run();
    } else {
      testmode = true; 
    }
  }
}

BLYNK_WRITE(V3){ //update button
  if(param.asInt() == 1) {
    getCurrentWeatherConditions(); 
  }
}


void testEnded() {
  Blynk.run();
  Serial.println("test end");
  testmode = false;
  Blynk.setProperty(V2, "onLabel", "select animation");
  Blynk.setProperty(V2, "offLabel", "select animation");
}

//test Methods for testing animations

void testClearSky() {
  int temp_now = temperature_Celsius_Int;
  int cloud_now = cloudiness;
  int wind_now = windforce;
  int vis_now = visibility;
  int Lhour_now = last_hour;
  int srise_now = sunrise_hour;
  int sset_now = sunset_hour;
 
  int cloudR_now = cloud_color[0];
  int cloudG_now = cloud_color[1];
  int cloudB_now = cloud_color[2];
  
  int fogR_now = fog_color[0];
  int fogG_now = fog_color[1];
  int fogB_now = fog_color[2];

  temperature_Celsius_Int = 20;
  cloudiness = 25;
  windforce = 3;
  visibility = 5000;
  last_hour = 12;
  sunrise_hour = 6;
  sunset_hour = 21;
  setFogColor(0, 255, 0);
  setCloudColor(0, 255, 150);
  allout();
  
  clearSky();

  delay(5000);
  allout();
  temperature_Celsius_Int = temp_now;
  cloudiness = cloud_now;
  windforce = wind_now;
  visibility = vis_now;
  last_hour = Lhour_now;
  sunrise_hour = srise_now;
  sunset_hour = sset_now;
  setFogColor(fogR_now, fogG_now, fogB_now);
  setCloudColor(cloudR_now, cloudG_now, cloudB_now);
  setLEDs();
}
//.............................................
void testCloudy() {
  int cloud_now = cloudiness;

  int wind_now = windforce;
  
  allout();
  delay(1000);
  
  cloudiness = 51;
  for(int i = 0; i < 5; i++) {
    windforce = i;
    cloudy();
    delay(3000);
  }

  allout();
  delay(5000);

  cloudiness = 85;
  for(int i = 0; i < 5; i++) {
    windforce = i;
    cloudy();
    delay(3000);
  }

  allout();
  cloudiness = cloud_now;
  windforce = wind_now;
}
//.............................................
void testFog() {
  int vis_now = visibility;
  int wind_now = windforce;
  
  allout();
  delay(1000);
  
  visibility = 199;
  for(int i = 0; i < 5; i++) {
    windforce = i;
    fog();
    delay(3000);
  }

  allout();
  delay(5000);
  
  visibility = 499;
  for(int i = 0; i < 5; i++) {
    windforce = i;
    fog();
    delay(3000);
  }

  allout();
  windforce = wind_now;
  visibility = vis_now;
}
//.............................................
void testRain(int heaviness) {
  int wID_now = weatherID;

  if (heaviness <= 0 || heaviness > 6) {
    Serial.println("incorrect heaviness selected");
    return;
  }

  if(heaviness == 6) {
    weatherID = 511;
  } else {
    weatherID = 500 + (heaviness - 1);
  }

  allout();
  delay(1000);
  
  rain();
  rain();
  rain();

  allout();
  delay(1000);
  weatherID = wID_now;
}
//.............................................
void testSnow(int heaviness) {
  int wind_now = windforce;
  int wID_now = weatherID;
  
  if (heaviness <= 0 || heaviness > 3) {
    Serial.println("incorrect heaviness selected");
    return;
  }

  switch(numAn) {
    case 5: weatherID = 600 + (heaviness - 1); break;
    case 6: weatherID = 610 + heaviness; break;
    default: break;
  }
  
  allout();
  
  delay(1000);

  for (int i = 0; i < 5; i++) {
    windforce = i;
    snow();
    delay(100);
  }
  
  allout();
  
  windforce = wind_now;
  weatherID = wID_now;
}
//.............................................
void testSleet(int heaviness) {
  testSnow(heaviness);
}
//.............................................
void testThunderstorm(int heaviness) {
  int wID_now = weatherID;

  if (heaviness <= 0 || heaviness > 3) {
    Serial.println("incorrect heaviness selected");
    return;
  }
  
  allout();
  delay(1000);
  
  weatherID = 200 + (heaviness-1);
  thunderstorm();
  allout();
  delay(5000);
  
  weatherID = wID_now;
}

//.............................................
void allout() { //turn all LEDS off
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}
//.............................................
void printValue(String val_name, int val){
  Serial.print(val_name);
  Serial.print(": ");
  Serial.print(val);
  Serial.println();
}
//.............................................
void printValue(String val_name, float val){
  Serial.print(val_name);
  Serial.print(": ");
  Serial.print(val);
  Serial.println();
}
