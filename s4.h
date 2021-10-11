// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

typedef unsigned char byte;
#define MAX_REGS  (26*26*26)
#define MAX_FUNCS (26*26*26)
typedef unsigned long addr;

extern void vmInit();
extern addr run(addr pc);
extern void dumpStack(int hdr);
extern void setCodeByte(addr loc, char ch);
extern long registerVal(int reg);
extern addr functionAddress(int fn);
extern void printChar(const char ch);
extern void printString(const char*);
extern void printStringF(const char* fmt, ...);
extern int getChar();
extern int charAvailable();

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
//#define USER_SZ       (24*1024)
//#define NUM_FUNCS     (26*2)
//#define NUM_REGS      (26*2)
#define USER_SZ       (512*1024)
#define NUM_FUNCS     (MAX_FUNCS)
#define NUM_REGS      (MAX_REGS)
#else
#define _DEV_BOARD_
#define __SERIAL__
#include <Arduino.h>
#define PICO 1
#define XIAO 0
#if PICO
#define USER_SZ      (64*1024)
#define NUM_FUNCS    (26*26)
#define NUM_REGS     (26*26)
#define ILED          25        // PICO
#elif XIAO
#define USER_SZ      (24*1024)
#define NUM_FUNCS    (26*2)
#define NUM_REGS     (26*2)
#define ILED          13        // XIAO
#endif
#endif // _WIN32
