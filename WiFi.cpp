#include "s4.h"

#if __WIFI__ == 0
void wifiStart() {}
void printWifi(const char* str) {}
int wifiCharAvailable() { return 0; }
char wifiGetChar() { return 0; }
void feedWatchDog() {}
#endif

#if __WIFI__ == 1
#if __BOARD__ == ESP8266
#include "ESP8266.h"
#elif __BOARD__ == ESP32_DEV
#include "ESP32.h"
#endif
#endif // __WIFI__
