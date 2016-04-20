#include <DHT.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>

//rain gauge
const int REED = 9;              
int val = 0;                    
int old_val = 0;                
int REEDCOUNT = 0;    
long starttime = 0;
long endtime = 0;

//DHT
#define DHTPIN 3 
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

//BMP180
#define ALTITUDE 180 //Profitis Ilias, Crete
SFE_BMP180 pressure;

//Wind speed and direstion
#define uint  unsigned int
#define ulong unsigned long
#define PIN_ANEMOMETER  2    // Digital 3
#define PIN_VANE        3     // Analog 3
#define MSECS_CALC_WIND_SPEED 5000
#define MSECS_CALC_WIND_DIR   5000
#define NUMDIRS 8
ulong   adc[NUMDIRS] = {26, 45, 77, 118, 161, 196, 220, 256};
char *strVals[NUMDIRS] = {"W","NW","N","SW","NE","S","SE","E"};
byte dirOffset=0;
volatile int numRevsAnemometer = 0; 
ulong nextCalcSpeed;                
ulong nextCalcDir;                  
ulong time;     


//Fwtinotita
const int lightPin = A0;  
int lightValue = 0;

//Gas Sensor
const int gasPin = A1; 
int gasValue = 0;

char status;
double T,P,p0,a;


//Ethernet
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

IPAddress ip(10, 0, 3, 163);
IPAddress myDns(147, 95, 2, 10);
IPAddress gateway(10, 0, 3, 1);
IPAddress subnet(255, 255, 255, 0);
EthernetClient client;
char server[] = "www.overload.ninja";
unsigned long lastConnectionTime = 0;       // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 5000; // delay between updates, in milliseconds

////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup(){

   Serial.begin(9600);
   
   //Ethernet
   Ethernet.begin(mac);

   //rain gauge 
   pinMode (REED, INPUT_PULLUP); 
 
   //DHT
   dht.begin();
 
   //Wind speed and direstion
   pinMode(PIN_ANEMOMETER, INPUT);
   digitalWrite(PIN_ANEMOMETER, HIGH);
   attachInterrupt(0, countAnemometer, FALLING);
   nextCalcSpeed = millis() + MSECS_CALC_WIND_SPEED;
   nextCalcDir   = millis() + MSECS_CALC_WIND_DIR;
 
   ////////////////////////////////////////////////////
   
   if (pressure.begin())
    Serial.println("BMP180 init success");
   else
   {
      Serial.println("BMP180 init fail\n\n");
      while(1);
   }
}


////////////////////////////////////////////////////////////
void loop(){
  

  int x, y, z;
  
  
/////////////////rain gauge/////////////////////////

 val = digitalRead(REED);    
 
 starttime = millis();
 endtime = starttime;
 while ((endtime - starttime) <=300000)
  {    
    if ((val == LOW) && (old_val == HIGH))    
    {
       delay(10);                  
       REEDCOUNT = REEDCOUNT + 1;   
       old_val = val;        
    }    
    else 
    {
       old_val = val;      
    }
       val = digitalRead(REED);
       endtime = millis(); 
  }
  
///////////////////DHT11////////////////////////////


  float h = dht.readHumidity();
  float t = dht.readTemperature();
    
//////////////////BMP180///////////////////////////  

 status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0)
    {
      status = pressure.startPressure(3);
      if (status != 0)
      {
        // Wait for the measurement to complete:
        delay(status);
        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          p0 = pressure.sealevel(P,ALTITUDE);
          a = pressure.altitude(P,p0);
        }
      }
    }
  }

  delay(5000); 
  
//////////Taxitita kai kateuthinsi aera//////////////
   time = millis();
   if (time >= nextCalcSpeed) {
      nextCalcSpeed = time + MSECS_CALC_WIND_SPEED;
   }
   if (time >= nextCalcDir) {
      calcWindDir();
      nextCalcDir = time + MSECS_CALC_WIND_DIR;
   }
   
//////////////////Fwtinotita//////////////////////////

lightValue = analogRead(lightPin);
//Serial.println(lightValue);

/////////////////////Gas/////////////////////////////

gasValue = analogRead(gasPin);
//Serial.println(gasValue);

//////////////////ETHERNET////////////////////////////
if (millis() - lastConnectionTime > postingInterval) 
{  
  if (client.connect(server, 80)) {
   // Serial.println("connecting...");
 
   
    client.print("GET /scripts/m_s?user=profiti_ilia&hi=");
    client.print(h);
    client.print("&ti=");
    client.print(t);
    client.print("&bt=");
    client.print(T);
    client.print("&bp=");
    client.print(P);
    client.print("&br=");
    client.print(p0);
    client.print("&ba=");
    client.print(a);
    client.print("&r=");
    client.print(REEDCOUNT);
    z = calcWindDir();
    client.print("&wd=");
    client.print(strVals[z]);
    client.print("&ws=");
    y = calcWindSpeed();
    x = y / 10;
    client.print(x);
    client.print('.');
    x = y % 10;
    client.print(x);
    client.print("&l=");
    client.print(lightValue);
    client.println(" HTTP/1.1");
    client.println("HOST: www.overload.ninja");
    client.println("Connection: close");
    client.println();
    client.stop(); 
    
    lastConnectionTime = millis();
  }
  }  
  REEDCOUNT = 0;
  
}
/////////////////////////////////////////////////////////
void countAnemometer() {
   numRevsAnemometer++;
}
/////////////////////////////////////////////////////////
int calcWindDir() {
   int val;
   byte x, reading;

   val = analogRead(PIN_VANE);
   val >>=2;                        
   reading = val;
   for (x=0; x<NUMDIRS; x++) {
      if (adc[x] >= reading)
         break;
   }
   x = (x + dirOffset) % 8;   
   return x;
}
////////////////////////////////////////////////////////   
   
int calcWindSpeed() {
   int x, iSpeed;
  
   long speed = 14920;
   speed *= numRevsAnemometer;
   speed /= MSECS_CALC_WIND_SPEED;
   iSpeed = speed;         
   numRevsAnemometer = 0; 
   x = iSpeed / 10;
   x = iSpeed % 10;
   return iSpeed;
}


