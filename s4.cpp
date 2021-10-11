// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "s4.h"

#define STK_SZ   31
#define HERE     (addr)MEM[7]
#define FN_SZ    1

typedef struct {
    addr pc;
    long from;
    long to;
} LOOP_ENTRY_T;

struct {
    long dsp, rsp, lsp;
    byte code[CODE_SZ];
    long mem[MEM_SZ];
    addr func[NUM_FUNCS];
    long dstack[STK_SZ + 1];
    addr rstack[STK_SZ + 1];
    LOOP_ENTRY_T lstack[4];
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
#define L        sys.lsp
#define DROP1    pop()
#define DROP2    pop(); pop()

void push(long v) { if (sys.dsp < STK_SZ) { sys.dstack[++sys.dsp] = v; } }
long pop() { return (sys.dsp > 0) ? sys.dstack[sys.dsp--] : 0; }

void rpush(addr v) { if (sys.rsp < STK_SZ) { sys.rstack[++sys.rsp] = v; } }
addr rpop() { return (sys.rsp > 0) ? sys.rstack[sys.rsp--] : 0; }

void vmReset() {
    sys.dsp = sys.rsp = sys.lsp = 0;
    for (int i = 0; i < CODE_SZ; i++) { CODE[i] = 0; }
    for (int i = 0; i < MEM_SZ; i++) { MEM[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { FUNC[i] = 0; }
    MEM['C' - 'A'] = CODE_SZ;
    MEM['D' - 'A'] = (long)&sys.code[0];
    MEM['F' - 'A'] = (long)&sys.func[0];
    MEM['M' - 'A'] = (long)&sys.mem[0];
    MEM['N' - 'A'] = NUM_FUNCS;
    MEM['S' - 'A'] = (long)&sys;
    MEM['V' - 'A'] = 104; // byte addr of first unused location
    MEM['Z' - 'A'] = MEM_SZB;
}

void vmInit() {
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

inline int funcNum(char x) {
    if (('A' <= x) && (x <= 'Z')) { return x - 'A'; }
    return -1;
}

addr GetFunctionNum(addr pc, long& fn, int isDefine) { 
    fn = funcNum(CODE[pc]);
    if (fn < 0) { return pc; }
    if (isDefine) {  
        CODE[HERE++] = '{';
        CODE[HERE++] = CODE[pc++];
    }
    int f2 = funcNum(CODE[++pc]);
    if (0 <= f2) {
        if (isDefine) { CODE[HERE++] = CODE[pc]; }
        fn = fn * 26 + f2;
        f2 = funcNum(CODE[++pc]);
        if (0 <= f2) { 
            if (isDefine) { CODE[HERE++] = CODE[pc]; }
            fn = fn * 26 + f2;
        }
    }
    if ((fn < 0) || (NUM_FUNCS <= fn)) { isError = 1; }
    return pc;
}

addr doDefineFunction(addr pc) {
    if (pc < HERE) { return pc; }
    long fn = -1;
    pc = GetFunctionNum(pc, fn, 1);
    if (isError) { return pc + FN_SZ; }
    FUNC[fn] = HERE;
    while ((pc < CODE_SZ) && CODE[pc]) {
        CODE[HERE++] = CODE[pc++];
        if (CODE[HERE - 1] == '}') { return pc; }
    }
    isError = 1;
    printString("-code-overflow-");
    return pc;
}

addr doBegin(addr pc) {
    rpush(pc);
    if (T == 0) {
        while ((pc < CODE_SZ) && (CODE[pc] != ']')) { pc++; }
    }
    return pc;
}

addr doWhile(addr pc) {
    if (T) { pc = R; }
    else { DROP1;  rpop(); }
    return pc;
}

addr doFor(addr pc) {
    if (L < 4) {
        LOOP_ENTRY_T* x = &sys.lstack[L];
        L++;
        x->pc = pc;
        x->to = pop();
        x->from = pop();
        if (x->to < x->from) {
            push(x->to);
            x->to = x->from;
            x->from = pop();
        }
    }
    return pc;
}

addr doNext(addr pc) {
    if (L < 1) { L = 0; }
    else {
        LOOP_ENTRY_T* x = &sys.lstack[L - 1];
        ++x->from;
        if (x->from <= x->to) { pc = x->pc; }
        else { L--; }
    }
    return pc;
}

addr doIJK(addr pc, int mode) {
    push(0);
    if ((mode == 1) && (0 < L)) { T = sys.lstack[L - 1].from; }
    if ((mode == 2) && (0 < L)) { T = sys.lstack[L - 2].from; }
    if ((mode == 3) && (0 < L)) { T = sys.lstack[L - 3].from; }
    return pc;
}

void dumpCode() {
    printStringF("\r\nCODE: size: %d bytes, HERE=%d", CODE_SZ, HERE);
    if (HERE == 0) { printString("\r\n(no code defined)"); return; }
    int ti = 0, x = HERE, npl = 20;
    char txt[32];
    for (addr i = 0; i < HERE; i++) {
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
            if (((n++) % 5) == 0) { printString("\r\n"); }
            printStringF("%c:%5d%10s", i+((i>25)?'G':'A'), FUNC[i], " ");
        }
    }
}

void dumpStack(int hdr) {
    if (hdr) { printStringF("\r\nSTACK: size: %d ", STK_SZ); }
    printString("(");
    for (int i = 1; i <= sys.dsp; i++) { printStringF("%s%ld", (i > 1 ? " " : ""), sys.dstack[i]); }
    printString(")");
}

void dumpRegs() {
    printStringF("\r\nREGISTERS: ");
    int n = 0;
    for (int i = 0; i < 26; i++) {
        long x = MEM[i];
        if (((n++) % 5) == 0) { printString("\r\n"); }
        printStringF("%c: %-10ld  ", i + 'A', x);
    }
}

void dumpAll() {
    dumpStack(1);   printString("\r\n");
    dumpRegs();     printString("\r\n");
    dumpCode();     printString("\r\n");
    dumpFuncs();    printString("\r\n");
}

addr doFile(addr pc) {
    int ir = CODE[pc++];
    switch (ir) {
#ifdef __PC__
    case 'C':
        if (T) { fclose((FILE*)T); }
        DROP1;
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
        if ((0 <= T) && (T < NUM_FUNCS)) { T = FUNC[T]; }
        else { T = 0; }
        break;
    }
    return pc;
}

addr doPin(addr pc) {
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

addr doExt(addr pc) {
    byte ir = CODE[pc++];
    switch (ir) {
    case 'F': pc = doFile(pc);          break;
    case 'I': pc = doIJK(pc, 1);        break;
    case 'J': pc = doIJK(pc, 2);        break;
    case 'K': pc = doIJK(pc, 3);        break;
    case 'P': pc = doPin(pc);           break;
    case 'S': sys.dsp = 0;              break;
    case 'T': isBye = 1;                break;
    case 'X': vmReset();                break;
    }
    return pc;
}

addr run(addr pc) {
    long t1, t2;
    byte* b
        p;
    isError = 0;
    while (!isError && (0 < pc)) {
        byte ir = CODE[pc++];
        switch (ir) {
        case 0: return -1; /* FREE */                      // 0
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
        case '\\': DROP1;                break;             // 92 (DROP)
        case '&': t1 = pop(); T &= t1;   break;             // 38
        case '\'': push(CODE[pc++]);     break;             // 39
        case '(': if (pop() == 0) {                         // 40
            while ((pc < CODE_SZ) && (CODE[pc] != ')')) { ++pc; }
            ++pc;
        }
                break;
        case ')': /*maybe ELSE?*/        break;             // 41
        case '*': t1 = pop(); T *= t1;   break;             // 42
        case '+': t1 = pop(); T += t1;   break;             // 43
        case '-': t1 = pop(); T -= t1;   break;             // 45
        case '/': t1 = pop(); 
            if (t1) { T /= t1; }
            else { isError = 1; }
            break;  // 47
        case ',': printStringF("%c", (char)pop());  break;  // 44
        case '.': printStringF("%ld", pop());       break;  // 46
        case '0': case '1': case '2': case '3': case '4':   // 48-57
        case '5': case '6': case '7': case '8': case '9':
            push(ir - '0');
            t1 = CODE[pc] - '0';
            while ((0 <= t1) && (t1 <= 9)) {
                T = (T * 10) + t1;
                t1 = CODE[++pc] - '0';
            }
            break;
        case ':': pc = GetFunctionNum(pc, t1, 0);                // 58
            if ((!isError) && (FUNC[a])) { rpush(pc); pc = FUNC[t1]; }
            break;
        case ';': if (sys.rsp == 0) { return pc; }
            pc = rpop();                             break;  // 59
        case '<': t1 = pop(); T = T < t1  ? 1 : 0;   break;  // 60
        case '=': t1 = pop(); T = T == t1 ? 1 : 0;   break;  // 61
        case '>': t1 = pop(); T = T > t1  ? 1 : 0;   break;  // 62
        // case '?': push(_getch());                    break;  // 63
        case '@': if ((0 <= T) && (T < MEM_SZ)) { T = MEM[T]; }
                break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z': ir -= 'a';
            push(MEM[ir]); t1 = CODE[pc];
            if (t1 == '+') { ++pc; ++MEM[ir]; }
            if (t1 == '-') { ++pc; --MEM[ir]; }
            if (t1 == ';') { pop(); ++pc; MEM[ir] = pop(); }
            break;
        case '[': pc = (CODE[pc] == '[') ? doBegin(pc + 1) : doFor(pc);
            break;
        case ']': pc = (CODE[pc] == ']') ? doWhile(pc + 1) : doNext(pc);
            break;
        case '^': t1 = pop(); T ^= t1;      break;          // 94
        case '_':                                           // 95
            while (CODE[pc] && (CODE[pc] != '_')) { bMem[T++] = CODE[pc++]; }
            ++pc; bMem[T++] = 0;
            break;
        case '`': /* FREE */                break;          // 96
        case 'A': ir = CODE[pc++];
            bp = (byte*)T;
            if (ir == '@') { T = *bp; }
            if (ir == '!') { *bp = N & 0xff; DROP2; }
            break;
        case 'B': printString(" ");         break;
        case 'C': ir = CODE[pc++];
            bp = &bMem[T];
            if ((0 <= T) && (T < MEM_SZB)) {
                if (ir == '@') { T = *bp; }
                if (ir == '!') { *bp = N & 0xff; DROP2; }
            }
            break;
        case 'D': ir = CODE[pc++];
            bp = &CODE[T];
            if ((0 <= T) && (T < CODE_SZ)) {
                if (ir == '@') { T = *bp; }
                if (ir == '!') { *bp = N & 0xff; DROP2; }
            }
            break;
        case 'E': rpush(pc); pc = (addr)pop();  break;
        case 'F': T = ~T;                       break;
        case 'G': /* FREE */                    break;
        case 'H': push(0);
            t1 = hexNum(CODE[pc]);
            while (0 <= t1) {
                T = (T * 0x10) + t1;
                t1 = hexNum(CODE[++pc]);
            }
            break;
        case 'I': t1 = CODE[pc++];
            if (t1 == 'A') { dumpAll(); }
            if (t1 == 'C') { dumpCode(); }
            if (t1 == 'F') { dumpFuncs(); }
            if (t1 == 'R') { dumpRegs(); }
            if (t1 == 'S') { dumpStack(0); }
            break;
        case 'J': t1 = GetFunctionNum(pc, t1, 0);
            if ((!isError) && (FUNC[t1])) { pc = FUNC[t1]; }
            break;
        case 'K': T *= 1000;   break;
        case 'L': t1 = pop();  // LOAD
#ifdef __PC__
            if (input_fp) { fclose(input_fp); }
            sprintf_s(buf, sizeof(buf), "block.%03ld", t1);
            fopen_s(&input_fp, buf, "rt");
#else
            printString("-l:pc only-");
#endif
            break;
        case 'M': ir = CODE[pc++];
            bp = (byte*)T;
            if (ir == '@') { T = *bp; }
            if (ir == '!') {
                t1 = N; DROP2;
                *(bp++) = ((t1) & 0xff);
                *(bp++) = ((t1 >> 8) & 0xff);
                *(bp++) = ((t1 >> 16) & 0xff);
                *(bp++) = ((t1 >> 24) & 0xff);
            }
            break;          // 97
        case 'N': printString("\r\n");       break;
        case 'O': T = -T;                    break;
        case 'P': T++;                       break;
        case 'Q': T--;                       break;
        case 'R': N = N >> T; DROP1;         break;
        case 'S': t2 = N; t1 = T;            // /MOD
            if (t1 == 0) { isError = 1; }
            else { N = (t2 / t1); T = (t2 % t1); }
            break;
        case 'T': push(millis());            break;
        case 'U': if (T < 0) { T = -T; }     break;
        case 'V': N = N << T; DROP1;         break;
        case 'W': delay(pop());              break;
        case 'X': pc = doExt(pc);            break;
        case 'Y': /* FREE */                 break;
        case 'Z':  if ((0 <= T) && (T < MEM_SZB)) { 
                bp = &bMem[pop()];
                printString((char*)bp); }
            break;
        case '{': pc = doDefineFunction(pc); break;    // 123
        case '|': t1 = pop(); T |= t1;       break;    // 124
        case '}': if (0 < sys.rsp) { pc = rpop(); }    // 125
                else { sys.rsp = 0; return pc; }
            break;
        case '~': T = (T) ? 0 : 1;           break;    // 126
        }
    }
    return 0;
}

void setCodeByte(addr loc, char ch) {
    if ((0 <= loc) && (loc < CODE_SZ)) { CODE[loc] = ch; }
}

long registerVal(int reg) {
    if ((0 <= 'A') && (reg <= 'Z')) { return MEM[reg - 'A']; }
    if ((0 <= 'a') && (reg <= 'z')) { return MEM[reg - 'a']; }
    return 0;
}

addr functionAddress(const char *fn) {
    long t1;
    CODE[HERE+0] = fn[0];
    CODE[HERE+1] = fn[1];
    CODE[HERE+2] = fn[2];
    GetFunctionNum(HERE, t1, 0);
    return (t1 < 0) ? -1 : FUNC[t1];
}
