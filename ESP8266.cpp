#include "s4.h"

#if __BOARD__ == ESP8266
#ifdef __WIFI__
#include <ESP8266WiFi.h>

    WiFiServer wifiServer(23);
    WiFiClient wifiClient;
    int haveClient = 0;

    void wifiStart() {
        printString("\r\nConnecting to WIFI...");
        WiFi.begin(NTWK, NTPW);
        while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          printString(".");
        }
        mySerial.print(WiFi.localIP());
        printString("\r\nStarting telnet server on port 23...");
        wifiServer.begin();
        wifiServer.setNoDelay(true);
        printString("started.");
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

    int wdtCount = 1;
    void feedWatchDog() {
        if (--wdtCount < 1) {
            wdtCount = 99999;
            ESP.wdtFeed();
        }
    }
#endif
#endif
