#include <TimeLib.h>      //TimeLib library is needed https://github.com/PaulStoffregen/Time
#include <NtpClientLib.h> //Include NtpClient library header
#include "SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"` https://github.com/ThingPulse/esp8266-oled-ssd1306
//#include <WiFi.h>
#include <string>
#include <HTTPClient.h>
#include <ArduinoJson.h>
/* modified font created at http://oleddisplay.squix.ch/ */
#include "DialogInput_Bold_12.h"
#include "DialogInput_Bold_24.h"
#include "Moon_Phases.h"
/* modified bitmap image */
#include "PM_image.h"

using namespace std;

SSD1306Wire display(0x3c, 5, 4);

#ifndef WIFI_CONFIG_H

// WiFi creds
#define YOUR_WIFI_SSID "YOUR_WIFI_SSID"
#define YOUR_WIFI_PASSWD "YOUR_WIFI_PASSWD"
#endif // !WIFI_CONFIG_H

#define ONBOARDLED 5 // Built in LED on ESP-12/ESP-07

// Timezone setting
int8_t timeZone = -5; // UTC -5:00 / -4:00 (-5 for daylight saving time)
int8_t minutesTimeZone = 0;

// Weather data variables
int temperature;
int feelsLike;
int tempHigh;
int tempLow;

// Variable for date calculation to count from
int MONTH = 1;
int DAY = 1;
int YEAR = 2020;

// Weather API variables
const String endpoint = "https://api.darksky.net/forecast/";
const String key = "DARKSKY_APY_KEY";
const String latLong = "/LAT,LONG";
const String exclusions = "?exclude=,minutely,hourly,flags&units=us";
//https://api.darksky.net/forecast/


//========================================================================================================= WEATHER DATA
void weatherData()
{
  if ((WiFi.status() == WL_CONNECTED)) //Check the current connection status
  {

    HTTPClient http;

    http.begin(endpoint + key + latLong + exclusions); //Specify the URL
    int httpCode = http.GET();                         //Make the request

    if (httpCode > 0) //Check for the returning code
    {

      String payload = http.getString();
      //Serial.println(httpCode); //200 means ok
      //Serial.println(payload); // Uncomment if you want to see JSON data

      const char *json = payload.c_str(); // convert "payload" string to char

      StaticJsonDocument<2600> doc; //https://arduinojson.org/v6/assistant/  if data is 0.0 than increse it
      deserializeJson(doc, json);
      JsonObject root = doc.as<JsonObject>();

      //Serial.println(json);
      //if(root.success()){
      //Serial.println("json ok");
      //}

      JsonObject main = root["currently"];
      JsonObject dailyData = root["daily"]["data"][0];
      String b = root["daily"]["data"][0]["temperatureHigh"];
      Serial.println("Temp High: " + b);

      temperature = round(main["temperature"]);
      feelsLike = round(main["apparentTemperature"]);
      tempHigh = round(dailyData["temperatureHigh"]);
      tempLow = round(dailyData["temperatureLow"]);

      Serial.println("Temprature: " + String(temperature));
    }
    else
    {
      Serial.println("Error on HTTP request");

      WiFi.begin(YOUR_WIFI_SSID, YOUR_WIFI_PASSWD); // Try connecting to wifi again...

      // Default values...
      temperature = 0;
      feelsLike = 0;
      tempHigh = 0;
      tempLow = 0;
    }

    http.end(); //Free the resources
  }
}

//========================================================================================================= MOON PHASE CALCULATION
int JulianDate(int d, int m, int y)
{
  int mm, yy;
  int k1, k2, k3;
  int j;

  yy = y - (int)((12 - m) / 10);
  mm = m + 9;
  if (mm >= 12)
  {
    mm = mm - 12;
  }
  k1 = (int)(365.25 * (yy + 4712));
  k2 = (int)(30.6001 * mm + 0.5);
  k3 = (int)((int)((yy / 100) + 49) * 0.75) - 38;
  // 'j' for dates in Julian calendar:
  j = k1 + k2 + d + 59;
  if (j > 2299160)
  {
    // For Gregorian calendar:
    j = j - k3; // 'j' is the Julian date at 12h UT (Universal Time)
  }
  return j;
}

