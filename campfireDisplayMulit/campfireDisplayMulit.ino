#include <ThingSpeak.h>

//#define debug

#include <WiFi.h>


#include <HTTPClient.h>
#define pinMCLR 17
#define pinSERCLK 18
#define pinRCLK 1
#define pinOE 22
#define pinSERIN 23


TaskHandle_t Task1 = NULL;
TaskHandle_t Task2 = NULL;


uint8_t pinLED[3] = {0, 16, 4};
volatile uint8_t displayChar[11] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
const uint8_t digit[11] =
{
  0b11010111,
  0b10000001,
  0b11001110,
  0b11001011,
  0b10011001,
  0b01011011,
  0b01011111,
  0b11000001,
  0b11011111,
  0b11011011,
  0b11111111
};
enum {NEW, NOW, LAST, STEP};
enum {DATE, MOT};

double EDATA[2][4] = {0};

int mood = -1;

double displayD[2] = {0};

//const char* ssid = "AlphaP";
//const char* password =  "";
const char* ssid = "FIBOWIFI";
const char* password =  "Fibo@2538";
unsigned long long int timer = 0;
char thingSpeakAddress[] = "api.thingspeak.com";
unsigned long channelID = 581739;
char* readAPIKey = "HQFNZEJYW6GQT7KR";
char* writeAPIKey = "YYYYYYYYYYYYYYYY";
WiFiClient client;
float readTSData( long TSChannel, unsigned int TSField ) {

  static int ErrorCount = 3;
  float data =  ThingSpeak.readFloatField( TSChannel, TSField, readAPIKey );
#ifdef debug
  Serial.println( " Data read from ThingSpeak: " + String( data, DEC ) );
#endif

  if (ThingSpeak.getLastReadStatus() == 200)
  {
    if (ErrorCount <= 0)
    {
      ErrorCount = 0;
    }
    ErrorCount = ErrorCount >= 3 ? ErrorCount : ErrorCount + 1;
  }
  else
  {
    data = -999999;
#ifdef debug
    Serial.println( " Error " + String( ThingSpeak.getLastReadStatus(), DEC ) );
#endif
    ErrorCount--;
  }
  if (ErrorCount < 0)
  {
    client.stop();
    WiFi.disconnect();
  }
  

  return data;

}

void DisplayUpdate()
{
#ifdef debug
  Serial.println("-------");
#endif
  for (int i = 0; i < 6; i++)
  {
    displayChar[i + 5] = ((int)displayD[MOT] % (int)pow(10, i + 1)) / pow(10, i);
#ifdef debug
    Serial.print(String(displayChar[i + 5] , DEC ));
#endif
  }
#ifdef debug
  Serial.println("-------");
#endif
  for (int i = 0; i < 5; i++)
  {
    displayChar[i] = ((int)displayD[DATE] % (int)pow(10, i + 1)) / pow(10, i);
#ifdef debug
    Serial.print( String(displayChar[i] , DEC ));
#endif
  }
#ifdef debug
  Serial.println("-------");
#endif

}

void LEDstatus()
{
  static uint32_t timestamp = 0 ;
  static uint32_t rate = 2000;
  static uint8_t toggle = 0;
  if (millis() - timestamp > rate || ( millis() - timestamp < 0))
  {
    timestamp = millis();
    toggle = !toggle;
    switch (mood)
    {
      case 0:
        rate = 1000;
        digitalWrite(pinLED[0], toggle);
        digitalWrite(pinLED[1], 0);
        digitalWrite(pinLED[2], 0);
        break;
      case 1:
        rate = 500;
        digitalWrite(pinLED[1], toggle);
        digitalWrite(pinLED[0], 0);
        digitalWrite(pinLED[2], 0);
        break;
      case 2:
        rate = 200;
        digitalWrite(pinLED[2], toggle);
        digitalWrite(pinLED[0], 0);
        digitalWrite(pinLED[1], 0);
        break;
      default:
        digitalWrite(pinLED[0], 1);
        digitalWrite(pinLED[1], 1);
        digitalWrite(pinLED[2], 1);
        break;


    }
  }
}

