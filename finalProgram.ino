//Headers
#include "dht11.h"
#include <MsTimer2.h>
#include <ESP8266.h>  
#include <SoftwareSerial.h> 
                           
dht11 DHT11;

//Sensors
#define txPin          2
#define rxPin          3
#define tempHmdPin     7       
#define pumpPin        8
#define fanPin         10  
#define soilHmdPin     A0        
#define lightPin       A1 
float temp, hmd, lux, soilHmd;                    
char  lux_c[7], soilHmd_c[7]; 
String fanStatus,pumpStatus;
//tempHmdModule
String sTemp,sHmd;
int incomingByte = 0;
String inputString = ""; 
boolean newLineReceived = false;
boolean startBit  = false;
String returntemp = "";    
char data[10] = {0};

//WIFI
#define SSID        "Sean" 
#define password    "qdzwcaex"
#define ID          "562733330" 
#define apiKey      "Sx=7vHJJkmka=OpFXfLLfsh7c=k=";
#define hostName    "api.heclouds.com"
#define hostPort    (80)
#define EspSerial mySerial
SoftwareSerial mySerial(txPin, rxPin);
ESP8266 wifi(&EspSerial);

//Networks
#define sensorInterval  1000           
#define netInterval     3000 
unsigned long netTime = millis();                         
unsigned long sensorTime = millis();                        
String postString;
String jsonToSend; 
char buffer[10];

void setup()    
{ 
  Serial.begin(115200);
  
  //To initialize sensors
   pinMode(tempHmdPin,OUTPUT);
   pinMode(pumpPin,OUTPUT);
   pinMode(fanPin,OUTPUT);
   digitalWrite(pumpPin,LOW);
   digitalWrite(fanPin,LOW);

 //To initialize WIFI
  while (!Serial);
  Serial.println("===============Setup begins===============");
  
  WifiInit(EspSerial,9600);

  //Serial.println("FW version:");
  //Serial.println(wifi.getVersion().c_str());

  //if (wifi.setOprToStationSoftAP()) Serial.println("To station & softap OK.");
  //else Serial.println("To station & softap ERROR.");

  if (wifi.joinAP(SSID, password)) {
    Serial.println("WIFI connected.");
    Serial.println("IP:");
    Serial.println( wifi.getLocalIP().c_str());
  } else Serial.println("WIFI connection ERROR.");
 
  //if (wifi.disableMUX()) Serial.println("Single OK.");
  //else Serial.println("Single ERROR.");
  
  Serial.println("===============Setup ends===============");
}

void loop()
{   
  if (sensorTime > millis())  sensorTime = millis();  
    
  if(millis() - sensorTime > sensorInterval)               
  {  
    getSensorData();
    judgeEffector();                                        
    sensorTime = millis();
  }  

  if (netTime > millis())  netTime = millis();
  
  if (millis() - netTime > netInterval)                  
  {                
    updateSensorData();                                 
    netTime = millis();
  }
}



//
void judgeEffector(){
  if(temp>=30){
    analogWrite(fanPin,6*temp);
    fanStatus = "ON";
  }
  else{
    analogWrite(fanPin,0);
    fanStatus = "OFF";
  }
  
  if(soilHmd<=58){
    digitalWrite(pumpPin,HIGH);
    pumpStatus = "ON";
  }
  else{
    digitalWrite(pumpPin,LOW);
    pumpStatus = "OFF";
  }
  
  }
void getSensorData(){  
  if(1)
  {
       int chk = DHT11.read(tempHmdPin);           
       temp = DHT11.temperature;        
       hmd = DHT11.humidity;          
       memset(data, 0x00, sizeof(data));       
       dtostrf(temp, 4, 1, data);  
       sTemp = data;        
       sHmd =  "";    
       sHmd += hmd;
       inputString = "";       
  }
    lux = (1023-analogRead(A1))/10.23;
    soilHmd = 3*analogRead(A0)/10.23;   
    dtostrf(lux, 3, 1, lux_c);
    dtostrf(soilHmd, 3, 1, soilHmd_c);
}

void updateSensorData() {
  if (wifi.createTCP(hostName, hostPort)) { 
    Serial.println("===============New Datapoint Sending===============");
    Serial.println("Create TCP OK.\r\n");

    jsonToSend="{\"Temperature\":";
    jsonToSend+="\""+sTemp+"\"";
    jsonToSend+=",\"Humidity\":";
    jsonToSend+="\""+sHmd+"\"";
    jsonToSend+=",\"LightIntesity(%)\":";
    dtostrf(lux,1,2,buffer);
    jsonToSend+="\""+String(buffer)+"\"";
    jsonToSend+=",\"SoilHumidity(%)\":";
    dtostrf(soilHmd,1,2,buffer);
    jsonToSend+="\""+String(buffer)+"\"";
    jsonToSend+=",\"FanStatus\":";
    jsonToSend+="\""+fanStatus+"\"";
    jsonToSend+=",\"PumpStatus\":";
    jsonToSend+="\""+pumpStatus+"\"";
    jsonToSend+="}";

    postString="POST http://api.heclouds.com/devices/";
    postString+=ID;
    postString+="/datapoints?type=3 HTTP/1.1";
    postString+="\r\n";
    postString+="api-key:";
    postString+=apiKey;
    postString+="\r\n";
    postString+="Host:api.heclouds.com\r\n";
    postString+="Connection:close\r\n";
    postString+="Content-Length:";
    postString+=jsonToSend.length();
    postString+="\r\n";
    postString+="\r\n";
    postString+=jsonToSend;
    postString+="\r\n";
    postString+="\r\n";
    postString+="\r\n";
  const char *postArray = postString.c_str();                 
  Serial.println(postArray);
  wifi.send((const uint8_t*)postArray, strlen(postArray));    
  Serial.println("===============New Datapoint sent===============");
  delay(1000);
   
  wifi.releaseTCP();                               
  postArray = NULL;                                       
  } 
  else {
    Serial.println("Create TCP ERROR.");
    Serial.println("===============Sending Failed===============");
  }
}


