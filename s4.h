#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    void printStringF(const char*, ...);
    #define INPUT            0
    #define INPUT_PULLUP     1
    #define OUTPUT           2
    #define STK_SZ          63
    #define MEM_SZ    (1024*16)
    #define CODE_SZ   (1024*64)
    #define TIB_SZ         128
    #define NUM_FUNCS   (36*36)
    #define __PC__
#else
    #include <Arduino.h>
    #define mySerial SerialUSB
    #define STK_SZ          31
    #define MEM_SZ    (1024*20)
    #define CODE_SZ       (512)
    #define TIB_SZ          80
    #define NUM_FUNCS   (10*36)
    #define __DEV_BOARD__
    int _getch();
    void printString(const char* str);
    int doFile(int pc);
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define NUM_REGS     26
#define TIB         (CODE_SZ-TIB_SZ-4)

#define CODE  memory.code
#define MEM   memory.mem

typedef unsigned char byte;
typedef struct {
    byte code[CODE_SZ];
    long mem[MEM_SZ];
} MEMORY_T;
