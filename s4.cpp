// https://w3group.de/stable.html
//
// A big thanks to to Sandor Schneider for this.
// This is my personal reverse-engineered implementation.

#ifndef _WIN32
    #define __DEV_BOARD__
#endif

#ifdef __DEV_BOARD__
    #include <Arduino.h>
    #define mySerial SerialUSB
#else
    #include <windows.h>
    #include <conio.h>
    long millis() { return GetTickCount(); }
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define CODE_SZ  1024
#define STK_SZ     63
#define NUM_VARS   32
#define NUM_FUNCS  52
#define NUM_REGS   26

typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char byte;

long   dstack[STK_SZ + 1];
ushort rstack[STK_SZ + 1];
ushort dsp, rsp;

#define T dstack[dsp]
#define N dstack[dsp-1]

#define IHERE_INIT    (CODE_SZ-100)
#define IHERE_SZ       96

char input_fn[24];
long reg[NUM_REGS];
ushort func[NUM_FUNCS];
long var[NUM_VARS];
byte code[CODE_SZ];
ushort here = 0;
ushort ihere = 0;
ushort curReg = 0;

void push(long v) { if (dsp < STK_SZ) { dstack[++dsp] = v; } }
long pop() { return (dsp > 0) ? dstack[dsp--] : 0; }

void rpush(ushort v) { if (rsp < STK_SZ) { rstack[++rsp] = v; } }
ushort rpop() { return (rsp > 0) ? rstack[rsp--] : -1; }

#ifdef __DEV_BOARD__
int _getch() { return (mySerial.available()) ? mySerial.read(): 0; }
void printString(const char* str) { mySerial.print(str); }
#else
void printString(const char* str) { printf("%s", str); }
#endif

void vmInit() {
    dsp = rsp = here = curReg = 0;
    ihere = IHERE_INIT;
    for (int i = 0; i < CODE_SZ; i++) { code[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { func[i] = 0; }
    for (int i = 0; i < NUM_REGS; i++) { reg[i] = 0; }
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

int digit(byte c) { return (('0' <= c) && (c <= '9')) ? (c - '0') : -1; }
int alphaL(byte c) { return (('a' <= c) && (c <= 'z')) ? (c - 'a') : -1; }
int alphaU(byte c) { return (('A' <= c) && (c <= 'Z')) ? (c - 'A') : -1; }

int alpha(byte c) {
    int x = alphaU(c);
    if (0 <= x) return x;
    x = alphaL(c);
    if (0 <= x) return x + 26;
    return -1;
}

int number(int pc) {
    long num = 0;
    while (('0' <= code[pc]) && (code[pc] <= '9')) {
        num = (num * 10) + code[pc] - '0';
        pc++;
    }
    push(num);
    return pc;
}

int defineFunc(int pc) {
    byte fId = code[pc++];
    int fn = alpha(fId);
    int v = ((0 <= fn) && (fn < NUM_FUNCS)) ? 1 : 0;
    if (v) {
        code[here++] = '{';
        code[here++] = fId;
        func[fn] = here;
    }
    else { printStringF("-invalid function number (A:%c)-", 'a'-1+(NUM_FUNCS-26)); }
    while ((pc < CODE_SZ) && code[pc]) {
        if (v) { code[here++] = code[pc]; }
        if (code[pc] == '}') { break; }
        pc++;
    }
    return pc+1;
}

int doFunc(int pc) {
    int f = alpha(code[pc++]);
    if ((0 <= f) && (f < NUM_FUNCS) && (func[f])) {
        rpush(pc);
        pc = func[f];
    }
    return pc;
}

void dumpCode() {
    printStringF("\r\nCODE\r\nhere: %04d", here);
    char* txt = (char*)&code[here + 10]; int ti = 0;
    for (int i = 0; i < here; i++) {
        if ((i % 20) == 0) {
            if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); ti = 0; }
            printStringF("\n%04d: ", i);
        }
        txt[ti++] = (code[i] < 32) ? '.' : code[i];
        printStringF(" %02x", code[i]);
    }
    int x = here;
    while (x % 20) {
        printString("   ");
        x++;
    }
    if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); }
}

void dumpFuncs() {
  printString("\r\nFUNCTIONS");
  for (int i = 0; i < NUM_FUNCS; i++) {
    byte fId = ((i < 26) ? 'A' : 'a') + (i%26);
    if ((0<i) && (i%5)) { printStringF("    "); } else { printString("\r\n"); }
    printStringF("f%c: %-4d", fId, (int)func[i]);
  }
}

void dumpRegs() {
    printString("\r\nREGISTERS");
    for (int i = 0; i < NUM_REGS; i++) {
        byte fId = 'a' + i;
        if ((0 < i) && (i % 5)) { printStringF("    "); }
        else { printString("\r\n"); }
        printStringF("%c: %-10ld", fId, reg[i]);
    }
}

void dumpStack() {
    printString("\r\nSTACK: (");
    for (int i = 1; i <= dsp; i++) { printStringF(" %d", dstack[i]); }
    printString(" )");
}

void dumpVars() {
    printString("\r\nVARIABLES");
    for (int i = 0; i < NUM_VARS; i++) { 
        if ((0 < i) && (i % 5)) { printStringF("    "); }
        else { printString("\r\n"); }
        printStringF("[%03d]: %-10d", i, var[i]);
    }
}

void dumpAll() {
    dumpStack();
    dumpRegs();
    dumpFuncs();
    dumpVars();
    dumpCode();
}

