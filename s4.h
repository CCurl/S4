// S4 - A Minimal Interpreter

#include "config.h"
#include <stdio.h>
#include <stdarg.h>

#ifndef S4CELL
#define S4CELL long
#endif

typedef S4CELL           CELL;
typedef unsigned S4CELL  UCELL;
typedef unsigned short   ushort;
typedef unsigned char    byte;
typedef byte*            addr;
#define CELL_SZ          sizeof(CELL)

#define REG              sys.reg
#define USER             sys.user
#define FUNC             sys.func
#define DSP              sys.dsp
#define RSP              sys.rsp
#define LSP              sys.lsp
#define INDEX            REG[8]
#define TOS              sys.dstack[DSP]
#define N                sys.dstack[DSP-1]
#define DROP1            pop()
#define DROP2            pop(); pop()
#define LTOS             (&sys.lstack[LSP])
#define BetweenI(n, x, y) ((x <= n) && (n <= y))

typedef struct {
    addr start;
    CELL from;
    CELL to;
    addr end;
} LOOP_ENTRY_T;

typedef struct {
    ushort dsp, rsp, lsp, u1;
    CELL   dstack[STK_SZ + 1];
    addr   rstack[STK_SZ + 1];
    LOOP_ENTRY_T lstack[LSTACK_SZ + 1];
    addr   func[NUM_FUNCS];
    CELL   reg[NUM_REGS];
    byte   user[USER_SZ];
} SYS_T;

extern SYS_T sys;
extern byte isBye;
extern byte isError;
extern addr HERE;

extern void vmInit();
extern CELL pop();
extern void push(CELL);
extern addr run(addr);
extern addr doCustom(byte, addr);
extern void printChar(const char);
extern void printString(const char*);
extern void printStringF(const char*, ...);
extern void dumpStack();
extern CELL getSeed();
extern int charAvailable();
extern int getChar();

// Wifi stuff
extern void wifiStart();
extern int wifiCharAvailable();
extern char wifiGetChar();
extern void printWifi(const char* str);
extern void feedWatchDog();
