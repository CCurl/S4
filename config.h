#ifndef __CONFIG__
#define __CONFIG__

#ifdef _WIN32
  #define __WINDOWS__
  #define  _CRT_SECURE_NO_WARNINGS
  #include <Windows.h>
  #include <conio.h>
  #define __PC__
  // For Windows 64-bit, use 'long long'
  // #define S4CELL long long
#endif

#ifdef __LINUX__
  #include <time.h>
  #define __PC__
#endif

#ifdef __PC__
  #define STK_SZ          16
  #define LSTACK_SZ       8
  #define USER_SZ        (256*1024)
  #define NUM_REGS       (26*26*26)
  #define NUM_FUNCS      (26*26*26)
#else
  /* Dev Board */
  #include <Arduino.h>
  #define __ESP8266__
  #define NTWK            "FiOS-T01SJ"
  #define NTPW            "marie2eric5936side"
  #define MYSSID          "ESP-8266-1"
  #define MYSSPW          "simplePW"
  #define _NEEDS_ALIGN_   1
  #define STK_SZ          8
  #define LSTACK_SZ       4
  #define USER_SZ        (22*1024)
  #define NUM_REGS       (26*26)
  #define NUM_FUNCS      (26*26)
  #define __SERIAL__      1
  #define mySerial        Serial
  #define ILED            13
#endif

#endif
