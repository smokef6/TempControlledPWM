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

float setpoint = 25.5;   // Diese Variable Deklaration in den Haupttab vor "setup()" verschieben um sie im gesamten Sketch verfügbar zu machen.

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Variablen
float temperatureC;
int fanSpeed;            // Variable für die Lüftergeschwindigkeit
int rpm = 0;                   // Variable für die gemittelte Drehzahl
float maxTemp;                 // EEProm value for maximum measured Temperature
bool modeAuto = true;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(PIN_TEMP_SENSOR);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature tempSensors(&oneWire);

ESP8266WebServer server(80);



// int getFanRpm()
// {
//   float rps = 0;                 // Variable mit Kommastelle für die Berechnung der Umdrehungen pro Sekunde
//   float umdrZeit = 0;            // Variable mit Kommastelle für die Zeit pro Umdrehung des Lüfters
//   float flankenZeit =0;          // Variable mit Kommastelle für die Zeit pro Puls des Lüfters 

//   if ( fanSpeed < 1 ) {      
//     rpm = 0;
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
//     analogWrite(PIN_FAN, fanSpeed);            // Setzt die Lüftergeschwindigkeit zurück
//     umdrZeit = ((flankenZeit * 4)/1000);      // Berechnung der Zeit pro Umdrehung in Millisekunden
//     rps = (1000/umdrZeit);                    // Umrechnung auf Umdrehungen pro Sekunde
//     rpm = (rps*60);                           // Umrechnung auf Umdrehungen pro Minute
//   }    
//   return rpm;  
// }

void setFanSpeed(int speed) {
  fanSpeed = speed;
  analogWrite(PIN_FAN, fanSpeed);
  if ( speed < 1 ) {
    digitalWrite(PIN_FAN, LOW);   //Led port ganz ausschalten        
  }
  Serial.print("fanSpeed = ");
  Serial.println( fanSpeed );
}


void get_hook() {
    if(server.hasArg("fanSpeed")){
      // Variable "name" wird übergeben
      Serial.println("Variable 'fanSpeed' übergeben!");

      // Anschließend die Prüfung, ob die Variable 'name' leer ist:      
      if ( server.arg("fanSpeed") != "" ){
        String pwm = server.arg("fanSpeed");
        modeAuto = false;
        setFanSpeed(pwm.toInt());
      } 
      
      // Wenn Variable die 'name' übergeben wurde, (leer oder nicht) ist:
      // Ausgabe im Webbrowser HTTP-Code 200: Ok
      server.send(200, "text/plain", String(fanSpeed));
    }
    if(server.hasArg("modeAuto")) {
      if ( server.arg("modeAuto") == "0" ){
        modeAuto = false;
      }
      else if ( server.arg("modeAuto") == "1" ){
        modeAuto = true;
      }
      server.send(200, "text/plain", String(modeAuto));
    }
    else if (server.hasArg("temperature")) {
      server.send(200, "text/plain", String(temperatureC));      
    }
    else{
      //Wenn gar keine Variablen übergeben wurden
      server.send(200, "text/html", "fanSpeed: " + String(fanSpeed) + "<br>" +
                                    "modeAuto: " + String(modeAuto) + "<br>" +
                                    "temperature: " + String(temperatureC) + + "&deg;C<br>" +
                                    "maxTemp: " + String(maxTemp) + "&deg;C");
    }
}


void setupModify() {
  server.on("/modified", []() {
    char buf[13];
    if (server.args()) setpoint = atof(server.arg(0).c_str());
    snprintf(buf, sizeof buf, "\"%8.3f\"", setpoint);
    server.send(200, "application/json", buf);
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
  setFanSpeed(100); // power on fan so it will say hello

  WiFi.begin(ssid, password);

  Serial.print( "Verbindung wird hergestellt ..." );
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print( "Verbunden! IP-Adresse: " );
  Serial.println( WiFi.localIP() );
  server.onNotFound([]() {  // Es wird keine Seite definiert, sodass bei jeder URL IMMER die Funktion get_hook() aufgerufen wird
    get_hook();
  });
  server.begin();  

  // Enable OTA update
  ArduinoOTA.begin();
  
  EEPROM.begin(EEPROM_SIZE);  
  EEPROM.get(0, maxTemp);
  Serial.print("maxTemp = ");
  Serial.println( maxTemp );
}

void handleTempUpdate(float temperature) {
  temperatureC = temperature;
  Serial.print(temperatureC);
  Serial.println("ºC");

  if ( maxTemp < temperatureC ) {
    maxTemp = temperatureC;
    EEPROM.put(0, maxTemp);
    EEPROM.commit();
  }

  if (modeAuto) {
    if (temperatureC > 35) { setFanSpeed(250); }
    else if (temperatureC > 32) { setFanSpeed(200); }
    else if (temperatureC > 30) { setFanSpeed(150); }
    else if (temperatureC > 25) { setFanSpeed(130); }
    else if (temperatureC > 23) { setFanSpeed(100); }
    else if (temperatureC > 23) { 
      if (fanSpeed < 50) {
        setFanSpeed(150);   // make shure fan starts rotation
        delay(2000);
      }
      setFanSpeed(50);
    }
    else { setFanSpeed(0); }
  }
}

void loop() {
  tempSensors.requestTemperatures();
  handleTempUpdate(tempSensors.getTempCByIndex(0));
  
  server.handleClient();  
  
  // Check for over the air update request and (if present) flash it
  ArduinoOTA.handle();

  delay(1000);              //1000 ms Pause
  
  // Serial.println("rpm:" + String(getFanRPM()));                      // Ausgabe der Drehzahl im Seriellen Monitor  
}