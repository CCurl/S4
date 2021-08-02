// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "s4.h"

#define STK_SZ 31

struct {
    long dsp, rsp;
    byte code[CODE_SZ];
    long mem[MEM_SZ];
    int func[NUM_FUNCS];
    long dstack[STK_SZ + 1];
    long rstack[STK_SZ + 1];
} sys;

byte isBye = 0, isError = 0;
char buf[100];
FILE* input_fp = NULL;
byte* bMem;

#define CODE       sys.code
#define MEM        sys.mem
#define FUNC       sys.func


#define T        sys.dstack[sys.dsp]
#define N        sys.dstack[sys.dsp-1]
#define R        sys.rstack[sys.rsp]
#define HERE     MEM[7]

void push(long v) { if (sys.dsp < STK_SZ) { sys.dstack[++sys.dsp] = v; } }
long pop() { return (sys.dsp > 0) ? sys.dstack[sys.dsp--] : 0; }

void rpush(long v) { if (sys.rsp < STK_SZ) { sys.rstack[++sys.rsp] = v; } }
long rpop() { return (sys.rsp > 0) ? sys.rstack[sys.rsp--] : -1; }

void vmReset() {
    sys.dsp = sys.rsp = 0;
    for (int i = 0; i < CODE_SZ; i++) { CODE[i] = 0; }
    for (int i = 0; i < MEM_SZ; i++) { MEM[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { FUNC[i] = 0; }
    MEM['C' - 'A'] = CODE_SZ;
    MEM['D' - 'A'] = (long)&sys.code[0];
    MEM['F' - 'A'] = (long)&sys.func[0];
    MEM['M' - 'A'] = (long)&sys.mem[0];
    MEM['N' - 'A'] = NUM_FUNCS;
    MEM['R' - 'A'] = (long)&sys.rstack[0];
    MEM['S' - 'A'] = (long)&sys.dstack[0];
    MEM['Y' - 'A'] = (long)&sys;
    MEM['Z' - 'A'] = MEM_SZ;
}

void vmInit(int code_sz, int mem_sz, int num_funcs) {
    bMem = (byte*)&sys.mem[0];
    vmReset();
}

void printStringF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printString(buf);
}

int hexNum(char x) {
    if (('0' <= x) && (x <= '9')) { return x - '0'; }
    if (('A' <= x) && (x <= 'F')) { return x - 'A' + 10; }
    if (('a' <= x) && (x <= 'f')) { return x - 'a' + 10; }
    return -1;
}

int funcNum(char x, int alpha) {
    if (('A' <= x) && (x <= 'Z')) { return x - 'A'; }
    if (('a' <= x) && (x <= 'z')) { return x - 'a' + 26; }
    if (('0' <= x) && (x <= '9') && (alpha)) { return x - '0' + 52; }
    isError = 1;
    return -1;
}

int GetFunctionNum(int pc) {
    int f1 = funcNum(CODE[pc], 0);
    int f2 = funcNum(CODE[pc + 1], 1);
    if ((f1 < 0) || (f2 < 0)) {
        printStringF("-%c%c:FN Bad-", CODE[pc], CODE[pc + 1]);
        return -1;
    }
    int fn = (f1 * 62) + f2;
    if ((fn < 0) || (NUM_FUNCS <= fn)) {
        printStringF("-%c%c:FN OOB-", CODE[pc], CODE[pc + 1]);
        isError = 1;
        return -1;
    }
    return fn;
}

int doDefineFunction(int pc) {
    if (pc < HERE) { return pc; }
    int fn = GetFunctionNum(pc);
    if (fn < 0) { return pc + 2; }
    CODE[HERE++] = '{';
    CODE[HERE++] = CODE[pc];
    CODE[HERE++] = CODE[pc + 1];
    pc += 2;
    FUNC[fn] = HERE;
    while ((pc < CODE_SZ) && CODE[pc]) {
        CODE[HERE++] = CODE[pc++];
        if (CODE[HERE - 1] == '}') { return pc; }
    }
    printString("-overflow-");
    return pc;
}

int doCallFunction(int pc) {
    int fn = GetFunctionNum(pc);
    if (fn < 0) { return pc + 2; }
    if (!FUNC[fn]) { return pc + 2; }
    rpush(pc + 2);
    return FUNC[fn];
}

void dumpCode() {
    printStringF("\r\nCODE: size: %d bytes, HERE=%d", CODE_SZ, HERE);
    if (HERE == 0) { printString("\r\n(no code defined)"); return; }
    int ti = 0, x = HERE, npl = 20;
    char txt[32];
    for (int i = 0; i < HERE; i++) {
        if ((i % npl) == 0) {
            if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); ti = 0; }
            printStringF("\n\r%05d: ", i);
        }
        txt[ti++] = (CODE[i] < 32) ? '.' : CODE[i];
        printStringF(" %3d", CODE[i]);
    }
    while (x % npl) {
        printString("    ");
        x++;
    }
    if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); }
}