// Calculating moon age using julian date.
double MoonAge(int d, int m, int y)
{
  double ip, ag;

  int j = JulianDate(d, m, y);
  //Calculate the approximate phase of the moon
  ip = (j + 4.867) / 29.53059;
  ip = ip - floor(ip);
  //After several trials I've seen to add the following lines,
  //which gave the result was not bad
  if (ip < 0.5)
    ag = ip * 29.53059 + 29.53059 / 2;
  else
    ag = ip * 29.53059 - 29.53059 / 2;
  // Moon's age in days
  ag = floor(ag) + 1;
  return ag;
}

//========================================================================================================= TIME (and print function)
// This function takes initial date and calculate days.
// Calculate day number from date.
int dayCalculation(int m, int d, int y)
{
  m = (m + 9) % 12;
  y = y - m / 10;

  return 365 * y + y / 4 - y / 100 + y / 400 + (m * 306 + 5) / 10 + (d - 1);
}

void printTime()
{
  display.clear();

  // To keep time updated you need to call now() from time to time inside loop
  // in this case getTimeDateString() implies a call to now()
  // Time and data string broken down into array string under
  String theTime[6] = {NTP.getTimeDateString().substring(0, 2), NTP.getTimeDateString().substring(3, 5), NTP.getTimeDateString().substring(6, 8),
                       NTP.getTimeDateString().substring(9, 11), NTP.getTimeDateString().substring(12, 14), NTP.getTimeDateString().substring(15, 19)};

  // Converting time to 12H time
  if (theTime[0].toInt() >= 12)
  {
    display.drawXbm(110, 0, 17, 13, PM_icon);

    if (theTime[0].toInt() < 22)
    {
      theTime[0] = "0" + String(theTime[0].toInt() - 12);
    }
    else
      theTime[0] = String(theTime[0].toInt() - 12);
  }
  if (theTime[0].toInt() == 0)
    theTime[0] = "12";

  display.setFont(Roboto_Black_24);       // Font and size
  display.drawString(30, 18, theTime[0]); // Time Hour

  display.setFont(ArialMT_Plain_24);      // Font and size
  display.drawString(62, 18, theTime[1]); // Time Minutes

  display.setFont(ArialMT_Plain_10);      // Font and size
  display.drawString(90, 20, theTime[2]); // Time Seconds

  display.setFont(ArialMT_Plain_16);                                          // Font and size
  display.drawString(0, 0, theTime[4] + "-" + theTime[3] + "-" + theTime[5]); // Time Date

  Serial.println(NTP.getTimeDateString());

  // Update weather data every 5 minutes
  if ((theTime[1].toInt() % 5) == 0 && (theTime[2].toInt() == 1))
  {
    weatherData();
  }

  // Weather data (check data is valid before printing)
  if (temperature == 0 && tempHigh == 0 && tempLow == 0)
  {
    weatherData(); // Initialize weather data AGAIN if everything == 0

    display.setFont(ArialMT_Plain_16); // Font and size
    display.drawString(40, 49, String(temperature) + "F");
    //display.drawString(70, 49, String(feelsLike));
    display.setFont(ArialMT_Plain_10); // Font and size
    display.drawString(0, 54, String(tempHigh) + "~" + String(tempLow));
  }
  else
  {
    display.setFont(ArialMT_Plain_16); // Font and size
    display.drawString(40, 49, String(temperature) + "F");
    //display.drawString(70, 49, String(feelsLike));
    display.setFont(ArialMT_Plain_10); // Font and size
    display.drawString(0, 54, String(tempHigh) + "~" + String(tempLow));
  }

  // Calculating days difference from starting date to now.
  int weeksOfWorkInDays = dayCalculation(String(theTime[4]).toInt(), String(theTime[3]).toInt(), String(theTime[5]).toInt()) - dayCalculation(MONTH, DAY, YEAR); // mm/dd/yyyy format
  display.setFont(ArialMT_Plain_16);                                                                                                                             // Font and size
  display.drawString(87, 42, "W: " + String(round((weeksOfWorkInDays / 7.0) + 0.7)).substring(0, 2));

  // Moon phase section
  double ag = MoonAge(theTime[3].toInt(), theTime[4].toInt(), theTime[5].toInt()); // Using this algorythm, found it easier

  display.setFont(Moon_Phases_14); // Font and size

  // Monn phase calculation adjustments
  Serial.print("Moon age: ");
  Serial.println(int(ag));

  String letter; // Variable to hold letter/character representing moonphase
  
  switch (int(ag))
  {
  case 0: letter = "0"; break; // full Moon
  case 1: letter = "a"; break; 
  case 2: letter = "b"; break; // -- duplicate b's
  case 3: letter = "b"; break;
  case 4: letter = "c"; break;
  case 5: letter = "d"; break;
  case 6: letter = "e"; break;
  case 7: letter = "f"; break;
  case 8: letter = "g"; break; // 1st quarter
  
  case 9: letter = "h"; break;
  case 10: letter = "i"; break;
  case 11: letter = "j"; break;
  case 12: letter = "k"; break;
  case 13: letter = "l"; break;
  case 14: letter = "m"; break;
  case 15: letter = "1"; break; // Full moon

  case 16: letter = "n"; break;
  case 17: letter = "o"; break;
  case 18: letter = "p"; break;
  case 19: letter = "q"; break;
  case 20: letter = "r"; break;
  case 21: letter = "s"; break;
  case 22: letter = "t"; break; // 2nd quarter
  
  case 23: letter = "u"; break;
  case 24: letter = "v"; break;
  case 25: letter = "w"; break;
  case 26: letter = "x"; break;
  case 27: letter = "y"; break;
  case 28: letter = "z"; break; 
  case 29: letter = "1"; break; // New Moon (Already fixed)
  case 30: letter = "1"; break; // New Moon
  default: display.drawString(60, 25, String(ag));
  }
  
  display.drawString(0, 25, letter); // Moon phase
  display.display();
  delay(1000);
}

