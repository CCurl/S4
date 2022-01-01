#include "s4.h"
#ifndef __ESP8266__
void wifiConnect() {}
void createAccessPoint() {}
#else
#include <ESP8266WiFi.h>

void wifiConnect() {
    printString("\r\nWIFI Connecting ...");
    WiFi.begin("FiOS-T01SJ", "marie2eric5936side");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      printString(".");
    }
    printString("\r\nConnected, IP address: ");
    mySerial.print(WiFi.localIP());
}

void createAccessPoint() {
}

#endif