int run(int pc) {
    long t1 = 0, t2 = 0;
    while (rsp >= 0) {
        if ((pc < 0) || (CODE_SZ <= pc)) { return 0; }
        byte ir = code[pc++];
        // printStringF("\npc:%04d, ir:%03d [%c] ", pc - 1, ir, ir); dumpStack();
        switch (ir) {
        case 0: return pc;
        case '{': pc = defineFunc(pc); break;
        case '}': pc = rpop(); break;
        case '#': push(T); break;
        case '$': t1 = pop(); t2 = pop(); push(t1); push(t2); break;
        case '@': push(N); break;
        case '\\': pop(); break;
        case ';': push(reg[curReg]); break;
        case ':': reg[curReg] = pop(); break;
        case '!': t1 = reg[curReg]; ((0 <= t1) && (t1 < NUM_VARS)) ? var[t1] = pop() : pop(); break;
        case '?': t1 = reg[curReg]; ((0 <= t1) && (t1 < NUM_VARS)) ? push(var[t1]) : push(0); break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            pc = number(pc - 1); break;
        case 'f': pc = doFunc(pc); break;
        case 'a': case 'b': case 'c': case 'd': case 'e': /* case 'f': */ case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            curReg = ir - 'a';
            if (code[pc] == '+') { ++pc; reg[curReg]++; }
            if (code[pc] == '-') { ++pc; reg[curReg]--; }
            break;
        case '+': if (code[pc] == '+') { pc++; T++; } else { if (dsp > 1) { t1 = pop(); T += t1; } } break;
        case '-': if (code[pc] == '-') { pc++; T--; } else { if (dsp > 1) { t1 = pop(); T -= t1; } } break;
        case '*': if (dsp > 1) { t1 = pop(); T *= t1; } break;
        case '/': if (dsp > 1) { t1 = pop(); T /= t1; } break;
        case '%': if (dsp > 1) { t1 = pop(); T %= t1; } break;
        case '|': if (dsp > 1) { t1 = pop(); T |= t1; } break;
        case '&': if (dsp > 1) { t1 = pop(); T &= t1; } break;
        case '_': if (dsp > 0) { T = -T; } break;
        case '~': if (dsp > 0) { T = ~T; } break;
        case '.': printStringF("%ld", pop());  break;
        case ',': printStringF("%c", (char)pop());  break;
        case '"': while ((code[pc] != '"') && (pc < CODE_SZ)) { printStringF("%c", code[pc]); pc++; } pc++; break;
        case '=': if (dsp > 1) { t1 = pop(); T = (T == t1) ? -1 : 0; } break;
        case '<': if (dsp > 1) { t1 = pop(); T = (T < t1) ? -1 : 0; } break;
        case '>': if (dsp > 1) { t1 = pop(); T = (T > t1) ? -1 : 0; } break;
        case '^': push(_getch()); break;
        case '[': rpush(pc); if (T == 0) { while ((pc < CODE_SZ) && (code[pc] != ']')) { pc++; } } break;
        case ']': if (pop()) { pc = rstack[rsp]; }
                else { rpop(); } break;
        case '(': if (pop() == 0) { while ((pc < CODE_SZ) && (code[pc] != ')')) { pc++; } } break;
        case 'B': printString(" "); break;
        case 'F': dumpFuncs(); break;
        case 'I': t1 = code[pc++];
            if (t1 == 'A') { dumpAll(); }
            if (t1 == 'C') { dumpCode(); }
            if (t1 == 'F') { dumpFuncs(); }
            if (t1 == 'R') { dumpRegs(); }
            if (t1 == 'S') { dumpStack(); }
            if (t1 == 'V') { dumpVars(); }
            break;
        case 'M': push(millis()); break;
        case 'R': dumpRegs(); break;
        case 'S': t1 = code[pc++]; if (t1 == 'S') { printString("Halleluya!"); } break;
        case 'X': t1 = code[pc++]; if (t1 == 'X') { vmInit(); } break;
        default: break;
        }
    }
    return 0;
}

void ok() {
    printString(" ok. (");
    for (int i = 1; i <= dsp; i++) { printStringF(" %d", dstack[i]); }
    printString(" )\r\n");
}

#ifdef __DEV_BOARD__
#define iLed 13
ulong nextBlink = 0;
int ledState = 0;
void setup() {
    mySerial.begin(19200);
    while (!mySerial) {}
    while (mySerial.available()) {}
    vmInit();
    pinMode(iLed, OUTPUT);
    ok();
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
            ihere = IHERE_INIT;
            run(ihere);
            ok();
        } else {
            if (ihere < CODE_SZ) {
                code[ihere++] = c;
                char b[2]; b[0] = c; b[1] = 0;
                printString(b);
            }
        }
    }
    // autoRun();
}

#else
int loop() {
    ihere = IHERE_INIT;
    char* buf = (char*)&code[ihere];
    ok();
    fgets(buf, IHERE_SZ, stdin);
    if (strcmp(buf, "bye\n") == 0) { return 0; }
    run(ihere);
    return 1;
}

void process_arg(char* arg)
{
    if ((*arg == 'i') && (*(arg + 1) == ':')) {
        arg = arg + 2;
        strcpy_s(input_fn, sizeof(input_fn), arg);
    } else if (*arg == '?') {
        printString("usage s4 [args] [source-file]\n");
        printString("  -i:file\n");
        printString("  -? - Prints this message\n");
        exit(0);
    } else { printf("unknown arg '-%s'\n", arg); }
}

int main(int argc, char** argv) {
    vmInit();
    char* buf = (char *)&code[ihere];
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
            while (fgets(buf, IHERE_SZ, fp) == buf) { run(ihere); }
            fclose(fp);
        }
    }

    while (loop());
}
#endif