//========================================================================================================= SETUP
void setup()
{
  display.init(); // Initialize screen

  //display.invertDisplay(); // Inverted display mode

  display.setContrast(100, 50, 64);
  // Convenience method to access
  // display.setBrightness(15);

  Serial.begin(115200);
  Serial.println();

  WiFi.disconnect(true); // Disconnect from wifi

  //Start WiFi communication
  //WiFi.mode (WIFI_STA);
  WiFi.begin(YOUR_WIFI_SSID, YOUR_WIFI_PASSWD); // Initial WiFi connection attempt

  // Check that wifi is connected
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    display.drawString(0, 0, "Connecting to WiFi..");
    display.display();
    display.clear();

    display.drawString(0, 0, "Connecting to WiFi....");
    delay(250);
    display.display();
    display.clear();

    display.drawString(0, 0, "Connecting to WiFi......");
    delay(250);
    display.display();
    display.clear();

    Serial.println("Connecting to WiFi..");
  }

  display.drawString(0, 0, "WiFi Connected!");
  display.display();
  delay(2000);
  display.clear();
  display.drawString(0, 0, "Local IP Address: \n" + WiFi.localIP().toString() + "\nConnected to SSID:\n" + WiFi.SSID());
  display.display();
  Serial.println("Connected to the WiFi network");

  //   NTP begin with default parameters:
  //   NTP server: pool.ntp.org
  //   TimeZone: UTC
  //   Daylight saving: off
  NTP.begin("pool.ntp.org", timeZone, true, minutesTimeZone); // Only statement needed to start NTP sync.

  weatherData(); // Initialize weather data

  delay(5000); // Wait 5 seconds
}

//========================================================================================================= LOOP
void loop()
{
  printTime();
}
