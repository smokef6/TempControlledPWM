/*********
  kruegro
  Complete project details at https://
*********/
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <MyWifiSecrets.h>
#include <EEPROM.h>           //https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include "c_Page_favicon.h"
#include "EmonLib.h"

#include <WiFiClient.h>  // for ajax
#include <myWebsite.h>  // for ajax

#define PIN_TEMP_SENSOR D2  // GPIO where the DS18B20 is connected to
#define PIN_TACHO D4
#define PIN_MOSFET_GATE D6
#define PIN_FAN D8

EnergyMonitor emon1;
const byte current1Pin = A0; // ADC-PIN
const byte Spannung = 230;  // Spannung von eurem Netz
int currentReadingIdx = 0;
const int IrmsArraySize = 10;
double irmsValues[IrmsArraySize];

const char* SSID = WIFI_SSID;
const char* PWD = WIFI_PASSWORD;

// Variablen
float _temperatureC;
int _fanSpeed;            // Variable für die Lüftergeschwindigkeit
int _rpm = 0;                   // Variable für die gemittelte Drehzahl
bool _modeAuto = true;
float _setpoint;   // Diese Variable Deklaration in den Haupttab vor "setup()" verschieben um sie im gesamten Sketch verfügbar zu machen.

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;        // will store last time LED was updated

// constants won't change :
const long interval = 1000;           // interval at which to blink (milliseconds)


typedef struct {
  float maxTemp;                 // EEProm value for maximum measured Temperature
  char hostname[30]="";
} PSettings;
PSettings psettings;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(PIN_TEMP_SENSOR);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature tempSensors(&oneWire);

ESP8266WebServer _server(80);



// int getFan_rpm()
// {
//   float rps = 0;                 // Variable mit Kommastelle für die Berechnung der Umdrehungen pro Sekunde
//   float umdrZeit = 0;            // Variable mit Kommastelle für die Zeit pro Umdrehung des Lüfters
//   float flankenZeit =0;          // Variable mit Kommastelle für die Zeit pro Puls des Lüfters 

//   if ( _fanSpeed < 1 ) {      
//     _rpm = 0;
//   }
//   else {
//     analogWrite(PIN_FAN, 255);                 // Den Lüfter konstant mit Strom versorgen damit das Tachosignal funktioniert
//     delay(20);
//     for ( int retryCount=0; retryCount<2; retryCount++ ) {            
//       flankenZeit = pulseIn(PIN_TACHO, HIGH);    // Abfrage der Zeit pro Puls in Mikrosekunden    
//       if (flankenZeit>100) {
//         break;
//       }      
//     }      
//     analogWrite(PIN_FAN, _fanSpeed);            // Setzt die Lüftergeschwindigkeit zurück
//     umdrZeit = ((flankenZeit * 4)/1000);      // Berechnung der Zeit pro Umdrehung in Millisekunden
//     rps = (1000/umdrZeit);                    // Umrechnung auf Umdrehungen pro Sekunde
//     _rpm = (rps*60);                           // Umrechnung auf Umdrehungen pro Minute
//   }    
//   return _rpm;  
// }

void sendWebPage() 
{
  String s = webpage;
  _server.send(200, "text/html", s);
}

void setFanSpeed(int speed) {
  _fanSpeed = speed;
  analogWrite(PIN_FAN, _fanSpeed);
  if ( _fanSpeed < 1 ) {
    digitalWrite(PIN_FAN, LOW);   //Led port ganz ausschalten        
  }
  Serial.print("_fanSpeed = ");
  Serial.println( _fanSpeed );
}

void sensor_data() {
  double irms = emon1.calcIrms(1480);
  irmsValues[currentReadingIdx] = irms;
  currentReadingIdx++;
  if ( currentReadingIdx >= 10 ) {
    currentReadingIdx = 0;
  }
  double irmsMean = 0;
  for ( int i = 0; i < IrmsArraySize; i++ ) {
    irmsMean = irmsMean + irmsValues[i];
  }
  irmsMean = irmsMean / IrmsArraySize;

  Serial.print( irmsMean*Spannung );
  Serial.print( " Watt  -  " );
  double watts = irmsMean*Spannung;
  
  String sensor_value = String(watts);
  _server.send(200, "text/plane", sensor_value);
}


void sendPlainText(String value) {
  _server.send( 200, "text/plane", value );
}

