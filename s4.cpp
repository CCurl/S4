// S4 - a stack VM, inspired by Sandor Schneider's STABLE - https://w3group.de/stable.html

#include "s4.h"

long dstack[STK_SZ + 1];
long rstack[STK_SZ + 1];
long dsp, rsp;
long func[NUM_FUNCS];
byte isBye = 0;
char input_fn[32];
FILE* input_fp = NULL;
MEMORY_T memory;

#define T        dstack[dsp]
#define N        dstack[dsp-1]
#define R        rstack[rsp]
#define HERE     MEM[7]
#define MEM_SZB  (MEM_SZ*4)

#ifdef __PC__
    long millis() { return GetTickCount(); }
    int analogRead(int pin) { printStringF("-AR(%d)-", pin); return 0; }
    void analogWrite(int pin, int val) { printStringF("-AW(%d,%d)-", pin, val); }
    int digitalRead(int pin) { printStringF("-DR(%d)-", pin); return 0; }
    void digitalWrite(int pin, int val) { printStringF("-DW(%d,%d)-", pin, val); }
    void pinMode(int pin, int mode) { printStringF("-pinMode(%d,%d)-", pin, mode); }
    void delay(DWORD ms) { Sleep(ms); }
    HANDLE hStdOut = 0;
    void printString(const char* str) {
        DWORD n = 0, l = strlen(str);
        if (l) { WriteConsoleA(hStdOut, str, l, &n, 0); }
    }
#endif

void push(long v) { if (dsp < STK_SZ) { dstack[++dsp] = v; } }
long pop() { return (dsp > 0) ? dstack[dsp--] : 0; }

void rpush(long v) { if (rsp < STK_SZ) { rstack[++rsp] = v; } }
long rpop() { return (rsp > 0) ? rstack[rsp--] : -1; }

void vmInit() {
    dsp = rsp = 0;
    for (int i = 0; i < NUM_FUNCS; i++) { func[i] = 0; }
    for (int i = 0; i < MEM_SZ; i++) { MEM[i] = 0; }
}

