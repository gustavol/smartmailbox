#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "../include/credentials.h"


// Descomente para ligar o debug
#define DEBUG

// Pinos utilizados
int         sensorFull = 4;                

// Opções diversas
float       batCal = 0.012295;               // Calibracao medicao da bateria
int         maxConTime = 10;                 // Timeout para se conectar ao wifi

// Inicio do codigo

// Definicoes de variaveis e objetos
char              Buffer[1024];
float             batterylevel = 0.0;
int               sensorstate = 0;

WiFiClient client;
void sendNotification(String message);

void setup() {
    sensorstate = digitalRead(sensorFull);

    rst_info    *resetInfo    = ESP.getResetInfoPtr();
    String      lastReset     = ESP.getResetReason(); 
    
    // Inicialização do Hardware

    ESP.wdtDisable();

    // Algumas strings para debug
    #ifdef DEBUG
      Serial.begin(9600);
      Serial.setTimeout(1000);
      Serial.println("\r\n\r\n\r\n");

      Serial.print("Reset reason: ");
      Serial.print("#" + String((*resetInfo).reason));
      Serial.println(" - " + lastReset);  
    #endif
  
    // Se o reboot foi causado por "power-up" (0)
    // Determina o alcance maximo.
    if ((*resetInfo).reason == 0) { 
        sprintf(Buffer,"Dispositivo energizado! \r\nMotivo: %s", lastReset.c_str());        
        #ifdef DEBUG
          Serial.println(Buffer);
        #endif 
        sendNotification(Buffer);
    }
    // Se o reboot for diferente do Sleep Wake-up (5)
    else if ((*resetInfo).reason != 5) { 
        sprintf(Buffer,"Dispositivo re-incializado! \r\nMotivo: %s", lastReset.c_str());
        #ifdef DEBUG
          Serial.println(Buffer);
        #endif 
        sendNotification(Buffer);
    }

    if (sensorstate) {           
      #ifdef DEBUG
        Serial.println("\r\n\r\n--> Full Mailbox!\r\n");
      #endif 
      sendNotification("Chegaram cartas!! :)");
    } else {
      #ifdef DEBUG
        Serial.println("\r\n\r\n--> Empty Mailbox!\r\n");
      #endif 
      sendNotification("As cartas foram removidas!");
    }
    // Wifi conectado? desconecta!
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
        Serial.println("- WiFi disconnected");
    }
   
    // Sleep!
    Serial.println("- Going to sleep mode.. \r\n\r\n");
    ESP.deepSleep(0);      
}

void sendNotification(String message){

    int wifiRetry = 0;
    
    #ifdef DEBUG
      Serial.print("- connecting to SID: " + String(ssid));
    #endif
    
    WiFi.begin(ssid, password);
    while ((wifiRetry <= (maxConTime*2)) && (WiFi.status() != WL_CONNECTED)) {
        delay(500);
        #ifdef DEBUG
          Serial.print(".");
        #endif
        wifiRetry++;
    }

    if (wifiRetry > (maxConTime*2)) { 
      #ifdef DEBUG
        Serial.println("\r\n\r\n- failed to connect ");
        Serial.println("- Going to sleep mode.. \r\n\r\n");
      #endif
      ESP.deepSleep(0);   
    }
    
    // Le o nivel da bateria.
    batterylevel = analogRead(A0) * batCal;
    
    #ifdef DEBUG
      Serial.println();
      Serial.println("- succesfully connected");
      Serial.println("- starting client");
      Serial.print("- battery level: ");
      Serial.println(batterylevel);  
      Serial.println("- connecting to pushing server...");
    #endif
        
    // IMPush    
    if (client.connect(IMPushHost, 80)) {
        #ifdef DEBUG
          Serial.println("- succesfully connected");
        #endif 
        message += "\r\n(Bat: ";
        message += String(batterylevel);
        message += "v / Sinal: ";
        message += String(WiFi.RSSI());
        message += "dBm)";      
        
        #ifdef DEBUG
          Serial.println("- sending data...");
        #endif
        
        // IMPush

        String postStr;
        //postStr += "title="; postStr += "";
        postStr += "&body=";  postStr += String(message);
        postStr += "&deviceId=";  postStr += String(IMPushDestToken);

        client.print("POST /message HTTP/1.1\n");
        client.print("Host: ");     client.print(IMPushHost);     client.print("\n");
        client.print("apiKey: ");   client.print(IMPushApiKey);   client.print("\n");
        client.print("apiToken: "); client.print(IMPushApiToken); client.print("\n");
        client.print("Connection: close\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: "); client.print(postStr.length());
        client.print("\n\n");

        client.print(postStr);
        
        #ifdef DEBUG
          Serial.println("- push notification sent:");
          Serial.println(String(message));
          Serial.println("--------------------------");
        #endif
    }
    
    client.stop();
    #ifdef DEBUG
      Serial.println("- stopping the client");  
    #endif
}
void loop() {}