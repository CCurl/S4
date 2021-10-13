// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "s4.h"

#define STK_SZ   31
#define HERE     REG[7]

typedef struct {
    addr pc;
    long from;
    long to;
} LOOP_ENTRY_T;

struct {
    long dsp, rsp, lsp;
    long reg[NUM_REGS];
    byte user[USER_SZ];
    addr func[NUM_FUNCS];
    CELL dstack[STK_SZ + 1];
    addr rstack[STK_SZ + 1];
    LOOP_ENTRY_T lstack[4];
} sys;

byte isBye = 0, isError = 0;
char buf[100];
extern FILE *input_fp;

#define REG        sys.reg
#define USER       sys.user
#define FUNC       sys.func

#define T        sys.dstack[sys.dsp]
#define N        sys.dstack[sys.dsp-1]
#define R        sys.rstack[sys.rsp]
#define L        sys.lsp
#define DROP1    pop()
#define DROP2    pop(); pop()

void push(CELL v) { if (sys.dsp < STK_SZ) { sys.dstack[++sys.dsp] = v; } }
CELL pop() { return (sys.dsp > 0) ? sys.dstack[sys.dsp--] : 0; }

void rpush(addr v) { if (sys.rsp < STK_SZ) { sys.rstack[++sys.rsp] = v; } }
addr rpop() { return (sys.rsp > 0) ? sys.rstack[sys.rsp--] : 0; }

