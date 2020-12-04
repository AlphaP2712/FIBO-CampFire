#include <ThingSpeak.h>
// to use serial debug ,define debug
//#define debug

#include <WiFi.h>


#include <HTTPClient.h>

//define pin for 7 secment , use 74hc575 shift register as memory 
#define pinMCLR 17
#define pinSERCLK 18
#define pinRCLK 1
#define pinOE 22
#define pinSERIN 23


uint8_t pinLED[3] = {0, 16, 4};//Green,yellow,red

//real digit output memory , datain this will convert to digit below and output real time
volatile uint8_t displayChar[11] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10};

//binary segment display,use to display segment 1 is on,0is off
const uint8_t digit[11] =
{
  0b11010111, //0
  0b10000001, //1
  0b11001110, //2
  0b11001011, //3
  0b10011001, //4
  0b01011011, //5
  0b01011111, //6
  0b11000001, //7
  0b11011111, //8
  0b11011011, //9
  0b11111111  //ALL Segment
};
//Word for easy to read array
enum {NEW, NOW, LAST, STEP}; 
enum {DATE, MOT};
//variable that store data to calculate display step use enum above to respesent data
double EDATA[2][4] = {0};
//enum{GREEN,YELLOW,RED}
//variable that store mood led 
int mood = -1;
//Real data to display
double displayD[2] = {0};

//server and wifi Config
const char* ssid = "FIBOWIFI";
const char* password =  "Fibo@2538";
unsigned long long int timer = 0;
char thingSpeakAddress[] = "api.thingspeak.com";
unsigned long channelID = 581739;
char* readAPIKey = "HQFNZEJYW6GQT7KR";
char* writeAPIKey = "YYYYYYYYYYYYYYYY";
WiFiClient client;

//function to read float data form thingspeak server , use a few sec to read once,Don't Call Too much
float readTSData( long TSChannel, unsigned int TSField ) {

  static int ErrorCount = 3;
  float data =  ThingSpeak.readFloatField( TSChannel, TSField, readAPIKey );
#ifdef debug
  Serial.println( " Data read from ThingSpeak: " + String( data, DEC ) );
#endif
  //check status of server. if cannot connect or error .reset wifi.
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


//function to Update data to display on seven segment(Convert data from Floating point to array of digit)
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
//control MOOD LED
void LEDstatus()
{
  static uint32_t timestamp = 0 ;
  static uint32_t rate = 2000;
  static uint8_t toggle = 0;
  //free running led function 
  if (millis() - timestamp > rate || ( millis() - timestamp < 0))
  {
    timestamp = millis();
    toggle = !toggle;
    switch (mood)
    {
      case 0:
        rate = 1000;
        digitalWrite(pinLED[GREEN], toggle);
        digitalWrite(pinLED[YELLOW], 0);
        digitalWrite(pinLED[RED], 0);
        break;
      case 1:
        rate = 500;
        digitalWrite(pinLED[YELLOW], toggle);
        digitalWrite(pinLED[GREEN], 0);
        digitalWrite(pinLED[RED], 0);
        break;
      case 2:
        rate = 200;
        digitalWrite(pinLED[RED], toggle);
        digitalWrite(pinLED[GREEN], 0);
        digitalWrite(pinLED[YELLOW], 0);
        break;
      default://for startup check,or can't recive data from server
        digitalWrite(pinLED[GREEN], 1);
        digitalWrite(pinLED[YELLOW], 1);
        digitalWrite(pinLED[RED], 1);
        break;


    }
  }
}
//like it name,"Communication Task" ,get it?
void CommunicationTask()
{
  static uint32_t timestamp = 5000;
  static int i = 1;
  //while (1);
  {
    //vTaskDelay(10);
    //try to connect to wifi
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
    //if we can connect to wifi, update data to Edata every 1 sec
    if ((WiFi.status() == WL_CONNECTED) && ( millis() - timestamp > 1000) || ( millis() - timestamp < 0))
    {
      static int readts = -1;
      timestamp = millis();
      ThingSpeak.begin( client );
      // but if we update every 1 sec ,it will too much too handel in this little cpu .So update every 10 cycle 
      switch (readts)
      {
        default:
          readts++;
          break;
        case -1:// first time get all data , it take 2-3 secound .but it for first time
          EDATA[DATE][NEW] = readTSData( channelID, 1 );
          EDATA[MOT][NEW] = readTSData( channelID, 2 );
          mood = readTSData( channelID, 3 );
          readts++;
          break;
        case 30: //update 1 day cost 
          EDATA[DATE][NEW] = readTSData( channelID, 1 );
          readts++;
          break;
        case 60: // update 30 day cost
          EDATA[MOT][NEW] = readTSData( channelID, 2 );
          readts++;
          break;
        case 90: //update mood
          mood = readTSData( channelID, 3 );
          readts = 0;
          break;
      }
      //check data is valid and calculate how many cost to add in 1 micro step , because sensor is update every 6 min 
      //and we need to update data every secound . to make number that show go up smoothly , we simmulate cost to display by
      //make a micro step every sec and calc how much number to add in 1 sec to make number goto real cost in 6 min.it have 360 micro step.
      if ( EDATA[DATE][NEW] != EDATA[DATE][NOW] && EDATA[DATE][NEW] != -999999)
      {
        EDATA[DATE][NOW] = EDATA[DATE][NEW];
        if (EDATA[DATE][NOW] >= EDATA[DATE][LAST] && EDATA[DATE][LAST] != 0)
        {
          EDATA[DATE][STEP] = (EDATA[DATE][NOW] - EDATA[DATE][LAST]) / 360.0;
        }
        else // on 00.00 o'clock data will reset to zero on server. so we don't simulate microstep in this situation
        {
          EDATA[DATE][LAST] = EDATA[DATE][NOW];
          EDATA[DATE][STEP] = 0;
#ifdef debug
          Serial.println( " update date " + String( EDATA[DATE][LAST], DEC ) );
#endif
        }
      }
      //on normal operation add microstep  
      if (EDATA[DATE][LAST] < EDATA[DATE][NOW])
      {
        EDATA[DATE][LAST] += EDATA[DATE][STEP];
#ifdef debug
        Serial.println( " update date " + String( EDATA[DATE][LAST], DEC ) );
#endif
      }
      //same as date update
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

      //update final data to display variable and update to 7 segment output
      displayD[MOT] = EDATA[MOT][LAST];
      displayD[DATE] = EDATA[DATE][LAST];
      DisplayUpdate();




    }

  }
}
//function to run 7segment display continusly 
//should implement to SPI.But for now.It implement as state machine
//for more infomation about what this do , see 74hc595 datasheet
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
//setup all input output pin
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
  //run function continusly
  CommunicationTask();
  DisplayTask();
}
