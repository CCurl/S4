// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

#include "config.h"

#define MAX_REGS  (26*26*26)
#define MAX_FUNCS (26*26*26)
typedef unsigned char byte;
#if USER_SZ <= 0x10000
typedef unsigned short addr;
#else
typedef unsigned long addr;
#endif
#define CELL long
#define UCELL unsigned long

extern void vmInit();
extern addr run(addr pc);
extern void dumpStack(int hdr);
extern void setCodeByte(addr loc, char ch);
extern void printChar(const char ch);
extern void printString(const char*);
extern void printStringF(const char* fmt, ...);
extern addr functionAddress(const char* fn);
extern int getChar();
extern int charAvailable();
extern void input_push(FILE* fp);
extern FILE *input_pop();
extern FILE *input_fp;

#if __PC__
	#define INPUT 0
	#define INPUT_PULLUP 1
	#define OUTPUT 2
	extern int  analogRead(int pin);
	extern void analogWrite(int pin, int val);
	extern void delay(unsigned long ms);
	extern int  digitalRead(int pin);
	extern void digitalWrite(int pin, int val);
	extern void pinMode(int pin, int mode);
	extern long millis();
	extern byte isBye, isError;
#endif // __PC__