void dumpFuncs() {
    printStringF("\r\nFUNCTIONS: (%d available)", NUM_FUNCS);
    int n = 0;
    for (int i = 0; i < NUM_FUNCS; i++) {
        if (FUNC[i]) {
            byte f1, f2, x = (i / 62);
            f1 = 'A' + x;
            if (26 <= x) { f1 = 'a' + (x-26); }
            x = (i % 62);
            f2 = 'A' + x;
            if (26 <= x) { f2 = 'a' + (x-26); }
            if (52 <= x) { f2 = '0' + (x-52); }
            if (((n++) % 6) == 0) { printString("\r\n"); }
            printStringF("%c%c:%4d (%d)    ", f1, f2, FUNC[i], i);
        }
    }
}

void dumpStack(int hdr) {
    if (hdr) { printStringF("\r\nSTACK: size: %d ", STK_SZ); }
    printString("(");
    for (int i = 1; i <= sys.dsp; i++) { printStringF("%s%ld", (i > 1 ? " " : ""), sys.dstack[i]); }
    printString(")");
}

void dumpMemory(int isRegs) {
    int c = 0, n = isRegs ? 26 : MEM_SZ;
    if (isRegs) { printStringF("\r\nREGISTERS: "); }
    else { printStringF("\r\nMEMORY: size: %d bytes (%d longs)", MEM_SZ * 4, MEM_SZ); }
    for (int i = 0; i < n; i++) {
        long x = MEM[i];
        if ((!x) && (!isRegs)) { continue; }
        if (((c++) % 6) == 0) { printString("\r\n"); }
        if (isRegs) { printStringF("%c: %-10ld  ", i + 'A', x); }
        else { printStringF("[%05d]: %-10ld  ", i, x); }
    }
    if (c == 0) { printString("\r\n(all memory empty)"); }
}

void dumpAll() {
    dumpStack(1);   printString("\r\n");
    dumpMemory(1);  printString("\r\n");
    dumpCode();     printString("\r\n");
    dumpFuncs();    printString("\r\n");
}

int doFile(int pc) {
    int ir = CODE[pc++];
    switch (ir) {
#ifdef __PC__
    case 'C':
        if (T) { fclose((FILE*)T); }
        pop();
        break;
    case 'O': {
        char* md = (char*)bMem + pop();
        char* fn = (char*)bMem + T;
        T = 0;
        fopen_s((FILE**)&T, fn, md);
    }
            break;
    case 'R': if (T) {
        long n = fread_s(buf, 2, 1, 1, (FILE*)T);
        T = ((n) ? buf[0] : 0);
        push(n);
    }
            break;
    case 'W': if (T) {
        FILE* fh = (FILE*)pop();
        buf[1] = 0;
        buf[0] = (byte)pop();
        fwrite(buf, 1, 1, fh);
    }
            break;
#endif
    case 'N':
        push(0);
        ir = GetFunctionNum(pc);
        if (0 <= ir) { T = FUNC[ir]; }
        pc += 2;
        break;
    }
    return pc;
}