void vmInit() {
    sys.dsp = sys.rsp = sys.lsp = 0;
    for (int i = 0; i < NUM_REGS; i++) { REG[i] = 0; }
    for (int i = 0; i < USER_SZ; i++) { USER[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { FUNC[i] = 0; }
    REG['F' - 'A'] = NUM_FUNCS;
    REG['G' - 'A'] = (long)&sys.reg[0];
    REG['K' - 'A'] = 1000;
    REG['R' - 'A'] = NUM_REGS;
    REG['S' - 'A'] = (long)&sys;
    REG['U' - 'A'] = (long)&sys.user[0];
    REG['Z' - 'A'] = USER_SZ;
}

void printStringF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printString(buf);
}

#define BetweenI(n, x, y) ((x <= n) && (n <= y))

int hexNum(char x) {
    if (BetweenI(x, '0', '9')) { return x - '0'; }
    if (BetweenI(x, 'A', 'Z')) { return x - 'A'; }
    if (BetweenI(x, 'a', 'z')) { return x - 'a'; }
    return -1;
}

int getNum3(addr pc, char st, char en, CELL& num) {
    addr oldPC = pc;
    byte ir = USER[pc];
    num = 0;
    if (BetweenI(ir, st, en)) { num =              (ir-st); ir = USER[++pc]; }
    if (BetweenI(ir, st, en)) { num = (num * 26) + (ir-st); ir = USER[++pc]; }
    if (BetweenI(ir, st, en)) { num = (num * 26) + (ir-st); ir = USER[++pc]; }
    if (pc == oldPC) { isError = 1; }
    return pc - oldPC;
}

addr doDefineFunction(addr pc) {
    CELL fn = 0;
    int nc = getNum3(pc, 'A', 'Z', fn);
    if (isError) { return pc; }
    USER[HERE++] = '`';
    FUNC[fn] = HERE+nc;
    while (USER[pc] && USER[pc] !='`') { USER[HERE++] = USER[pc++]; }
    if (USER[pc] == '`') { 
        USER[HERE++] = USER[pc++];
        while (USER[FUNC[fn]] == ' ') { ++FUNC[fn]; }
        return pc;
    }
    isError = 1;
    printString("-dfErr-");
    return 0;
}

addr getRegFuncNum(addr pc, char st, char en, CELL& num) {
    int nc = getNum3(pc, st, en, num);
    if (isError) { return pc; }
    return pc + nc;
}

addr doBegin(addr pc) {
    rpush(pc);
    if (T == 0) {
        while ((pc < USER_SZ) && (USER[pc] != ']')) { pc++; }
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

addr doIJK(addr pc, char mode) {
    push(0);
    if ((mode == 'I') && (0 < L)) { T = sys.lstack[L - 1].from; }
    if ((mode == 'J') && (0 < L)) { T = sys.lstack[L - 2].from; }
    if ((mode == 'K') && (0 < L)) { T = sys.lstack[L - 3].from; }
    return pc;
}

void dumpCode() {
    printStringF("\r\nCODE: size: %d bytes, HERE=%d", USER_SZ, HERE);
    if (HERE == 0) { printString("\r\n(no user defined)"); return; }
    int ti = 0, x = HERE, npl = 20;
    char txt[32];
    for (long i = 0; i < HERE; i++) {
        if ((i % npl) == 0) {
            if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); ti = 0; }
            printStringF("\n\r%05d: ", i);
        }
        txt[ti++] = (USER[i] < 32) ? '.' : USER[i];
        printStringF(" %3d", USER[i]);
    }
    while (x % npl) {
        printString("    ");
        x++;
    }
    if (ti) { txt[ti] = 0;  printStringF(" ; %s", txt); }
}

void dumpStack(int hdr) {
    if (hdr) { printStringF("\r\nSTACK: size: %d ", STK_SZ); }
    printString("(");
    for (int i = 1; i <= sys.dsp; i++) { printStringF("%s%ld", (i > 1 ? " " : ""), sys.dstack[i]); }
    printString(")");
}

char *RFName(char *buf, int i, char b) {
    char f1 = i / (26 * 26) + b;
    char f2 = (i / 26) % 26 + b;
    char f3 = i % 26 + b;
    if (f1 == b) { f1 = ' '; }
    if ((f1 == ' ') && (f2 == b)) { f2 = ' '; }
    sprintf(buf, "%c%c%c", f1, f2, f3);
    return buf;
}

void dumpFuncs() {
    printStringF("\r\nFUNCTIONS: (%d available)", NUM_FUNCS);
    int n = 0;
    char buf[4];
    for (int i = 0; i < NUM_FUNCS; i++) {
        if (FUNC[i]) {
            if (((n++) % 5) == 0) { printString("\r\n"); }
            printStringF("%3s: %-15d", RFName(buf,i,'A'), FUNC[i], " ");
        }
    }
}

void dumpRegs() {
    printStringF("\r\nREGISTERS: (%d available)", NUM_REGS);
    int n = 0;
    char buf[4];
    for (int i = 0; i < NUM_REGS; i++) {
        if (REG[i]) {
            if (((n++) % 5) == 0) { printString("\r\n"); }
            printStringF("%3s: %-15d", RFName(buf, i, 'a'), REG[i], " ");
        }
    }
}

void dumpAll() {
    dumpStack(1);   printString("\r\n");
    dumpRegs();     printString("\r\n");
    dumpCode();     printString("\r\n");
    dumpFuncs();    printString("\r\n");
}

addr doFile(addr pc) {
    int ir = USER[pc++];
    switch (ir) {
#ifdef __PC__
    case 'C':
        if (T) { fclose((FILE*)T); }
        DROP1;
        break;
    case 'O': {
            char* md = (char*)&USER[pop()];
            char* fn = (char*)&USER[T];
            T = (long)fopen(fn, md);
        } break;
    case 'R': if (T) {
            long n = fread(buf, 1, 1, (FILE*)T);
            T = ((n) ? buf[0] : 0);
            push(n);
        } break;
    case 'W': if (T) {
            FILE* fh = (FILE*)pop();
            buf[1] = 0;
            buf[0] = (byte)pop();
            fwrite(buf, 1, 1, fh);
        } break;
#endif
    case 'N':
        if ((0 <= T) && (T < NUM_FUNCS)) { T = FUNC[T]; }
        else { T = 0; }
        break;
    }
    return pc;
}

addr doPin(addr pc) {
    int ir = USER[pc++];
    long pin = pop(), val = 0;
    switch (ir) {
    case 'I': pinMode(pin, INPUT); break;
    case 'U': pinMode(pin, INPUT_PULLUP); break;
    case 'O': pinMode(pin, OUTPUT); break;
    case 'R': ir = USER[pc++];
        if (ir == 'D') { push(digitalRead(pin)); }
        if (ir == 'A') { push(analogRead(pin)); }
        break;
    case 'W': ir = USER[pc++]; val = pop();
        if (ir == 'D') { digitalWrite(pin, val); }
        if (ir == 'A') { analogWrite(pin, val); }
        break;
    }
    return pc;
}

addr doExt(addr pc) {
    int ir = USER[pc++];
    switch (ir) {
    case 'F': pc = doFile(pc);          break;
    case 'I': ir = USER[pc++];
        if (ir == 'A') { dumpAll(); }
        if (ir == 'C') { dumpCode(); }
        if (ir == 'F') { dumpFuncs(); }
        if (ir == 'R') { dumpRegs(); }
        if (ir == 'S') { dumpStack(0); }
        break;
    case 'P': pc = doPin(pc);           break;
    case 'S': sys.dsp = 0;              break;
    case 'T': isBye = 1;                break;
    case 'R': vmInit();                 break;
    }
    return pc;
}

addr run(addr pc) {
    CELL t1, t2;
    byte* bp;
    isError = 0;
    while (!isError && (0 < pc)) {
        byte ir = USER[pc++];
        switch (ir) {
        case 0: return -1;
        case ' ': while (USER[pc] == ' ') { pc++; } break;  // 32
        case '!': t2 = pop(); t1 = pop();                   // 33
            *(long*)&USER[t2] = t1;
            break;
        case '"': buf[1] = 0;                          // 34
            while ((pc < USER_SZ) && (USER[pc] != '"')) {
                buf[0] = USER[pc++];
                printString(buf);
            }
            ++pc; break;
        case '#': push(T);                      break;  // 35 (DUP)
        case '$': t1 = N; N = T; T = t1;        break;  // 36 (SWAP)
        case '%': push(N);                      break;  // 37 (OVER)
        case '&': t1 = pop(); T &= t1;          break;  // 38
        case '\'': push(USER[pc++]);            break;  // 39
        case '(': if (pop() == 0) {                     // 40
            while ((pc < USER_SZ) && (USER[pc] != ')')) { ++pc; }
            ++pc;
        } break;
        case ')': /* THEN */                    break;  // 41
        case '*': t1 = pop(); T *= t1;          break;  // 42
        case '+': t1 = pop(); T += t1;          break;  // 43
        case ',': printChar((char)pop());       break;  // 44
        case '-': t1 = pop(); T -= t1;          break;  // 45
        case '.': printStringF("%ld", pop());   break;  // 46
        case '/': t1 = pop();                           // 47
            if (t1) { T /= t1; }
            else { isError = 1; }
            break;
        case '0': case '1': case '2': case '3': case '4':   // 48-57
        case '5': case '6': case '7': case '8': case '9':
            push(ir - '0');
            t1 = USER[pc] - '0';
            while ((0 <= t1) && (t1 <= 9)) {
                T = (T * 10) + t1;
                t1 = USER[++pc] - '0';
            }
            break;
        case ':': pc = getRegFuncNum(pc, 'A', 'Z', t1);          // 58
            if ((!isError) && (FUNC[t1])) { rpush(pc); pc = FUNC[t1]; }
            break;
        case ';': if (sys.rsp < 1) { sys.rsp = 0;  return pc; }
                pc = rpop();                           break;  // 59
        case '<': t1 = pop(); T = T < t1  ? 1 : 0;     break;  // 60
        case '=': t1 = pop(); T = T == t1 ? 1 : 0;     break;  // 61
        case '>': t1 = pop(); T = T > t1  ? 1 : 0;     break;  // 62
        case '?': /* FREE */                           break;  // 63
        case '@': T = *(long*)&USER[T];                break;  // 64
        case 'A': ir = USER[pc++];
            if (ir == '@') { T = *(byte *)T; }
            if (ir == '!') { *(byte *)T = N & 0xff; DROP2; }
            break;
        case 'B': printChar(' ');                      break;
        case 'C': ir = USER[pc++];
            if (ir == '@') { T = USER[T]; }
            if (ir == '!') { USER[T] = (byte)N; DROP2; }
            break;
        case 'D': t2 = pop();  t1 = pop(); // Open block file
#ifdef __PC__
            sprintf(buf, "block.%03ld", t1);
            push((CELL)fopen(buf, t2 ? "wt" : "rt"));
#endif
            break;
        case 'E': t1 = pop();  // LOAD
#ifdef __PC__
            if (input_fp) { fclose(input_fp); }
            sprintf(buf, "block.%03ld", t1);
            input_fp = fopen(buf, "rt");
#endif
            break;
        case 'F': T = ~T;                              break;
        case 'G': pc = getRegFuncNum(pc, 'A', 'Z', t1);
            if ((!isError) && (FUNC[t1])) { pc = FUNC[t1]; }
            break;
        case 'H': push(0);
            t1 = hexNum(USER[pc]);
            while (0 <= t1) {
                T = (T * 0x10) + t1;
                t1 = hexNum(USER[++pc]);
            }
            break;
        case 'I': pc = doIJK(pc, ir);                  break;
        case 'J': pc = doIJK(pc, ir);                  break;
        case 'K': ir = USER[pc++];
            if (ir == '?') { push(charAvailable()); }
            if (ir == 'b') { push(getChar()); }
            break;
        case 'L': N = N << T; DROP1;                   break;
        case 'M': ir = USER[pc++];
            bp = (byte*)T;
            if (ir == '@') { T = *(long *)bp; }
            if (ir == '!') {
                t1 = N; DROP2;
                *(bp++) = ((t1) & 0xff);
                *(bp++) = ((t1 >> 8) & 0xff);
                *(bp++) = ((t1 >> 16) & 0xff);
                *(bp++) = ((t1 >> 24) & 0xff);
            }                                          break;
        case 'N': printString("\r\n");                 break;
        case 'O': T = -T;                              break;
        case 'P': T++;                                 break;
        case 'Q': T--;                                 break;
        case 'R': N = N >> T; DROP1;                   break;
        case 'S': t2 = N; t1 = T;                      // SLASH-MOD
            if (t1 == 0) { isError = 1; }
            else { N = (t2 / t1); T = (t2 % t1); }
            break;
        case 'T': push(millis());                      break;
        case 'U': if (T < 0) { T = -T; }               break;
        case 'V': /* FREE */                           break;
        case 'W': delay(pop());                        break;
        case 'X': pc = doExt(pc);                      break;
        case 'Y': /* FREE */                           break;
        case 'Z':  printString((char*)&USER[pop()]);   break;
        case '[': pc = doFor(pc);                      break;  //  91
        case '\\': DROP1;                              break;  //  92
        case ']': pc = doNext(pc);                     break;  //  93
        case '^': t1 = pop(); T ^= t1;                 break;  //  94
        case '_':                                              //  95
            while (USER[pc] && (USER[pc] != ir)) { USER[T++] = USER[pc++]; }
            ++pc; USER[T++] = 0;
            break;
        case '`': if (USER[pc] == ir) {                        //  96
                pc++; 
                if (USER[HERE-1] == ir) { HERE--; }
                while ((USER[pc]) && (USER[pc] != ir)) { USER[HERE++] = USER[pc++]; }
                USER[HERE++] = ir;
                pc++;
            } else { pc = doDefineFunction(pc); }
            break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
        case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
        case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
        case 's': case 't': case 'u': case 'v': case 'w': case 'x':
        case 'y': case 'z': t1 = ir - 'a'; 
            pc = getRegFuncNum(pc-1, 'a', 'z', t1);
            if (t1 < NUM_REGS) {
                push(REG[t1]);
                ir = USER[pc];
                if (ir == '+') { ++pc; ++REG[t1]; }
                if (ir == '-') { ++pc; --REG[t1]; }
                if (ir == ';') { pop(); ++pc; REG[t1] = pop(); }
            }
            else { printString("-regNum-"); isError = 1; }
            break;
        case '{': pc = doBegin(pc);                    break;  // 123
        case '|': t1 = pop(); T |= t1;                 break;  // 124
        case '}': pc = doWhile(pc);                    break;  // 125
        case '~': T = (T) ? 0 : 1;                     break;  // 126
        }
    }
    if (isError && ((CELL)pc < HERE)) { REG[4] = pc; }
    return pc;
}

void setCodeByte(addr loc, char ch) {
    if ((0 <= loc) && (loc < USER_SZ)) { USER[loc] = ch; }
}

addr functionAddress(const char *fn) {
    long t1;
    USER[HERE+0] = fn[0];
    USER[HERE+1] = fn[1];
    USER[HERE+2] = fn[2];
    getRegFuncNum(HERE, 'A', 'Z', t1);
    return (t1 < 0) ? -1 : FUNC[t1];
}
