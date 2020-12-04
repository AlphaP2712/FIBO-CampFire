#include <WiFi.h>
#include <HTTPClient.h>
#include "EmonLib.h"
#define CurrentCaribateFactor 111.1
#define CostFactor 0.132

//Energy measument Variable for 3 phase AC
EnergyMonitor emon1;
EnergyMonitor emon2;
EnergyMonitor emon3;
//Wifi Config Variable
const char* ssid = "FIBOWIFI";
const char* password =  "Fibo@2538";
//Timer config
unsigned long long int timer =0;
void setup()
{
  
  Serial.begin(115200);
  delay(4000);
  //init Energy Monitor with caribate Factor
  emon1.current(A6, CurrentCaribateFactor);
  emon2.current(A7, CurrentCaribateFactor);
  emon3.current(A4, CurrentCaribateFactor);
  //init wifi
  //NOTE: Because Fibowifi is Not good to stay Connect, So . if Can't connect to network ,the best way (and easy) to fix problem is Reset Esp itself.
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
  //if we can connect wifi , calc energy 2-3 time before use to make data stable.
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
//NOTE: point to calculate Energy data every 6 min is Thinkspeak server.it can update data 15 time per hour , so , we delay 6 min to sent 1 data every time 
void loop()
{
  //measurement Energy 
  double current1 = emon1.calcIrms(4095)/4.0;
  double current2 = emon2.calcIrms(4095)/4.0;
  double current3 = emon3.calcIrms(4095)/4.0;

  //Caluclate cost estimate in next 6 min,Yes it just estimate.  
  double money = CostFactor *  ( current1 + current2 + current3 );
  //make string to sent to thingspeak server
  String c1 = String(current1, 3);
  String c2 = String(current2, 3);
  String c3 = String(current3, 3);
  String c4 = String(money, 3);
  //sent to thingspeak
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
  //delay 6 min
  while(millis()-timer<360000&&millis()-timer>0);
  timer =millis();
  
}