int doPin(int pc) {
    int ir = CODE[pc++];
    long pin = pop(), val = 0;
    switch (ir) {
    case 'I': pinMode(pin, INPUT); break;
    case 'U': pinMode(pin, INPUT_PULLUP); break;
    case 'O': pinMode(pin, OUTPUT); break;
    case 'R': ir = CODE[pc++];
        if (ir == 'D') { push(digitalRead(pin)); }
        if (ir == 'A') { push(analogRead(pin)); }
        break;
    case 'W': ir = CODE[pc++]; val = pop();
        if (ir == 'D') { digitalWrite(pin, val); }
        if (ir == 'A') { analogWrite(pin, val); }
        break;
    }
    return pc;
}

int doExt(int pc) {
    byte ir = CODE[pc++];
    switch (ir) {
    case 'A': rpush(pc); pc = pop();    break;
    case 'F': pc = doFile(pc);          break;
    case 'J': pc = pop();               break;
    case 'X': vmReset();                break;
    case 'P': pc = doPin(pc);           break;
    case 'T': isBye = 1;                break;
    }
    return pc;
}


int step(int pc) {
    byte ir = CODE[pc++];
    long t1, t2;
    switch (ir) {
    case 0: return -1;                                  // 0
    case ' ': while (CODE[pc] == ' ') { pc++; }         // 32
        break;
    case '!': t2 = pop(); t1 = pop();                   // 33
        if ((0 <= t2) && (t2 < MEM_SZ)) { MEM[t2] = t1; }
        break;
    case '"': buf[1] = 0;                          // 34
        while ((pc < CODE_SZ) && (CODE[pc] != '"')) {
            buf[0] = CODE[pc++];
            printString(buf);
        }
        ++pc; break;
    case '#': push(T);               break;             // 35 (DUP)
    case '$': t1 = N; N = T; T = t1; break;             // 36 (SWAP)
    case '%': push(N);               break;             // 37 (OVER)
    case '\\': pop();                break;             // 92 (DROP)
    case '&': t1 = pop(); T &= t1;   break;             // 38
    case '\'': push(CODE[pc++]);     break;             // 39
    case '(': if (pop() == 0) {                         // 40
        while ((pc < CODE_SZ) && (CODE[pc] != ')')) { ++pc; }
        ++pc;
    }
            break;
    case ')': /*maybe ELSE?*/        break;             // 41
    case '*': t1 = pop(); T *= t1;   break;             // 42
    case '+': N += T; pop();         break;             // 43
    case ',': printStringF("%c", (char)pop());  break;  // 44
    case '-': N -= T; pop();         break;             // 45
    case '.': printStringF("%ld", pop());      break;   // 46
    case '/': t1 = pop(); if (t1) { T /= t1; } break;   // 47
    case '0': case '1': case '2': case '3': case '4':   // 48-57
    case '5': case '6': case '7': case '8': case '9':
        push(ir - '0');
        t1 = CODE[pc] - '0';
        while ((0 <= t1) && (t1 <= 9)) {
            T = (T * 10) + t1;
            t1 = CODE[++pc] - '0';
        }
        break;
    case ':': pc = doCallFunction(pc); break;           // 58
    case ';': pc = rpop(); break;                       // 59
    case '<': t1 = pop(); T = T < t1 ? -1 : 0;  break;  // 60
    case '=': t1 = pop(); T = T == t1 ? -1 : 0; break;  // 61
    case '>': t1 = pop(); T = T > t1 ? -1 : 0;  break;  // 62
    // case '?': push(_getch());                   break;  // 63
    case '@': if ((0 <= T) && (T < MEM_SZ)) { T = MEM[T]; }
            break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': ir -= 'A';
        push(MEM[ir]); t1 = CODE[pc];
        if (t1 == '+') { ++pc; ++MEM[ir]; }
        if (t1 == '-') { ++pc; --MEM[ir]; }
        if (t1 == ';') { pop(); ++pc; MEM[ir] = pop(); }
        break;
    case '[': rpush(pc);                                // 91
        if (T == 0) {
            while ((pc < CODE_SZ) && (CODE[pc] != ']')) { pc++; }
        }
        break;
    case ']': if (T) { pc = R; }                        // 93
            else { pop();  rpop(); }
            break;
    case '^': t1 = pop(); T ^= t1;      break;          // 94
    case '_': t1 = T;                                   // 95
        while (CODE[pc] && CODE[pc] != '_') { bMem[t1++] = CODE[pc++]; }
        ++pc; bMem[t1++] = 0; T = t1;
        break;
    case '`': /* FREE*/                 break;          // 96
    case 'b': printString(" ");         break;
    case 'c': ir = CODE[pc++];
        t1 = pop();
        if ((0 <= t1) && (t1 < (MEM_SZ * 4))) {
            if (ir == '@') { push(bMem[t1]); }
            if (ir == '!') { bMem[t1] = (byte)pop(); }
        }
        break;
    case 'd': ir = CODE[pc++];
        t1 = pop();
        if ((0 <= t1) && (t1 < CODE_SZ)) {
            if (ir == '@') { push(CODE[t1]); }
            if (ir == '!') { CODE[t1] = (byte)pop(); }
        }
        break;
    case 'h': push(0);
        t1 = hexNum(CODE[pc]);
        while (0 <= t1) {
            T = (T * 0x10) + t1;
            t1 = hexNum(CODE[++pc]);
        }
        break;
    case 'i': t1 = CODE[pc++];
        if (t1 == 'A') { dumpAll(); }
        if (t1 == 'C') { dumpCode(); }
        if (t1 == 'F') { dumpFuncs(); }
        if (t1 == 'M') { dumpMemory(0); }
        if (t1 == 'R') { dumpMemory(1); }
        if (t1 == 'S') { dumpStack(0); }
        break;
    case 'l':
        t1 = pop();
#ifdef __PC__
        if (input_fp) { fclose(input_fp); }
        sprintf_s(buf, sizeof(buf), "block.%03ld", t1);
        fopen_s(&input_fp, buf, "rt");
#else
        printString("-l:pc only-");
#endif
        break;
    case 'm': ir = CODE[pc++]; {
        byte* bp = (byte*)T;
        if (ir == '@') { T = *bp; }
        if (ir == '!') { t1 = pop(); t2 = pop(); bp[t1] = (byte)t2; }
    } break;
    case 'n': printString("\r\n");  break;
    case 'r': ir = CODE[pc++]; {
        if (ir == '<') { rpush(pop()); }
        if (ir == '@') { push(R); }
        if (ir == '>') { push(rpop()); }
    } break;
    case 't': push(millis());       break;
    case 'w': delay(pop());         break;
    case 'x': pc = doExt(pc);       break;
    case '{': pc = doDefineFunction(pc); break;         // 123
    case '|': t1 = pop(); T |= t1; break;               // 124
    case '}': pc = rpop(); break;                       // 125
    case '~': T = ~T; break;                            // 126
    }
    return pc;
}

int run(int pc) {
    isError = 0;
    while (sys.rsp >= 0) {
        if ((pc < 0) || (CODE_SZ <= pc)) { return pc; }
        pc = step(pc);
        if (isError) { break; }
    }
    return pc;
}

void setCodeByte(int addr, char ch) {
    if ((0 <= addr) && (addr < CODE_SZ)) { CODE[addr] = ch; }
}

long getRegister(int reg) {
    if ((0 <= 'A') && (reg <= 'Z')) { return MEM[reg - 'A']; }
    return 0;
}

int getFunctionAddress(const char* fname) {
    int pc = getRegister('H') + 2;
    CODE[pc] = fname[0];
    CODE[pc+1] = fname[0];
    int fn = GetFunctionNum(pc);
    return (fn < 0) ? fn : FUNC[fn];
}
