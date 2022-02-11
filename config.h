#ifndef __CONFIG__
#define __CONFIG__

#define PC       1
#define XIAO     2
#define PICO     3
#define ESP8266  4
#define ESP32    5

#define __BOARD__ PICO

#ifdef _WIN32
  #define __WINDOWS__
  #define  _CRT_SECURE_NO_WARNINGS
  #include <Windows.h>
  #include <conio.h>
  #undef __BOARD__
  #define __BOARD__ PC
  // For Windows 64-bit, use 'long long'
  // #define S4CELL long long
#endif

#ifdef __LINUX__
  #include <time.h>
  #undef __BOARD__
  #define __BOARD__ PC
#endif

#if __BOARD__ != PC
#include <Arduino.h>
#define _NEEDS_ALIGN_   1
#define __SERIAL__      1
#define mySerial        Serial
#endif

#if __BOARD__ == PC
  #define __FILES__
  #define STK_SZ          16
  #define LSTACK_SZ       8
  #define USER_SZ        (256*1024)
  #define NUM_REGS       (26*26*26)
  #define NUM_FUNCS      (100)
#elif __BOARD__ == XIAO
  #define STK_SZ          8
  #define LSTACK_SZ       4
  #define USER_SZ        (22*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (26*26)
#elif __BOARD__ == PICO
  #define __FILES__
  #define __LITTLEFS__
  #define STK_SZ          16
  #define LSTACK_SZ       8
  #define USER_SZ        (96*1024)
  #define NUM_REGS       (26*26*26)
  #define NUM_FUNCS      (26*26)
#elif __BOARD__ == ESP8266
  #define __WIFI__        1
  #define __WATCHDOG__
  #define __FILES__
  #define __LITTLEFS__
  #define NTWK            "FiOS-T01SJ"
  #define NTPW            "marie2eric5936side"
  #define MYSSID          "ESP-8266-1"
  #define MYSSPW          "simplePW"
  #define STK_SZ          8
  #define LSTACK_SZ       4
  #define USER_SZ        (24*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (26*26)
#else
  #define STK_SZ          8
  #define LSTACK_SZ       2
  #define USER_SZ        (1*1024)
  #define NUM_REGS       (261)
  #define NUM_FUNCS      (26)
#endif

#endif
