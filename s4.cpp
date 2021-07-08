// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

#ifndef _WIN32
#define __DEV_BOARD__
#endif

#ifdef __DEV_BOARD__
#include <Arduino.h>
#define mySerial SerialUSB
#define STK_SZ          31
#define MEM_SZ     (1024*6)
#define TIB_SZ         150
int _getch() { return (mySerial.available()) ? mySerial.read() : 0; }
void printString(const char* str) { mySerial.print(str); }
int doFileOpen(int pc, const char* x) { return pc; }
int doFileClose(int pc) { return pc; }
int doFileRead(int pc) { return pc; }
int doFileWrite(int pc) { return pc; }
#else
#include <windows.h>
#include <conio.h>
void printStringF(const char*, ...);
long millis() { return GetTickCount(); }
int analogRead(int pin) { printStringF("-AR(%d)-", pin); return 0; }
void analogWrite(int pin, int val) { printStringF("-AW(%d,%d)-", pin, val); }
int digitalRead(int pin) { printStringF("-DR(%d)-", pin); return 0; }
void digitalWrite(int pin, int val) { printStringF("-DW(%d,%d)-", pin, val); }
void pinMode(int pin, int mode) { printStringF("-pinMode(%d,%d)-", pin, mode); }
void delay(DWORD ms) { Sleep(ms); }
#define INPUT            0
#define INPUT_PULLUP     1
#define OUTPUT           2
#define STK_SZ          63
#define MEM_SZ    (1024*64)
#define TIB_SZ         128
HANDLE hStdOut = 0;
char input_fn[24];
void printString(const char* str) {
    int l = strlen(str);
    DWORD n = 0;
    if (l) { WriteConsoleA(hStdOut, str, l, &n, 0); }
}
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CODE_SZ     (MEM_SZ*4)
#define NUM_REGS     26
#define TIB         (CODE_SZ-TIB_SZ-4)

typedef unsigned char byte;

long dstack[STK_SZ + 1];
long rstack[STK_SZ + 1];
long dsp, rsp;
long reg[NUM_REGS];
long func[NUM_REGS];
long here  = 0;
long curReg = 0;
byte isBye = 0;

union {
    byte code[CODE_SZ];
    long mem[MEM_SZ];
} memory;

#define T   dstack[dsp]
#define N   dstack[dsp-1]
#define R   rstack[rsp]

void push(long v) { if (dsp < STK_SZ) { dstack[++dsp] = v; } }
long pop() { return (dsp > 0) ? dstack[dsp--] : 0; }

void rpush(long v) { if (rsp < STK_SZ) { rstack[++rsp] = v; } }
long rpop() { return (rsp > 0) ? rstack[rsp--] : -1; }

void vmInit() {
    dsp = rsp = here = curReg = 0;
    for (int i = 0; i < NUM_REGS; i++) { reg[i] = 0; }
    for (int i = 0; i < NUM_REGS; i++) { func[i] = 0; }
    for (int i = 0; i < MEM_SZ; i++) { memory.mem[i] = 0; }
}

void printStringF(const char* fmt, ...) {
    char buf[100];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printString(buf);
}

int doNumber(int pc) {
    long num = 0;
    char c = memory.code[pc];
    while (('0' <= c) && (c <= '9')) {
        num = (num * 10) + (c - '0');
        c = memory.code[++pc];
    }
    push(num);
    return pc;
}

int doIf(int pc) {
    if (pop() == 0) {
        while ((pc < CODE_SZ) && (memory.code[pc] != ')')) { ++pc; }
        ++pc;
    }
    return pc;
}

int doDefineFunction(int pc) {
    if (pc < here) { return pc; }
    int fn = memory.code[pc] - 'A';
    if ((fn < 0) || (NUM_REGS <= fn)) { return pc; }
    memory.code[here++] = '{';
    memory.code[here++] = memory.code[pc++];
    func[fn] = here;
    while ((pc < CODE_SZ) && memory.code[pc]) {
        memory.code[here++] = memory.code[pc++];
        if (memory.code[here-1] == '}') { return pc; }
    }
    printString("-code-overflow-");
    return pc;
}

