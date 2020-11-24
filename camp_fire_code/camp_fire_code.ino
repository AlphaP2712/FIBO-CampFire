#include <WiFi.h>


#include <HTTPClient.h>
#include "EmonLib.h"

EnergyMonitor emon1;
EnergyMonitor emon2;
EnergyMonitor emon3;

const char* ssid = "FIBOWIFI";
const char* password =  "Fibo@2538";
unsigned long long int timer =0;
void setup()
{
  
  Serial.begin(115200);
  delay(4000);
  emon1.current(A6, 111.1);
  emon2.current(A7, 111.1);
  emon3.current(A4, 111.1);
  WiFi.begin(ssid, password);
  int i =1;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
    i++;
    if (i%20==0)
    {
      WiFi.disconnect();
      Serial.println("Reconention");
      WiFi.begin(ssid, password);
    }
    if (i>60)
    {
      WiFi.disconnect();
     ESP.restart();
    }
  }

  Serial.println("Connected to the WiFi network");
  emon1.calcIrms(4095);
  emon2.calcIrms(4095);
  emon3.calcIrms(4095);
  emon1.calcIrms(4095);
  emon2.calcIrms(4095);
  emon3.calcIrms(4095);
  emon1.calcIrms(4095);
  emon2.calcIrms(4095);
  emon3.calcIrms(4095);
  emon1.calcIrms(4095);
  emon2.calcIrms(4095);
  emon3.calcIrms(4095);

}

void loop()
{
  double current1 = emon1.calcIrms(4095)/4.0;
  double current2 = emon2.calcIrms(4095)/4.0;
  double current3 = emon3.calcIrms(4095)/4.0;


  double money = 0.132 *  ( current1 + current2 + current3 );

  String c1 = String(current1, 3);
  String c2 = String(current2, 3);
  String c3 = String(current3, 3);
  String c4 = String(money, 3);
  if ((WiFi.status() == WL_CONNECTED))
  { //Check the current connection status

    HTTPClient http;
    http.begin("https://api.thingspeak.com/update?api_key=9I9PUG0IMOJFITT5&field1=" + c1 + "&field2=" + c2 + "&field3=" + c3 + "&field4=" + c4); //Specify the URL
    int httpCode = http.GET();                                        //Make the request

    if (httpCode >= 200 &&httpCode < 300)
    { //Check for the returning code

      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      http.end();
    }

    else
    {
      Serial.println("Error on HTTP request");
      http.end(); //Free the resources
      ESP.restart();
    }

    http.end(); //Free the resources
  }
  else
  {
    WiFi.disconnect();
   ESP.restart();
  }
  while(millis()-timer<360000&&millis()-timer>0);
  timer =millis();
  
}
