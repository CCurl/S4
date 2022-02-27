#ifndef __CONFIG__
#define __CONFIG__

#define PC          1
#define XIAO        2
#define PICO        3
#define ESP8266     4
#define ESP32_DEV   5
#define LEO         6

#define __BOARD__ XIAO

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
  #define __EDITOR__
  #define STK_SZ          16
  #define FSTK_SZ         16
  #define LSTACK_SZ        8
  #define USER_SZ        (256*1024)
  #define NUM_REGS       (26*26*26)
  #define NUM_FUNCS      (1000)
#elif __BOARD__ == XIAO
  // #define __GAMEPAD__
  #define STK_SZ          16
  #define LSTACK_SZ        4
  #define USER_SZ        (22*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (100)
#elif __BOARD__ == PICO
  #define __FILES__
  #define __LITTLEFS__
  #define __EDITOR__
  #define FSTK_SZ          4
  #define STK_SZ          16
  #define LSTACK_SZ        8
  #define USER_SZ        (128*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (1000)
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
  #define FSTK_SZ         4
  #define LSTACK_SZ       4
  #define USER_SZ        (24*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (500)
#elif __BOARD__ == ESP32_DEV
  #define __WIFI__        1
  // #define __WATCHDOG__
  #define __FILES__
  #define __LITTLEFS__
  #define NTWK            "FiOS-T01SJ"
  #define NTPW            "marie2eric5936side"
  #define MYSSID          "ESP32-1"
  #define MYSSPW          "simplePW"
  #define STK_SZ          16
  #define FSTK_SZ          4
  #define LSTACK_SZ        4
  #define USER_SZ        (64*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (1000)
#elif __BOARD__ == LEO
  #define FSTK_SZ         1
  #define STK_SZ          6
  #define LSTACK_SZ       2
  #define USER_SZ        (1*1024)
  #define NUM_REGS       (13)
  #define NUM_FUNCS      (25)
#endif

#endif
