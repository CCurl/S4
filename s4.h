// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

typedef unsigned char byte;
#define MAX_FUNC (2600)
#define MAX_CODE (64*1024)
#define MEM_SZ   (MEM_SZB/4)
typedef unsigned short addr;

extern void vmInit();
extern addr run(addr pc);
extern void dumpStack(int hdr);
extern void setCodeByte(addr loc, char ch);
extern long registerVal(int reg);
extern addr functionAddress(int fn);
extern void printString(const char*);
extern void printStringF(const char* fmt, ...);

#ifdef _WIN32
#define __PC__
#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 2
extern long millis();
extern int analogRead(int pin);
extern void analogWrite(int pin, int val);
extern int digitalRead(int pin);
extern void digitalWrite(int pin, int val);
extern void pinMode(int pin, int mode);
extern void delay(unsigned long ms);
extern FILE* input_fp;
extern byte isBye;
#define CODE_SZ      MAX_CODE
#define MEM_SZB      (256*1024)
#define NUM_FUNCS     MAX_FUNC
#else
#define _DEV_BOARD_
#define __SERIAL__
#include <Arduino.h>
#define PICO 1
#define XIAO 0
#if PICO
#define CODE_SZ      MAX_CODE   // PICO
#define MEM_SZB      (96*1024)  // PICO
#define NUM_FUNCS    MAX_FUNC   // PICO
#define ILED          25        // PICO
#elif XIAO
#define CODE_SZ      (12*1024)  // XIAO
#define MEM_SZB      (12*1024)  // XIAO
#define NUM_FUNCS    MAX_FUNC   // XIAO
#define ILED          13        // XIAO
#endif
#endif // _WIN32