void setLedState() {
  String state = "OFF";
  String act_state = _server.arg("state");
  if (act_state == "0") {
    digitalWrite(LED_BUILTIN,HIGH); //LED aus
    state = "OFF";
  }
  else {
    digitalWrite(LED_BUILTIN,LOW); //LED an
    state = "ON";
  }
  _server.send(200, "text/plane", state);
}

void sendLedState() {
  
  int state = digitalRead(LED_BUILTIN); //LED
  String stateText = (state == 1) ? "ON" : "OFF";    
  
  sendPlainText( stateText );
}

void apiGetHostname() {
  Serial.println("apiGetHostname: ");
  Serial.print("hostname: ");
  Serial.println(psettings.hostname);
  sendPlainText( String(psettings.hostname) );
}

void apiSetHostname() {
  Serial.println("apiSetHostname: ");  
  String hostname = _server.arg("new_hostname");    
  strcpy(psettings.hostname, hostname.c_str());  
  Serial.print("new hostname: ");
  Serial.println( psettings.hostname );  
  WiFi.hostname( psettings.hostname );
  EEPROM.put(0, psettings);
  EEPROM.commit();
  sendPlainText( String(psettings.hostname) );
}

void getModeAuto() {
  if (_modeAuto == true) {
    sendPlainText( "aktiv" );
  } else {
    sendPlainText( "inaktiv" );
  }
}

void sendTemperature() {
  sendPlainText( String(_temperatureC) );
}

void setFanSpeedWeb() {  
  String act_state = _server.arg("fanSpeed");  
  setFanSpeed( atoi(act_state.c_str()) );
}

void sendFanSpeed() {
  sendPlainText( String(_fanSpeed) );
}

void sendMaxTemp() {
  sendPlainText( String(psettings.maxTemp) );
}

void sendModeAuto() { 
  if (_modeAuto == true) {
    sendPlainText("aktiv");
  } else {
    sendPlainText("inaktiv");
  }
}

void setModeAuto() {  
  String act_state = _server.arg("modeAuto");
  if (act_state == "1") {
    _modeAuto = true;    
  }
  else {
    _modeAuto = false;
  }
  sendModeAuto();
}

void sendFavicon() {  
  Serial.println("favicon.ico");
  _server.send_P(200, "image/x-icon", PAGE_favicon, sizeof(PAGE_favicon));
}



void handleTempUpdate() {
  tempSensors.requestTemperatures();
  _temperatureC = tempSensors.getTempCByIndex(0);
  Serial.print(_temperatureC);
  Serial.println("ºC");

  if ( psettings.maxTemp < _temperatureC ) {
    psettings.maxTemp = _temperatureC;    
    EEPROM.put(0, psettings);
    EEPROM.commit();    
  }

  if (_modeAuto) {
    if ( fabs(_temperatureC + 127.0f) < 0.0001 ) {  // temp = -127°C? -> sensor not readable
      _modeAuto = false;
    }

    // Funktion: 0,13*(x-35,5)³+250 siehe https://www.arndt-bruenner.de/mathe/scripts/jsplotter.htm
    float newFanspeed = 0.13 * pow( _temperatureC - 35.5, 3) + 250;
    if (newFanspeed < 10) { newFanspeed = 0; };     // min:10
    if (newFanspeed > 255) { newFanspeed = 255; };  // max:255  
    if (( _fanSpeed == 0 ) && ( newFanspeed >= 40)) { 
      setFanSpeed(100);
      delay(4000);
    }    // 4s Anlaufen nach Ruhemodus
    setFanSpeed( newFanspeed );
  }
}

