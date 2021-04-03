// S4 - a stack VM, inspired by Sandor Schneider's STABLE
// see https://w3group.de/stable.html

#ifndef _WIN32
#define __DEV_BOARD__
#endif

void printStringF(const char *fmt, ...);

#ifdef __DEV_BOARD__
    #include <Arduino.h>
    // **NOTE** tweak these for your target dev board
    // These would be for a board like the Teensy 4.0
    #define mySerial SerialUSB
    #define CODE_SZ   (1024*32)
    #define STK_SZ          63
    #define NUM_VARS        32
    #define TIB_SZ          96
    #define NUM_FUNCS   (52*52)
#else
    #include <windows.h>
    #include <conio.h>
    long millis() { return GetTickCount(); }
    int analogRead(int pin) { printStringF("-AR(%d)-", pin); return 0; }
    void analogWrite(int pin, int val) { printStringF("-AW(%d,%d)-", pin, val); }
    int digitalRead(int pin) { printStringF("-DR(%d)-", pin); return 0;  }
    void digitalWrite(int pin, int val) { printStringF("-DW(%d,%d)-", pin, val); }
    void pinMode(int pin, int mode) { printStringF("-pinMode(%d%d)-", pin, mode); }
    void delay(DWORD ms) { Sleep(ms); }
    #define INPUT 0
    #define OUTPUT 0
    #define CODE_SZ   (1024*64)
    #define STK_SZ          63
    #define NUM_VARS  (32*1024)
    #define TIB_SZ         128
    #define NUM_FUNCS   (52*52)
    HANDLE hStdOut = 0;
    char input_fn[24];
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define NUM_REGS     26
#define TIB         (CODE_SZ-TIB_SZ-4)

typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char byte;

byte code[CODE_SZ];
long   dstack[STK_SZ + 1];
ushort rstack[STK_SZ + 1];
ushort dsp, rsp;
long reg[NUM_REGS];
ushort func[NUM_FUNCS];
long var[NUM_VARS];
ushort here = 0;
ushort curReg = 0;
byte isBye = 0;

#define T dstack[dsp]
#define N dstack[dsp-1]

void push(long v) { if (dsp < STK_SZ) { dstack[++dsp] = v; } }
long pop() { return (dsp > 0) ? dstack[dsp--] : 0; }

void rpush(ushort v) { if (rsp < STK_SZ) { rstack[++rsp] = v; } }
ushort rpop() { return (rsp > 0) ? rstack[rsp--] : -1; }

#ifdef __DEV_BOARD__
int _getch() { return (mySerial.available()) ? mySerial.read() : 0; }
void printString(const char* str) { mySerial.print(str); }
#else
void printString(const char* str) {
    int l = strlen(str);
    DWORD n = 0;
    if (l) { WriteConsoleA(hStdOut, str, l, &n, 0); }
}
#endif

void vmInit() {
    dsp = rsp = here = curReg = 0;
    for (int i = 0; i < CODE_SZ; i++) { code[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { func[i] = 0; }
    for (int i = 0; i < NUM_REGS; i++) { reg[i] = 0; }
    for (int i = 0; i < NUM_VARS; i++) { var[i] = 0; }
    printString("S4 - v0.0.1 - Chris Curl\r\nHello.");
}

void printStringF(const char* fmt, ...) {
    char buf[100];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printString(buf);
}

int digit(byte c)  { return (('0' <= c) && (c <= '9')) ? (c - '0') : -1; }
int alphaL(byte c) { return (('a' <= c) && (c <= 'z')) ? (c - 'a') : -1; }
int alphaU(byte c) { return (('A' <= c) && (c <= 'Z')) ? (c - 'A') : -1; }

int alpha(byte c) {
    int x = alphaU(c); if (0 <= x) return x;
    x     = alphaL(c); if (0 <= x) return x + 26;
    return -1;
}

int number(int pc) {
    long num = 0;
    while (('0' <= code[pc]) && (code[pc] <= '9')) {
        num = (num * 10) + (code[pc] - '0');
        pc++;
    }
    push(num);
    return pc;
}

int getFN(int pc) {
    int fL = alpha(code[pc]);
    int fH = alpha(code[pc + 1]);
    if ((0 <= fL) && (0 <= fH)) {
        int fn = ((fH * 52) + fL);
        if ((0 <= fn) && (fn < NUM_FUNCS)) { return fn; }
    }
    return -1;
}

int defineFunc(int pc) {
    int fn = getFN(pc);
    int v = (0 <= fn) ? 1 : 0;
    if (v) {
        code[here++] = '{';
        code[here++] = code[pc];
        code[here++] = code[pc + 1];
        func[fn] = here;
    }
    else { printStringF("-invalid function '%c%c' (0:%d)-", code[pc], code[pc + 1], NUM_FUNCS); }
    pc += 2;
    v = (v && (here < pc)) ? 1 : 0;
    while ((pc < CODE_SZ) && code[pc]) {
        if (v) { code[here++] = code[pc]; }
        if (code[pc] == '}') { break; }
        pc++;
    }
    return pc + 1;
}

int doFunc(int pc) {
    int fn = getFN(pc);
    pc += 2;
    if ((0 <= fn) && (func[fn])) {
        rpush(pc);
        pc = func[fn];
    }
    return pc;
}

void dumpCode() {
    printStringF("\r\nCODE: size: %d, HERE: %d", CODE_SZ, here);
    if (here == 0) { printString("\r\n(no code defined)"); return; }
    int ti = 0, x = here; 
    char* txt = (char*)&code[here + 10];
    for (int i = 0; i < here; i++) {
        if ((i % 20) == 0) {
            if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); ti = 0; }
            printStringF("\n\r%04d: ", i);
        }
        txt[ti++] = (code[i] < 32) ? '.' : code[i];
        printStringF(" %3d", code[i]);
    }
    while (x % 20) {
        printString("    ");
        x++;
    }
    if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); }
}

