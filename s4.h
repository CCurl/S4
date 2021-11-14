// MINT - A Minimal Interpreter - for details, see https://github.com/monsonite/MINT

#include "config.h"
#include <stdio.h>
#include <stdarg.h>

#define CELL        long
#define UCELL       unsigned CELL
#define CELL_SZ     sizeof(CELL)
#define ushort      unsigned short
#define byte        unsigned char
#define addr        byte *

#define REG        sys.reg
#define USER       sys.user
#define FUNC       sys.func
#define HERE       REG[7]
#define INDEX      REG[8]
#define T          sys.dstack[sys.dsp]
#define N          sys.dstack[sys.dsp-1]
#define R          sys.rstack[sys.rsp]
#define LSP        sys.lsp
#define DROP1      pop()
#define DROP2      pop(); pop()
#define BetweenI(n, x, y) ((x <= n) && (n <= y))

typedef struct {
    addr start;
    CELL from;
    CELL to;
    addr end;
} LOOP_ENTRY_T;

typedef struct {
    ushort dsp, rsp, lsp, u1;
    CELL   reg[NUM_REGS];
    byte   user[USER_SZ];
    addr   func[NUM_FUNCS];
    CELL   dstack[STK_SZ + 1];
    addr   rstack[STK_SZ + 1];
    LOOP_ENTRY_T lstack[LSTACK_SZ+1];
} SYS_T;

extern SYS_T sys;
extern byte isBye;
extern byte isError;

extern void vmInit();
extern CELL pop();
extern void push(CELL);
extern addr run(addr);
extern addr doCustom(byte, addr);
extern void printChar(const char);
extern void printString(const char*);
extern void printStringF(const char*, ...);
extern void dumpStack();
