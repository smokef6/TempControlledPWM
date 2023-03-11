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

#define PIN_TEMP_SENSOR D2  // GPIO where the DS18B20 is connected to
#define PIN_TACHO D4
#define PIN_MOSFET_GATE D6
#define PIN_FAN D8
#define EEPROM_SIZE 12

const char* SSID = WIFI_SSID;
const char* PWD = WIFI_PASSWORD;

// Variablen
float _temperatureC;
int _fanSpeed;            // Variable für die Lüftergeschwindigkeit
int _rpm = 0;                   // Variable für die gemittelte Drehzahl
float _maxTemp;                 // EEProm value for maximum measured Temperature
bool _modeAuto = true;
float _setpoint;   // Diese Variable Deklaration in den Haupttab vor "setup()" verschieben um sie im gesamten Sketch verfügbar zu machen.

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

void set_fanSpeed(int speed) {
  _fanSpeed = speed;
  analogWrite(PIN_FAN, _fanSpeed);
  if ( speed < 1 ) {
    digitalWrite(PIN_FAN, LOW);   //Led port ganz ausschalten        
  }
  Serial.print("_fanSpeed = ");
  Serial.println( _fanSpeed );
}

void handleTempUpdate() {
  tempSensors.requestTemperatures();  
  float temperature = tempSensors.getTempCByIndex(0);

  _temperatureC = temperature;
  Serial.print(_temperatureC);
  Serial.println("ºC");

  if ( _maxTemp < _temperatureC ) {
    _maxTemp = _temperatureC;
    EEPROM.put(0, _maxTemp);
    EEPROM.commit();
  }

  if (_modeAuto) {
    if (_temperatureC > 35) { set_fanSpeed(250); }
    else if (_temperatureC > 32) { set_fanSpeed(200); }
    else if (_temperatureC > 30) { set_fanSpeed(150); }
    else if (_temperatureC > 25) { set_fanSpeed(130); }
    else if (_temperatureC > 23) { set_fanSpeed(100); }
    else if (_temperatureC > 23) { 
      if (_fanSpeed < 50) {
        set_fanSpeed(150);   // make shure fan starts rotation
        delay(2000);
      }
      set_fanSpeed(50);
    }
    else if ( fabs(_temperatureC + 127.0f) < 0.0001 ) {
      _modeAuto = 0;
    }
    else { set_fanSpeed(0); }
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
        set_fanSpeed(pwm.toInt());
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
      _server.send(200, "text/html", "fanSpeed: " + String(_fanSpeed) + "<br>" +
                                    "modeAuto: " + String(_modeAuto) + "<br>" +
                                    "temperature: " + String(_temperatureC) + + "&deg;C<br>" +
                                    "maxTemp: " + String(_maxTemp) + "&deg;C");
    }
}


void setupModify() {
  _server.on("/modified", []() {
    char buf[13];
    if (_server.args()) _setpoint = atof(_server.arg(0).c_str());
    snprintf(buf, sizeof buf, "\"%8.3f\"", _setpoint);
    _server.send(200, "application/json", buf);
  });
}


void setup() {
  tempSensors.begin();

  pinMode(PIN_FAN, OUTPUT);  // Port aus Ausgang schalten
  pinMode(PIN_MOSFET_GATE, OUTPUT);  // Port aus Ausgang schalten
  pinMode(PIN_TACHO, INPUT);
  analogWriteFreq( 18000 ); // default 1k

  Serial.begin( 9600 );
  Serial.println( "ESP Gestartet" );
  set_fanSpeed(100); // power on fan so it will say hello

  WiFi.begin(SSID, PWD);

  Serial.print( "Verbindung wird hergestellt ..." );
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print( "Verbunden! IP-Adresse: " );
  Serial.println( WiFi.localIP() );
  _server.onNotFound([]() {  // Es wird keine Seite definiert, sodass bei jeder URL IMMER die Funktion get_hook() aufgerufen wird
    get_hook();
  });
  _server.begin();  

  // Enable OTA update
  ArduinoOTA.begin();
  
  EEPROM.begin(EEPROM_SIZE);  
  EEPROM.get(0, _maxTemp);
  if (_maxTemp  > 200) {
    // probybly not initialized eeprom value so initilize    
    _maxTemp = 0.f;
    EEPROM.put(0, _maxTemp);
  } 
  Serial.print("maxTemp = ");
  Serial.println( _maxTemp );
}

void loop() {    
  if (_modeAuto) {
    handleTempUpdate();
  }
  
  _server.handleClient();  
  
  // Check for over the air update request and (if present) flash it
  ArduinoOTA.handle();

  delay(1000);              //1000 ms Pause
  
  // Serial.println("_rpm:" + String(getFan_rpm()));                      // Ausgabe der Drehzahl im Seriellen Monitor  
}