void dumpFuncs() {
    printStringF("\r\nFUNCTIONS: size: %d", NUM_FUNCS);
    int n = 0;
    for (int i = 0; i < NUM_FUNCS; i++) {
        int a = func[i];
        if (a == 0) { continue; }
        if ((0 < n) && (n % 5)) { printStringF("    "); }
        else { printString("\r\n"); }
        printStringF("%c%c: %-4d", code[a-2], code[a-1], (int)a);
        ++n;
    }
    if (n == 0) { printString("\r\n(no functions defined)"); }
}

void dumpRegs() {
    printString("\r\nREGISTERS:");
    for (int i = 0; i < NUM_REGS; i++) {
        byte fId = 'a' + i;
        if ((0 < i) && (i % 5)) { printStringF("    "); }
        else { printString("\r\n"); }
        printStringF("%c: %-10ld", fId, reg[i]);
    }
}

void dumpStack(int hdr) {
    if (hdr) { printStringF("\r\nSTACK: size: %d ", STK_SZ); }
    printString("(");
    for (int i = 1; i <= dsp; i++) { printStringF("%s%ld", (i>1?" ":""), dstack[i]); }
    printString(")");
}

void dumpVars() {
    int n = 0;
    printStringF("\r\nMEMORY, size=%d", NUM_VARS);
    for (int i = 0; i < NUM_VARS; i++) {
        if (var[i] == 0) { continue; }
        if ((0 < n) && (n % 5)) { printStringF("    "); }
        else { printString("\r\n"); }
        printStringF("[%04d]: %-10ld", i, var[i]);
        ++n;
    }
    if (n == 0) { printString("\r\n(all memory empty)"); }
}

void dumpAll() {
    dumpStack(1);  printString("\r\n");
    dumpRegs();    printString("\r\n");
    dumpFuncs();   printString("\r\n");
    dumpVars();    printString("\r\n");
    dumpCode();    printString("\r\n");
}

