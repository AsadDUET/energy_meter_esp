#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <LiquidCrystal_I2C.h>
#include "time.h"

#define WIFI_SSID "*******"
#define WIFI_PASSWORD "****"
// #define WIFI_SSID "Asad@gnps"
// #define WIFI_PASSWORD "15963214"
#define FIREBASE_HOST "****" //Do not include https:// in FIREBASE_HOST
#define FIREBASE_AUTH "****"
#define relay 2
#define v_mul 0.0876
#define c_mul 0.00434
#define c_offset 1800

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 6 * 60 * 60;
const int daylightOffset_sec = 0;
char timeStringBuff[50];

const int ptPin = 34;
const int ctPin = 33;
int disp = 0, readings[1000], ct_read[1000], i = 0, j = 0, maxm, minm, maxm_ct, minm_ct, supply = 0, data_interval = 2000;
float tarrif1, tarrif2, voltage, voltage1, current, load, load1, kwh_m, bill;
unsigned long t = 0, tt, ttt;
String date = "";
LiquidCrystal_I2C lcd(0x27, 16, 2);

FirebaseData firebaseData;

String UID = "/energy_meter/cl2";
void printLocalTime();


void setup()
{
  pinMode(relay, OUTPUT);
  Serial.begin(9600);
  lcd.init();
  // turn on LCD backlight
  lcd.backlight();
  //////////////////////////////////////////////// wifi, firebase
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  lcd.clear();
  lcd.print("Connecting...");
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  lcd.clear();
  lcd.print("Connected");
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  ////////////////////////////////////////////////////////// NTP Time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  ////////////////////////////////////////////Get Time from server
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
  }
  else
  {
    strftime(timeStringBuff, sizeof(timeStringBuff), "%Y%m", &timeinfo);
  }
  delay(1000);

  tt = millis();
  ttt = millis();
}

