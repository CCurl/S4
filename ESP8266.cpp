#include "s4.h"
#ifndef __ESP8266__
void wifiConnect() {}
void wifiCreateAP() {}
void handleWifiClient() {}
void printWifi(const char* str) {}
int wifiAvailable() { return 0; }
char wifiRead() { return 0; }
void wifiDisableWDT();
#else
#include <ESP8266WiFi.h>

WiFiServer wifiServer(23);
WiFiClient wifiClient;
int haveClient = 0;

void wifiConnect(const char *net, const char *pw) {
    printString("\r\nConnecting to WIFI...");
    WiFi.begin(net, pw);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      printString(".");
    }
    mySerial.print(WiFi.localIP());
    printString("\r\nStarting telnet server on port 23...");
    wifiServer.begin();
    wifiServer.setNoDelay(true);
}

void createWifiServer() {
}

int wifiCharAvailable() {
    if (haveClient && !wifiClient.connected()) { 
        haveClient = 0; 
        printString("-wifi-disconnect-");
    }
    if (wifiServer.hasClient()) {
        wifiClient = wifiServer.available();
        printString("\r\n-new-wifi-client-");
        printWifi("\r\Hello.\r\n");
        haveClient = 1;
    }
    if (haveClient && wifiClient.available()) { return 1; }
    return 0;
}

char wifiGetChar() {
    while (!wifiCharAvailable()) {}
    return wifiClient.read();
}

void printWifi(const char* str) {
    if (haveClient) {
        wifiClient.write(str, strlen(str));
    }
}
#endif
