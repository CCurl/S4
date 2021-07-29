// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

typedef unsigned char byte;
#define MAX_FUNC (36*26)

extern void vmInit(int code_sz, int mem_sz, int num_funcs);
extern int run(int pc);
extern void dumpStack(int hdr);
extern void setCodeByte(int addr, char ch);
extern long getRegister(int reg);
extern int getFunctionAddress(const char* fname);
extern void printString(const char*);
extern void printStringF(const char* fmt, ...);

extern byte isBye;
extern FILE* input_fp;

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
#else
#include <Arduino.h>
#endif // _WIN32