#ifndef __DEV_BOARD__
int doFileOpen(int pc, const char* mode) {
    char buf[24];
    long blk = T;
    FILE* fh = 0;
    sprintf_s<24>(buf, "block.%d", blk);
    fopen_s(&fh, buf, mode);
    T = (long)fh;
    return pc;
}

int doFileClose(int pc) {
    FILE* fh = (FILE *)pop();
    if (fh) { fclose(fh); }
    return pc;
}

int doFileRead(int pc) {
    char buf[2];
    FILE* fh = (FILE*)pop();
    if (fh) {
        SIZE_T n = fread_s(buf, 2, 1, 1, fh);
        push((n) ? buf[0] : 0);
    }
    return pc;
}

int doFileWrite(int pc) {
    char buf[2];
    FILE* fh = (FILE*)pop();
    buf[0] = (byte)pop();
    if (fh) { fwrite(buf, 1, 1, fh); }
    return pc;
}
#endif

int doQuote(int pc, int isPush) {
    char x[2];
    x[1] = 0;
    while ((pc < CODE_SZ) && (memory.code[pc] != '"')) {
        x[0] = memory.code[pc++];
        printString(x);
    }
    return ++pc;
}

int doLoop(int pc) {
    rpush(pc);
    if (T == 0) {
        while ((pc < CODE_SZ) && (memory.code[pc] != ']')) { pc++; }
    }
    return pc;
}

void dumpCode() {
    printStringF("\r\nCODE: size: %d ($%x), HERE=%d ($%x)", CODE_SZ, CODE_SZ, here, here);
    if (here == 0) { printString("\r\n(no code defined)"); return; }
    int ti = 0, x = here, npl = 20;
    char* txt = (char*)&memory.code[here + 10];
    for (int i = 0; i < here; i++) {
        if ((i % npl) == 0) {
            if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); ti = 0; }
            printStringF("\n\r%05d: ", i);
        }
        txt[ti++] = (memory.code[i] < 32) ? '.' : memory.code[i];
        printStringF(" %3d", memory.code[i]);
    }
    while (x % npl) {
        printString("    ");
        x++;
    }
    if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); }
}

void dumpRegs() {
    printString("\r\nREGISTERS:");
    for (int i = 0; i < NUM_REGS; i++) {
        byte fId = 'a' + i;
        if ((i % 5) == 0) { printString("\r\n"); }
        printStringF("%c: %-15ld", fId, reg[i]);
    }
}

void dumpFuncs() {
    printString("\r\nFUNCTIONS:");
    for (int i = 0; i < NUM_REGS; i++) {
        byte fId = 'A' + i;
        if ((i % 7) == 0) { printString("\r\n"); }
        printStringF("%c: %-5ld    ", fId, func[i]);
    }
}

void dumpStack(int hdr) {
    if (hdr) { printStringF("\r\nSTACK: size: %d ", STK_SZ); }
    printString("(");
    for (int i = 1; i <= dsp; i++) { printStringF("%s%ld", (i > 1 ? " " : ""), dstack[i]); }
    printString(")");
}

void dumpMemory() {
    int n = 0;
    printStringF("\r\nMEMORY: size: %d (%d bytes)\r\n", MEM_SZ, CODE_SZ);
    for (int i = 0; i < MEM_SZ; i++) {
        if (memory.mem[i] == 0) { continue; }
        long x = memory.mem[i];
        printStringF("[%04d]: %-10ld (", i, x);
        for (int j = 0; j < 4; j++) {
            if (0<j) { printString(","); }
            printStringF("%d", x&255);
            x /= 256;
        }
        printStringF(")\r\n");
        ++n;
    }
    if (n == 0) { printString("(all memory empty)"); }
}

void dumpAll() {
    dumpStack(1);  printString("\r\n");
    dumpRegs();    printString("\r\n");
    dumpCode();    printString("\r\n");
    dumpFuncs();   printString("\r\n");
}

int doPin(int pc) {
    int ir = memory.code[pc++];
    long pin = pop(), val = 0;
    switch (ir) {
    case 'I': pinMode(pin, INPUT); break;
    case 'U': pinMode(pin, INPUT_PULLUP); break;
    case 'O': pinMode(pin, OUTPUT); break;
    case 'R': ir = memory.code[pc++];
        if (ir == 'D') { push(digitalRead(pin)); }
        if (ir == 'A') { push(analogRead(pin)); }
        break;
    case 'W': ir = memory.code[pc++]; val = pop();
        if (ir == 'D') { digitalWrite(pin, val); }
        if (ir == 'A') { analogWrite(pin, val); }
        break;
    }
    return pc;
}