void get_hook() {
    if(_server.hasArg("fanSpeed")){
      // Variable "name" wird übergeben
      Serial.println("Variable 'fanSpeed' übergeben!");

      // Anschließend die Prüfung, ob die Variable 'name' leer ist:      
      if ( _server.arg("fanSpeed") != "" ){
        String pwm = _server.arg("fanSpeed");
        _modeAuto = false;
        setFanSpeed(pwm.toInt());
      } 
      
      // Wenn Variable die 'name' übergeben wurde, (leer oder nicht) ist:
      // Ausgabe im Webbrowser HTTP-Code 200: Ok
      _server.send(200, "text/plain", String(_fanSpeed));
    }
    if(_server.hasArg("modeAuto")) {
      if ( _server.arg("modeAuto") == "0" ){
        _modeAuto = false;
      }
      else if ( _server.arg("modeAuto") == "1" ){
        _modeAuto = true;
      }
      _server.send(200, "text/plain", String(_modeAuto));
    }
    else if (_server.hasArg("temperature")) {      
      handleTempUpdate();
      _server.send(200, "text/plain", String(_temperatureC));      
    }
    else {
      handleTempUpdate();
      //Wenn gar keine Variablen übergeben wurden
      _server.send(200, "text/html", 
        "fanSpeed: " + String(_fanSpeed) + "<br>" +
        "modeAuto: " + String(_modeAuto) + "<br>" +
        "temperature: " + String(_temperatureC) + + "&deg;C<br>" +
        "maxTemp: " + String(psettings.maxTemp) + "&deg;C");
    }
}


void setupModify() {
  _server.on( "/modified", []() {
    char buf[13];
    if (_server.args()) _setpoint = atof(_server.arg(0).c_str());
    snprintf(buf, sizeof buf, "\"%8.3f\"", _setpoint);
    _server.send(200, "application/json", buf);
  } );
}


void setup() {
  tempSensors.begin();

  pinMode(PIN_FAN, OUTPUT);  // Port aus Ausgang schalten
  pinMode(PIN_MOSFET_GATE, OUTPUT);  // Port aus Ausgang schalten
  pinMode(LED_BUILTIN, OUTPUT); // LED als Output definieren
  digitalWrite(LED_BUILTIN,HIGH); // LED aus
  analogWriteFreq( 18000 ); // default 1k

  Serial.begin( 9600 );
  Serial.println( "ESP Gestartet" );
  setFanSpeed(100); // power on fan so it will say hello

  EEPROM.begin( sizeof(psettings) );
  EEPROM.get(0, psettings);
  
  if ((psettings.maxTemp  > 200) ||
      (psettings.maxTemp != psettings.maxTemp)) {   // chech if _maxTemp == nan
    // probybly not initialized eeprom value so initilize    
    Serial.println("Initialize Settings...");
    psettings.maxTemp = 0.f;    
    strcpy(psettings.hostname,WiFi.hostname().c_str());
  }
  Serial.print("hostname: ");
  Serial.println(psettings.hostname);
  WiFi.begin(SSID, PWD);

  Serial.print( "Verbindung wird hergestellt ..." );
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  WiFi.hostname( psettings.hostname );

  Serial.print( "Verbunden! IP-Adresse: " );
  Serial.println( WiFi.localIP() );

  _server.on("/", sendWebPage);
  _server.on("/led_set", setLedState);
  _server.on("/ledState_get", sendLedState);
  _server.on("/modeAuto_set", setModeAuto);
  _server.on("/modeAuto_get", sendModeAuto);
  _server.on("/maxTemp_get", sendMaxTemp);
  _server.on("/temperature_get", sendTemperature);  
  _server.on("/wattage_get", sensor_data);
  _server.on("/fanSpeed_get", sendFanSpeed);
  _server.on("/fanSpeed_set", setFanSpeedWeb);
  _server.on("/hostname_get", apiGetHostname);
  _server.on("/hostname_set", apiSetHostname);
  

  _server.on("/favicon.ico", sendFavicon);
  
  _server.begin();  

  // Enable OTA update
  ArduinoOTA.begin();

  // Current Sensor Reading SCT013
  emon1.current(current1Pin, 0.8);  // Pin und Kalibrierung
}

void loop() {    
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    if (_modeAuto) {
      handleTempUpdate();
    }

    // Check for over the air update request and (if present) flash it
    ArduinoOTA.handle();
  }
  
  _server.handleClient();  
  
  // double irms = emon1.calcIrms(1480);
  // irmsValues[currentReadingIdx] = irms;
  // currentReadingIdx++;
  // if ( currentReadingIdx >= 10 ) {
  //   currentReadingIdx = 0;
  // }
  // double irmsMean = 0;
  // for ( int i = 0; i < IrmsArraySize; i++ ) {
  //   irmsMean = irmsMean + irmsValues[i];
  // }
  // irmsMean = irmsMean / IrmsArraySize;

  // Serial.print( irmsMean*Spannung );
  // Serial.println( " Watt  -  " );

  // Serial.println("_rpm:" + String(getFan_rpm()));                      // Ausgabe der Drehzahl im Seriellen Monitor  
}