int run(int pc) {
    long t1 = 0, t2 = 0;
    while (rsp >= 0) {
        if ((pc < 0) || (CODE_SZ <= pc)) { return 0; }
        byte ir = code[pc++];
        // printStringF("\npc:%04d, ir:%03d [%c] ", pc - 1, ir, ir); dumpStack(0);
        switch (ir) {
        case 0: return pc;
        case ' ': break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            pc = number(pc - 1); break;
        case '{': pc = defineFunc(pc); break;
        case '}': pc = rpop(); break;
        case '#': push(T); break;
        case '$': break;
        case '\\': pop(); break;
        case '\'': break;
        case '`': break;
        case ':': pc = doFunc(pc); break;
        case ';': break;
        case '@': push(reg[curReg]); break;
        case '!': reg[curReg] = pop(); break;
        case '?': break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            curReg = ir - 'a';
            if (code[pc] == '+') { ++pc; reg[curReg]++; }
            if (code[pc] == '-') { ++pc; reg[curReg]--; }
            break;
        case '+': if (code[pc] == '+') { pc++; T++; }
                else { if (dsp > 1) { t1 = pop(); T += t1; } } break;
        case '-': if (code[pc] == '-') { pc++; T--; }
                else { if (dsp > 1) { t1 = pop(); T -= t1; } } break;
        case '*': if (dsp > 1) { t1 = pop(); T *= t1; } break;
        case '/': if (dsp > 1) { t1 = pop(); if (t1) T /= t1; } break;
        case '%': if (dsp > 1) { t1 = pop(); T %= t1; } break;
        case '|': if (dsp > 1) { t1 = pop(); T |= t1; } break;
        case '&': if (dsp > 1) { t1 = pop(); T &= t1; } break;
        case '_': if (dsp > 0) { T = -T; } break;
        case '~': if (dsp > 0) { T = ~T; } break;
        case '.': printStringF("%ld", pop());  break;
        case ',': printStringF("%c", (char)pop());  break;
        case '"': while ((code[pc] != '"') && (pc < CODE_SZ)) { printStringF("%c", code[pc]); pc++; } pc++; break;
        case '=': t1 = (dsp > 1) ? pop() : 0; T = (T == t1) ? -1 : 0; break;
        case '<': t1 = (dsp > 1) ? pop() : 0; T = (T < t1)  ? -1 : 0;  break;
        case '>': t1 = (dsp > 1) ? pop() : 0; T = (T > t1)  ? -1 : 0;  break;
        case '^': push(_getch()); break;
        case '[': rpush(pc); if (T == 0) { while ((pc < CODE_SZ) && (code[pc] != ']')) { pc++; } } break;
        case ']': if (pop()) { pc = rstack[rsp]; } else { rpop(); } break;
        case '(': if (pop() == 0) { while ((pc < CODE_SZ) && (code[pc] != ')')) { pc++; } } break;
        case 'A': t1 = code[pc++];
            if (t1 == 'R') { T = analogRead(T); }
            if (t1 == 'W') { t2 = pop(); t1 = pop(); analogWrite(t2, t1); }
            break;
        case 'B': printString(" "); break;
        case 'C': t1 = code[pc++];
            if (t1 == '@') { if ((0 <= T) && (T < CODE_SZ)) { T = code[T]; } }
            if (t1 == '!') { t1 = pop(); t2 = pop(); if ((0 <= t1) && (t1 < CODE_SZ)) { code[t1] = (byte)t2; } }
            break;
        case 'D': t1 = code[pc++];
            if (t1 == 'R') { T = digitalRead(T); }
            if (t1 == 'W') { t2 = pop(); t1 = pop(); digitalWrite(t2, t1); }
          break;
        case 'E': case 'F': case 'G': break;
        case 'H': t1 = code[pc++];
            if (t1 == '@') { push(here); }
            if (t1 == '!') { t2 = pop(); if ((0 < t2) && (t2 < CODE_SZ)) { here = (ushort)t2; } }
            break;
        case 'I': t1 = code[pc++];
            if (t1 == 'A') { dumpAll(); }
            if (t1 == 'C') { dumpCode(); }
            if (t1 == 'F') { dumpFuncs(); }
            if (t1 == 'R') { dumpRegs(); }
            if (t1 == 'S') { dumpStack(1); }
            if (t1 == 'V') { dumpVars(); }
            break;
        case 'J':  break;
        case 'K': T *= 1000; break;
        case 'L': break;
        case 'M': t1 = code[pc++];
            if (t1 == '@') { if ((0 <= T) && (T < NUM_VARS)) { T = var[T]; } }
            if (t1 == '!') { t2 = pop(); t1 = pop(); if ((0 <= t2) && (t2 < NUM_VARS)) { var[t2] = t1; } }
            break;
        case 'N': break;
        case 'O': push(N); break;
        case 'P': t1 = code[pc++]; t2 = pop();
            if (t1 == 'I') { pinMode(t2, INPUT); }
            if (t1 == 'O') { pinMode(t2, OUTPUT); }
            printStringF("-pinMode(%d, %c)-", t2, t1);
            break;
        case 'Q': break;
        case 'R': printString("\r\n"); break;
        case 'S': t1 = pop(); t2 = pop(); push(t1); push(t2);  break;
        case 'T': push(millis()); break;
        case 'U': break;
        case 'V': break;
        case 'W': delay(pop()); break;
        case 'X': t1 = code[pc++]; if (t1 == 'X') { vmInit(); } break;
        case 'Y': break;
        case 'Z': isBye = code[pc];
        default: break;
        }
    }
    return 0;
}

void s4() {
    printString("\r\ns4:"); dumpStack(0);
    printString("> ");
}

void loadCode(const char* src) {
    char* tgt = (char*)&code[TIB];
    while (*src) { *(tgt++) = *(src++); }
    *tgt = 0;
    run(TIB);
}

#ifdef __DEV_BOARD__
#define iLed 13
ushort ihere = 0;
ulong nextBlink = 0;
int ledState = 0;

void setup() {
    mySerial.begin(19200);
    while (!mySerial) {}
    while (mySerial.available()) {}
    vmInit();
    ihere = TIB;
    pinMode(iLed, OUTPUT);
    // ********************************************
    // * HERE is where you load your default code *
    // ********************************************
    // loadCode("IA");
    s4();
}

void loop() {
    ulong curTm = millis();
    if (nextBlink < curTm) {
        ledState = (ledState == LOW) ? HIGH : LOW;
        digitalWrite(iLed, ledState);
        nextBlink = curTm + 777;
    }

    while (mySerial.available()) {
        char c = mySerial.read();
        if (c == 13) {
            code[ihere] = (char)0;
            printString(" ");
            run(TIB);
            s4();
            ihere = TIB;
        }
        else {
            if (ihere < CODE_SZ) {
                code[ihere++] = c;
                char b[2]; b[0] = c; b[1] = 0;
                printString(b);
            }
        }
    }
    if (reg[25]) { run(reg[25]); }    // autorun
}

#else
void loop() {
    char* tib = (char*)&code[TIB];
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
            char* tib = (char*)&code[TIB];
            while (fgets(tib, TIB_SZ, fp) == tib) { run(TIB); }
            fclose(fp);
        }
    }

    while (isBye == 0) { loop(); }
}
#endif