int doExt(int pc) {
    byte ir = memory.code[pc++];
    long t1, t2;
    switch (ir) {
    case 'A': break;   /* *** FREE ***  */
    case 'B': break;   /* *** FREE ***  */
    case 'C': t1 = memory.code[pc++];
        if (t1 == '@') { if ((0 <= T) && (T < CODE_SZ)) { T = memory.code[T]; } }
        if (t1 == '!') { t1 = pop(); t2 = pop(); if ((0 <= t1) && (t1 < CODE_SZ)) { memory.code[t1] = (byte)t2; } }
        break;
    case 'D': break;   /* *** FREE ***  */
    case 'E': break;   /* *** FREE ***  */
    case 'F': t1 = memory.code[pc++];
        if (t1 == 'O') { pc = doFileOpen(pc, "rb"); }
        if (t1 == 'N') { pc = doFileOpen(pc, "wb"); }
        if (t1 == 'C') { pc = doFileClose(pc); }
        if (t1 == 'R') { pc = doFileRead(pc); }
        if (t1 == 'W') { pc = doFileWrite(pc); }
        if (t1 == 'F') { push(0); }
        if (t1 == 'T') { push(-1); }
        break;
    case 'G': break;   /* *** FREE ***  */
    case 'H': t1 = memory.code[pc++];
        if (t1 == '@') { push(here); }
        if (t1 == '!') { t2 = pop(); if ((0 < t2) && (t2 < CODE_SZ)) { here = (long)t2; } }
        break;
    case 'I': t1 = memory.code[pc++];
        if (t1 == 'A') { dumpAll(); }
        if (t1 == 'C') { dumpCode(); }
        if (t1 == 'F') { dumpFuncs(); }
        if (t1 == 'M') { dumpMemory(); }
        if (t1 == 'R') { dumpRegs(); }
        if (t1 == 'S') { dumpStack(1); }
        break;
    case 'J': break;   /* *** FREE ***  */
    case 'K': T *= 1000; break;
    case 'L': break;   /* *** FREE ***  */
    case 'M': t1 = memory.code[pc++];
        if (t1 == '@') { if ((0 <= T) && (T < MEM_SZ)) { T = memory.mem[T]; } }
        if (t1 == '!') { t2 = pop(); t1 = pop(); if ((0 <= t2) && (t2 < MEM_SZ)) { memory.mem[t2] = t1; } }
        break;
    case 'N': N = T; pop(); break; // NIP
    case 'O': push(N);      break; // OVER
    case 'P': pc = doPin(pc); break;
    case 'Q': break;   /* *** FREE ***  */
    case 'R': break;   /* *** FREE ***  */
    case 'S': break;   /* *** FREE ***  */
    case 'T': push(millis()); break;
    case 'U': break;   /* *** FREE ***  */
    case 'V': break;   /* *** FREE ***  */
    case 'W': delay(pop()); break;
    case 'X': t1 = memory.code[pc++];
        if (t1 == 'A') { rpush(pc); pc = pop(); }
        if (t1 == 'X') { vmInit(); }
        break;
    case 'Y': break;   /* *** FREE ***  */
    case 'Z': isBye = (memory.code[pc++] == 'Z'); break;
    default: break;
    }
    return pc;
}

