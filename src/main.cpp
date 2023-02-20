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

#define PIN_FAN D8  //nur Falls du externe LED z.B. an D7 (GPIO13)
                // über ca. 330 Ohm Widerstand angeschlossen hast auskommentieren
#define PIN_MOSFET_GATE D6      
#define PIN_TACHO D4

// GPIO where the DS18B20 is connected to
const int oneWireBus = D2;
const int pwmRange = 1023;
float setpoint = 25.5;   // Diese Variable Deklaration in den Haupttab vor "setup()" verschieben um sie im gesamten Sketch verfügbar zu machen.
ESP8266WebServer server(80);
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
float temperatureC;


// Variablen
int fanSpeed = 160;            // Variable für die Lüftergeschwindigkeit
int rpm = 0;                   // Variable für die gemittelte Drehzahl

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature tempSensors(&oneWire);


void setupModify() {
  server.on("/modified", []() {
    char buf[13];
    if (server.args()) setpoint = atof(server.arg(0).c_str());
    snprintf(buf, sizeof buf, "\"%8.3f\"", setpoint);
    server.send(200, "application/json", buf);
  });
}

int getFanRpm()
{
  float rps = 0;                 // Variable mit Kommastelle für die Berechnung der Umdrehungen pro Sekunde
  float umdrZeit = 0;            // Variable mit Kommastelle für die Zeit pro Umdrehung des Lüfters
  float flankenZeit =0;          // Variable mit Kommastelle für die Zeit pro Puls des Lüfters 

  if ( fanSpeed < 1 ) {      
    rpm = 0;
  }
  else {
    analogWrite(PIN_FAN, 255);                 // Den Lüfter konstant mit Strom versorgen damit das Tachosignal funktioniert
    delay(20);
    for ( int retryCount=0; retryCount<2; retryCount++ ) {            
      flankenZeit = pulseIn(PIN_TACHO, HIGH);    // Abfrage der Zeit pro Puls in Mikrosekunden    
      if (flankenZeit>100) {
        break;
      }      
    }      
    analogWrite(PIN_FAN, fanSpeed);            // Setzt die Lüftergeschwindigkeit zurück
    umdrZeit = ((flankenZeit * 4)/1000);      // Berechnung der Zeit pro Umdrehung in Millisekunden
    rps = (1000/umdrZeit);                    // Umrechnung auf Umdrehungen pro Sekunde
    rpm = (rps*60);                           // Umrechnung auf Umdrehungen pro Minute
  }    
  return rpm;  
}

void get_hook() {
    if(server.hasArg("fanSpeed")){
      // Variable "name" wird übergeben
      Serial.println("Variable 'fanSpeed' übergeben!");

      // Anschließend die Prüfung, ob die Variable 'name' leer ist:      
      if ( server.arg("fanSpeed") != "" ){
        String pwm = server.arg("fanSpeed");
        fanSpeed = pwm.toInt();

        analogWrite(PIN_FAN, fanSpeed);
        if ( pwm.toInt() < 1 ) {
          digitalWrite(PIN_FAN, LOW);   //Led port ganz ausschalten        
        }

        // Falls Variable 'name' nicht leer ist:
        Serial.println(fanSpeed);
        
        // Ausgabe im Webbrowser HTTP-Code 200: Ok
        server.send(200, "text/plain", String(fanSpeed));
      } else {        
        // Wenn Variable die 'name' übergeben wurde, aber leer ist:
        // Ausgabe im Webbrowser HTTP-Code 200: Ok
        server.send(200, "text/plain", String(fanSpeed));
      }
    }     
    else if (server.hasArg("temperature")) {
      server.send(200, "text/plain", String(temperatureC));      
    }
    else if (server.hasArg("roundsPerMinute")) {
      server.send(200, "text/plain", String(getFanRpm()));      
    }
    else{
      //Wenn gar keine Variablen übergeben wurden
      server.send(200, "text/html", "fanSpeed: " + String(fanSpeed) + "<br>" +
                                    "temperature: " + String(temperatureC) + "&deg;C" + "<br>" +
                                    "roundsPerMinute: " + String(getFanRpm()) + "U/min");
    }
}


void setup() {
    tempSensors.begin();

  pinMode(PIN_FAN, OUTPUT);  // Port aus Ausgang schalten
  pinMode(PIN_MOSFET_GATE, OUTPUT);  // Port aus Ausgang schalten
  pinMode(PIN_TACHO, INPUT);
  analogWriteFreq( 15000 ); // default 1k

  Serial.begin(9600);
  Serial.println("ESP Gestartet");

  WiFi.begin(ssid, password);

  Serial.print("Verbindung wird hergestellt ...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Verbunden! IP-Adresse: ");
  Serial.println(WiFi.localIP());
  server.onNotFound([]() {  // Es wird keine Seite definiert, sodass bei jeder URL IMMER die Funktion get_hook() aufgerufen wird
    get_hook();
  });
  server.begin();

  fanSpeed = 150;
  analogWrite(PIN_FAN, fanSpeed);

  // Enable OTA update
  ArduinoOTA.begin();
}

void loop() {
  tempSensors.requestTemperatures();
  temperatureC = tempSensors.getTempCByIndex(0);
  Serial.print(temperatureC);
  Serial.println("ºC");
  
  server.handleClient();  
  
  // Check for over the air update request and (if present) flash it
  ArduinoOTA.handle();

  delay(1000);              //1000 ms Pause
  
  // Serial.println("rpm:" + String(getFanRPM()));                      // Ausgabe der Drehzahl im Seriellen Monitor  
}