void CommunicationTask()
{
  static uint32_t timestamp = 5000;
  static int i = 1;
  //while (1);
  {
    //vTaskDelay(10);
    if (WiFi.status() != WL_CONNECTED && ( millis() - timestamp > 5000) || ( millis() - timestamp < 0)) {
      timestamp = millis();
      WiFi.begin(ssid, password);
#ifdef debug
      Serial.println("Connecting to WiFi..");
#endif
      i++;
      if (i % 20 == 0)
      {
        WiFi.disconnect();
        Serial.println("Reconention");
        WiFi.begin(ssid, password);
      }
      if (i > 60)
      {
        WiFi.disconnect();
        ESP.restart();
      }
    }

    if ((WiFi.status() == WL_CONNECTED) && ( millis() - timestamp > 1000) || ( millis() - timestamp < 0))
    {
      static int readts = -1;
      timestamp = millis();
      ThingSpeak.begin( client );

      switch (readts)
      {
        default:
          readts++;
          break;
        case -1:
          EDATA[DATE][NEW] = readTSData( channelID, 1 );
          EDATA[MOT][NEW] = readTSData( channelID, 2 );
          mood = readTSData( channelID, 3 );
          readts++;
          break;
        case 30:
          EDATA[DATE][NEW] = readTSData( channelID, 1 );
          readts++;
          break;
        case 60:
          EDATA[MOT][NEW] = readTSData( channelID, 2 );
          readts++;
          break;
        case 90:
          mood = readTSData( channelID, 3 );
          readts = 0;
          break;
      }

      if ( EDATA[DATE][NEW] != EDATA[DATE][NOW] && EDATA[DATE][NEW] != -999999)
      {
        EDATA[DATE][NOW] = EDATA[DATE][NEW];
        if (EDATA[DATE][NOW] >= EDATA[DATE][LAST] && EDATA[DATE][LAST] != 0)
        {
          EDATA[DATE][STEP] = (EDATA[DATE][NOW] - EDATA[DATE][LAST]) / 360.0;
        }
        else
        {
          EDATA[DATE][LAST] = EDATA[DATE][NOW];
          EDATA[DATE][STEP] = 0;
#ifdef debug
          Serial.println( " update date " + String( EDATA[DATE][LAST], DEC ) );
#endif
        }
      }
      if (EDATA[DATE][LAST] < EDATA[DATE][NOW])
      {
        EDATA[DATE][LAST] += EDATA[DATE][STEP];
#ifdef debug
        Serial.println( " update date " + String( EDATA[DATE][LAST], DEC ) );
#endif
      }
      //
      if ( EDATA[MOT][NEW] != EDATA[MOT][NOW] && EDATA[MOT][NEW] != -999999)
      {
        EDATA[MOT][NOW] = EDATA[MOT][NEW];
        if (EDATA[MOT][NOW] >= EDATA[MOT][LAST] && EDATA[MOT][LAST] != 0)
        {
          EDATA[MOT][STEP] = (EDATA[MOT][NOW] - EDATA[MOT][LAST]) / 360.0;
        }
        else
        {
          EDATA[MOT][LAST] = EDATA[MOT][NOW];
          EDATA[MOT][STEP] = 0;
#ifdef debug
          Serial.println( " update date " + String( EDATA[MOT][LAST], DEC) );
#endif
        }
      }
      if (EDATA[MOT][LAST] < EDATA[MOT][NOW])
      {
        EDATA[MOT][LAST] += EDATA[MOT][STEP];
#ifdef debug
        Serial.println( " update date " + String( EDATA[MOT][LAST], DEC ) );
#endif
      }
      displayD[MOT] = EDATA[MOT][LAST];
      displayD[DATE] = EDATA[DATE][LAST];
      DisplayUpdate();




    }

  }
}

void displayRun()
{
  static uint8_t state = 0, pos = 0, sec = 0, updateblink = 0;
  static uint32_t timestamp = 0;
  if (millis() - timestamp > 1 || ( millis() - timestamp < 0))
  {
    timestamp = millis();
    switch (state)
    {
      default:
      case 0:
        pos = 10;
        sec = 0;
        digitalWrite(pinRCLK, HIGH);
        digitalWrite(pinSERCLK, LOW);
        vTaskDelay(10);
        state = 1;
        break;
      case 1:
        digitalWrite(pinSERCLK, LOW);
        if (pos == 0 && sec == 5)
        {
          digitalWrite(pinSERIN, updateblink);
        }
        else if (pos == 5 && sec == 5)
        {
          if (WiFi.status() == WL_CONNECTED)
          {
            digitalWrite(pinSERIN, HIGH);
          }
          else
          {
            digitalWrite(pinSERIN, LOW);
          }
        }
        else if (digit[displayChar[pos]] & (1 << sec))
        {
          digitalWrite(pinSERIN, HIGH);
        }
        else
        {
          digitalWrite(pinSERIN, LOW);
        }
        if (sec == 7)
        {
          if (pos == 0)
          {
            state = 99;
          }
          else
          {
            pos--;
            sec = 0;
            state = 2;
          }
        }
        else
        {
          sec++;
          state = 2;
        }
        break;
      case 2:
        digitalWrite(pinSERCLK, HIGH);
        state = 1;
        break;
      case 99:
        digitalWrite(pinSERCLK, HIGH);
        digitalWrite(pinRCLK, LOW);
        updateblink = !updateblink;
        state = 0;
        break;
    }
  }
}
void setup()
{
#ifdef debug
  Serial.begin(115200);
#endif

  pinMode(pinOE, OUTPUT);
  digitalWrite(pinOE, LOW);
  pinMode(pinMCLR, OUTPUT);
  digitalWrite(pinMCLR, HIGH);
  pinMode(pinSERCLK, OUTPUT);
  digitalWrite(pinSERCLK, LOW);
  pinMode(pinRCLK, OUTPUT);
  digitalWrite(pinRCLK, HIGH);
  pinMode(pinSERIN, OUTPUT);
  digitalWrite(pinSERIN, LOW);
  pinMode(pinLED[0], OUTPUT);
  pinMode(pinLED[1], OUTPUT);
  pinMode(pinLED[2], OUTPUT);
  //xTaskCreatePinnedToCore(CommunicationTask, "Task1", 100000, NULL, 1, &Task1, 0);
  //xTaskCreatePinnedToCore(DisplayTask, "Task2", 1000, NULL, 1, &Task2, 1);


}
void DisplayTask()
{
  //while (1)
  {
    LEDstatus();
#ifndef debug
    displayRun();
#endif
  }
}
void loop()
{
  CommunicationTask();
  DisplayTask();
}