int step(int pc) {
    byte ir = memory.code[pc++];
    long t1;
    switch (ir) {
    case 0: return -1;                                  // 0
    case ' ': break;                                    // 32
    case '!': reg[curReg] = pop();   break;             // 33
    case '"': pc = doQuote(pc, 0);   break;             // 34
    case '#': push(T);               break;             // 35
    case '$': t1 = N; N = T; T = t1; break;             // 36
    case '%': t1 = pop(); T %= t1;   break;             // 37
    case '&': t1 = pop(); T &= t1;   break;             // 38
    case '\'': push(memory.code[pc++]); break;          // 39
    case '(': pc = doIf(pc);         break;             // 40
    case ')': /*maybe ELSE?*/        break;             // 41
    case '*': t1 = pop(); T *= t1;   break;             // 42
    case '+': N += T; pop();         break;             // 43
    case ',': printStringF("%c", (char)pop());  break;  // 44
    case '-': N -= T; pop();         break;             // 45
    case '.': printStringF("%ld", pop());      break;   // 46
    case '/': t1 = pop(); if (t1) { T /= t1; } break;   // 47
    case '0': case '1': case '2': case '3': case '4':   // 48-57
    case '5': case '6': case '7': case '8': case '9':
        pc = doNumber(pc - 1); break;
    case ':': pc = doExt(pc); break;                    // 58
    case ';': pc = rpop(); break;                       // 59
    case '<': t1 = pop(); T = T < t1 ? -1 : 0;  break;  // 60
    case '=': t1 = pop(); T = T == t1 ? -1 : 0; break;  // 61
    case '>': t1 = pop(); T = T > t1 ? -1 : 0;  break;  // 62
    case '?': push(_getch());                   break;  // 63
    case '@': push(reg[curReg]);                break;  // 64
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': // 65-90
    case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
    case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
    case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z': 
        t1 = ir - 'A';
        if (func[t1]) { rpush(pc); pc = func[t1]; }
        break;
    case '[': pc = doLoop(pc); break;                   // 91
    case '\\': pop(); break;                            // 92
    case ']': if (T) { pc = R; }                        // 93
            else { pop();  rpop(); }
            break;
    case '^': t1 = pop(); T ^= t1;  break;              // 94
    case '_': T = -T;      break;                       // 95
    case '`': break;  /* *** FREE ***  */               // 96
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': // 97-122
    case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
    case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
    case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z': curReg = ir - 'a'; t1 = memory.code[pc];
        if (t1 == '+') { ++pc; ++reg[curReg]; }
        if (t1 == '-') { ++pc; --reg[curReg]; }
        break;
    case '{': pc = doDefineFunction(pc); break;         // 123
    case '|': t1 = pop(); T |= t1; break;               // 124
    case '}': pc = rpop(); break;                       // 125
    case '~': T = ~T; break;                            // 126
    }
    return pc;
}

int run(int pc) {
    while (rsp >= 0) {
        if ((pc < 0) || (CODE_SZ <= pc)) { return pc; }
        pc = step(pc);
    }
    return pc;
}

void s4() {
    printString("\r\ns4:"); dumpStack(0);
    printString("> ");
}

void loadCode(const char* src) {
    char* tgt = (char*)&memory.code[TIB];
    while (*src) { *(tgt++) = *(src++); }
    *tgt = 0;
    run(TIB);
}

#ifndef __DEV_BOARD__
void loop() {
    char* tib = (char*)&memory.code[TIB];
    s4();
    fgets(tib, TIB_SZ, stdin);
    run(TIB);
}

void process_arg(char* arg)
{
    if ((*arg == 'i') && (*(arg + 1) == ':')) {
        arg = arg + 2;
        strcpy_s(input_fn, sizeof(input_fn), arg);
    }
    else if (*arg == '?') {
        printString("usage s4 [args] [source-file]\n");
        printString("  -i:file\n");
        printString("  -? - Prints this message\n");
        exit(0);
    }
    else { printf("unknown arg '-%s'\n", arg); }
}

int main(int argc, char** argv) {
    hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m; GetConsoleMode(hStdOut, &m);
    SetConsoleMode(hStdOut, (m | ENABLE_VIRTUAL_TERMINAL_PROCESSING));
    vmInit();
    strcpy_s(input_fn, sizeof(input_fn), "");

    for (int i = 1; i < argc; i++)
    {
        char* cp = argv[i];
        if (*cp == '-') { process_arg(++cp); }
        else { strcpy_s(input_fn, sizeof(input_fn), cp); }
    }

    if (strlen(input_fn) > 0) {
        FILE* fp = NULL;
        fopen_s(&fp, input_fn, "rt");
        if (fp) {
            char* tib = (char*)&memory.code[TIB];
            while (fgets(tib, TIB_SZ, fp) == tib) { run(TIB); }
            fclose(fp);
        }
    }

    while (isBye == 0) { loop(); }
}
#endif