void loop()
{
  if ((millis() - tt) > data_interval)
  {
    tt = millis();
    i = 0;
    t = micros();
    while ((micros() - t) < 40000)
    {
      readings[i] = analogRead(ptPin);
      ct_read[i] = analogRead(ctPin);
      i++;
      delayMicroseconds(50);
    }

    maxm = readings[0];
    minm = readings[0];
    for (j = 0; j < i; j++)
    {
      if (readings[j + 1] > maxm)
      {
        maxm = readings[j + 1];
      }
      if (readings[j + 1] < minm)
      {
        minm = readings[j + 1];
      }
    }
    maxm_ct = ct_read[0];
    minm_ct = ct_read[0];
    for (j = 0; j < i; j++)
    {
      if (ct_read[j + 1] > maxm_ct)
      {
        maxm_ct = ct_read[j + 1];
      }
      if (ct_read[j + 1] < minm_ct)
      {
        minm_ct = ct_read[j + 1];
      }
    }
    ////////////////////////////////////////////////// average ct readings
    // current=0;
    //    for (j = 0; j < i; j++)
    //   {
    //     current=current+ct_read[j];
    //   }
    //   current=current/i;
    ///////////////////////////////////////////////// graph
    // for (j = 0; j < i; j++)
    // {
    //   Serial.print(maxm);
    //   Serial.print(",");
    //   Serial.print(maxm_ct);
    //   Serial.print(",");
    //   Serial.print(ct_read[j]);
    //   Serial.print(",");
    //   Serial.println(readings[j]);
    // }
    ///////////////////////////////////////////////
    voltage1 = maxm * v_mul;
    voltage = 230;
    current = abs((maxm_ct - c_offset) * c_mul);
    Serial.println(current);
    /////////////////////////////////////////////////////cl1
    if (UID == "/energy_meter/cl1")
    {
      if (current < 0.2)
      {
        current = 0;
      }
      if (current >= 0.2 && current < 0.37)
      {
        current = 0.2608696; //60
      }
      if (current >= 0.37 && current < 0.47)
      {
        current = 0.434782608; //100
      }
      if (current >= 0.47 && current < 0.54)
      {
        current = 0.52173913;
      }
      if (current >= .54 && current < 0.75)
      {
        current = 0.6956522; //160
      }
      if (current >= .75 && current <= 0.9)
      {
        current = 0.86956522;
      }
    }

    ///////////////////////////////////////////////////////// cl2
    if (UID == "/energy_meter/cl2")
    {
      if (current < 0.29)
      {
        current = 0;
      }
      if (current >= 0.29 && current < 0.44)
      {
        current = 0.2608696; //60
      }
      if (current >= 0.44 && current < 0.5)
      {
        current = 0.434782608; //100
      }
      if (current >= 0.5 && current < 0.6)
      {
        current = 0.52173913;
      }
      if (current >= .6 && current < 0.8)
      {
        current = 0.6956522; //160
      }
      if (current >= .8 && current <= 0.9)
      {
        current = 0.86956522;
      }
    }

    load = voltage * current;
    Serial.println(current);
    Serial.println(load);
    Serial.println(",");
    if (Firebase.getString(firebaseData, UID + "/supply"))
    {
      if (firebaseData.stringData() == "1")
      {
        supply = 1;
      }
      else
      {
        supply = 0;
      }

      digitalWrite(relay, supply);
    }
    else
    {
      while (Firebase.getInt(firebaseData, UID + "/supply"))
      {
        digitalWrite(relay, 0);
        lcd.clear();
        lcd.print("waiting...");
      }
      if (firebaseData.stringData() == "1")
      {
        supply = 1;
      }
      else
      {
        supply = 0;
      }
      digitalWrite(relay, supply);
    }
    // ////////////////////////////////////////////Get Time from server
    // struct tm timeinfo;
    // if (!getLocalTime(&timeinfo))
    // {
    //   Serial.println("Failed to obtain time");
    // }
    // else
    // {
    //   strftime(timeStringBuff, sizeof(timeStringBuff), "%Y%m", &timeinfo);
    // }
    ///////////////////////////////////////////////Get and put monthly kwh
    if (load == load1 )
    {
      
      if (Firebase.getFloat(firebaseData, UID + "/" + timeStringBuff + "/kwh"))
      {
        // regular
        kwh_m = firebaseData.floatData();
        kwh_m = kwh_m + (load / ((millis() - ttt)));
        if (Firebase.setFloat(firebaseData, UID + "/" + timeStringBuff + "/kwh", kwh_m)) ///   (load/(1000*((millis()-ttt)/1000)))
        {
          ttt = millis();
          // if (Firebase.getFloat(firebaseData, "/energy_meter/tarrif1"))
          // {
          //   tarrif1=firebaseData.floatData();
          // }
          // if (Firebase.getFloat(firebaseData, "/energy_meter/tarrif2"))
          // {
          //   tarrif2=firebaseData.floatData();
          // }
          if (Firebase.getString(firebaseData, "/energy_meter/tarrif1"))
          {
            sscanf(firebaseData.stringData().c_str(), "%f", &tarrif1);
          }
          if (Firebase.getString(firebaseData, "/energy_meter/tarrif2"))
          {
            sscanf(firebaseData.stringData().c_str(), "%f", &tarrif2);
          }
          if (kwh_m > 5)
          {
            bill = kwh_m * tarrif2;
          }
          else
          {
            bill = kwh_m * tarrif1;
          }
          if (Firebase.setFloat(firebaseData, UID + "/" + timeStringBuff + "/bill", bill))
          {
          }
        }
      }
      else
      {
        if (Firebase.getInt(firebaseData, UID + "/supply"))
        {
          //New Month
          if (Firebase.setFloat(firebaseData, UID + "/" + timeStringBuff + "/kwh", (load / ((millis() - ttt))))) ///   (load/(1000*((millis()-ttt)/1000)))
          {
            ttt = millis();
          }
        }
        else
        {
          //error
        }
      }

      ///////////////////////////////////////////////Put kw
      if (Firebase.setFloat(firebaseData, UID + "/kw", load))
      {
      }
      if (disp)
      {
        disp = !disp;
        lcd.clear();
        lcd.print(voltage);
        lcd.print(" V ");
        lcd.print(current);
        lcd.print(" A");
        lcd.setCursor(0, 1);
        lcd.print(load);
        lcd.print(" W");
      }
      else
      {
        disp = !disp;
        lcd.clear();
        lcd.print(kwh_m);
        lcd.print(" KWh ");
        lcd.setCursor(0, 1);
        lcd.print(bill);
        lcd.print(" TK");
      }
    }
    load1 = load;
  }
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %m %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%m");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay, 10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}