void printStringF(const char* fmt, ...) {
    char buf[100];
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

int funcNum(char x, int alphaOnly) {
    if (('a' <= x) && (x <= 'z')) { return x - 'a'; }
    if (('A' <= x) && (x <= 'Z')) { return x - 'A'; }
    if ((!alphaOnly) && ('0' <= x) && (x <= '9')) { return x - '0' + 26; }
    return -1;
}

int GetFunctionNum(int pc) {
    int f1 = funcNum(CODE[pc], 0);
    int f2 = funcNum(CODE[pc + 1], 0);
    if ((f1 < 0) || (f2 < 0)) {
        printStringF("-%c%c:FN Bad-", CODE[pc], CODE[pc + 1]);
        return -1;
    }
    int fn = (f1 * 36) + f2;
    if ((fn < 0) || (NUM_FUNCS <= fn)) {
        printStringF("-%c%c:FN OOB-", CODE[pc], CODE[pc + 1]);
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
    CODE[HERE++] = CODE[pc+1];
    pc += 2;
    func[fn] = HERE;
    while ((pc < CODE_SZ) && CODE[pc]) {
        CODE[HERE++] = CODE[pc++];
        if (CODE[HERE-1] == '}') { return pc; }
    }
    printString("-overflow-");
    return pc;
}

int doCallFunction(int pc) {
    int fn = GetFunctionNum(pc);
    if (fn < 0) { return pc + 2; }
    if (!func[fn]) { return pc + 2; }
    rpush(pc + 2);
    return func[fn];
}

#ifdef __PC__
int doFile(int pc) {
    int ir = CODE[pc++];
    switch (ir) {
    case 'C':
        if (T) { fclose((FILE *)T); }
        pop();
        break;
    case 'O': ir = CODE[pc++]; 
        if (T) {
            char m[3] = {'r','b',0};
            if (ir == 'A') { m[0] = 'a'; }
            if (ir == 'R') { m[0] = 'r'; }
            if (ir == 'W') { m[0] = 'w'; }
            sprintf_s(input_fn, 24, "block.%03d", T);
            fopen_s((FILE **)&T, input_fn, m);
        }
        break;
    case 'R': if (T) {
            char buf[2];
            SIZE_T n = fread_s(buf, 2, 1, 1, (FILE *)T);
            T = ((n) ? buf[0] : 0);
        }
        break;
    case 'W': if (T) {
            FILE *fh = (FILE*)pop();
            char buf[2] = {0,0};
            buf[0] = (byte)pop();
            fwrite(buf, 1, 1, fh);
        }
        break;
    case 'N':
        push(0);
        ir = GetFunctionNum(pc);
        if (0 <= ir) { T = func[ir]; }
        pc += 2;
        break;
    }
    return pc;
}
#endif

int doQuote(int pc, int isPush) {
    char x[2];
    x[1] = 0;
    while ((pc < CODE_SZ) && (CODE[pc] != '"')) {
        x[0] = CODE[pc++];
        printString(x);
    }
    return ++pc;
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
        if (func[i]) {
            byte f1 = 'a' + (i / 36);
            byte f2 = 'a' + (i % 36);
            if ('z' < f1) { f1 -= 75; }
            if ('z' < f2) { f2 -= 75; }
            if (((n++) % 6) == 0) { printString("\r\n"); }
            printStringF("%c%c:%4d    ", f1, f2, func[i]);
        }
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
    printStringF("\r\nMEMORY: size: %d bytes (%d longs)", MEM_SZ*4, MEM_SZ);
    for (int i = 0; i < MEM_SZ; i++) {
        if (MEM[i] == 0) { continue; }
        long x = MEM[i];
        if (((n++) % 6) == 0) { printString("\r\n"); }
        printStringF("[%05d]: %-10ld  ", i, x);
        ++n;
    }
    if (n == 0) { printString("\r\n(all memory empty)"); }
}

void dumpAll() {
    dumpStack(1);  printString("\r\n");
    dumpCode();    printString("\r\n");
    dumpMemory();  printString("\r\n");
    dumpFuncs();   printString("\r\n");
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
    case 'F': pc = doFile(pc); break;
    case 'P': pc = doPin(pc); break;
    default: break;
    }
    return pc;
}

int step(int pc) {
    byte ir = CODE[pc++];
    byte* bMem = (byte*)(&MEM[0]);
    long t1, t2;
    switch (ir) {
    case 0: return -1;                                  // 0
    case ' ': break;                                    // 32
    case '!': t2 = pop(); t1 = pop();                   // 33
        if ((0 <= t2) && (t2 < MEM_SZ)) { MEM[t2] = t1; }
        break;
    case '"': input_fn[1] = 0;                          // 34
        while ((pc < CODE_SZ) && (CODE[pc] != '"')) {
            input_fn[0] = CODE[pc++];
            printString(input_fn);
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
    case '?': push(_getch());                   break;  // 63
    case '@': if ((0 <= T) && (T < MEM_SZ)) { T = MEM[T]; }
        break;
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z': ir = ir - 'A'; t1 = CODE[pc];
        push(ir);
        if (t1 == '+') { ++pc; ++MEM[ir]; }
        if (t1 == '-') { ++pc; --MEM[ir]; }
        break;

#ifdef __PC__
    case '[': rpush(pc);                                // 91
        if (T == 0) {
            while ((pc < CODE_SZ) && (CODE[pc] != ']')) { pc++; }
        }
        break;
    case ']': if (T) { pc = R; }                        // 93
            else { pop();  rpop(); }
            break;
    case '^': t1 = pop(); T ^= t1;      break;          // 94
    case '_': T = -T;                   break;          // 95
    case '`': pc = doExt(pc);           break;          // 96
    case 'c': ir = CODE[pc++];
        t1 = pop();
        if ((0 <= t1) && (t1 < MEM_SZB)) {
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
        if (t1 == 'M') { dumpMemory(); }
        if (t1 == 'S') { dumpStack(0); }
        break;
    case 'l':
        if (input_fp) { fclose(input_fp); }
        sprintf_s(input_fn, sizeof(input_fn), "block.%03ld", pop());
        fopen_s(&input_fp, input_fn, "rt");
        break;
#endif
    case 't': push(millis());  break;
    case 'w': delay(pop()); break;
    case 'x': t1 = CODE[pc++];
        if (t1 == 'A') { rpush(pc); pc = pop(); }
        if (t1 == 'X') { vmInit(); }
        if (t1 == 'Z') { isBye = 1; }
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
    printString("\r\nS4:"); dumpStack(0); printString(">");
}

#ifdef __PC__
void doHistory(const char *txt) {
    FILE* fp = NULL;
    fopen_s(&fp, "history.txt", "at");
    if (fp) {
        fprintf(fp, "%s", txt);
        fclose(fp);
    }
}

void loop() {
    char* tib = (char*)&CODE[TIB];
    FILE* fp = (input_fp) ? input_fp : stdin;
    if (fp == stdin) { s4(); }
    if (fgets(tib, TIB_SZ, fp) == tib) {
        if (fp == stdin) { doHistory(tib); }
        run(TIB);
        return;
    }
    if (input_fp) { 
        fclose(input_fp); 
        input_fp = NULL; 
    }
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
    input_fp = NULL;

    for (int i = 1; i < argc; i++)
    {
        char* cp = argv[i];
        if (*cp == '-') { process_arg(++cp); }
        else { strcpy_s(input_fn, sizeof(input_fn), cp); }
    }

    if (strlen(input_fn) > 0) {
        fopen_s(&input_fp, input_fn, "rt");
    }

    while (isBye == 0) { loop(); }
}
#endif
