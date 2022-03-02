// S4 - A Minimal Interpreter

#include "s4.h"

SYS_T sys;
byte ir, isBye = 0, isError = 0;
short locBase, lastFunc;
static char buf[64];
addr pc, HERE;
CELL seed, t1;
FUNC_T func[NUM_FUNCS]; 
CELL locs[STK_SZ * 10];

void push(CELL v) { if (DSP < STK_SZ) { sys.dstack[++DSP] = v; } }
CELL pop() { return (DSP) ? sys.dstack[DSP--] : 0; }

inline void rpush(addr v) { if (RSP < STK_SZ) { sys.rstack[++RSP] = v; } }
inline addr rpop() { return (RSP) ? sys.rstack[RSP--] : 0; }

inline LOOP_ENTRY_T* lpush() { if (LSP < STK_SZ) { ++LSP; } return LTOS; }
inline LOOP_ENTRY_T *ldrop() { if (0 < LSP) { --LSP; } return LTOS; }

void vmInit() {
    seed = DSP = RSP = LSP = lastFunc = 0;
    for (int i = 0; i < NUM_REGS; i++) { REG[i] = 0; }
    for (int i = 0; i < USER_SZ; i++) { USER[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { func[i].hash = 0; }
    HERE = USER;
    REG[21] = (CELL) (USER + (USER_SZ / 2)); // REG v
}

void setCell(byte* to, CELL val) {
#ifdef _NEEDS_ALIGN_
    *(to++) = (byte)val; 
    for (uint i = 1; i < CELL_SZ; i++) {
        val = (val >> 8);
        *(to++) = (byte)val;
    }
#else
    *((CELL *)to) = val;
#endif
}

CELL getCell(byte* from) {
    CELL val = 0;
#ifdef _NEEDS_ALIGN_
    from += (CELL_SZ - 1);
    for (uint i = 0; i < CELL_SZ; i++) {
        val = (val << 8) + *(from--);
    }
#else
    val = *((CELL *)from);
#endif
    return val;
}

void dumpStack() {
    printChar('(');
    for (UCELL i = 1; i <= DSP; i++) {
        printStringF("%s%ld", (i > 1 ? " " : ""), (CELL)sys.dstack[i]);
    }
    printChar(')');
}

void printStringF(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printString(buf);
}

void skipTo(byte to) {
    while (*pc) {
        ir = *(pc++);
        if ((to == '"') && (ir != to)) { continue; }
        if (ir == to) { return; }
        if (ir == '\'') { ++pc; continue; }
        if (ir == '(') { skipTo(')'); continue; }
        if (ir == '[') { skipTo(']'); continue; }
        if (ir == '"') { skipTo('"'); continue; }
    }
    isError = 1;
}

addr findFunc(UCELL hash, addr vector) {
    if (isError) { return 0; }
    for (int i = lastFunc-1; 0 <= i; i--) {
        if (func[i].hash == hash) { 
            if (vector) { func[i].val = vector; }
            return func[i].val; 
        }
    }
    return 0;
}

void addFunc(UCELL hash) {
    // DEBUG: report if the hash is already defined
    if (findFunc(hash, pc)) { printStringF("-redef (%ld)-", hash);  return; }
    if (NUM_FUNCS <= lastFunc) { isError = 1; printString("-oof-"); }
    if (isError) { return; }
    func[lastFunc].hash = hash;
    func[lastFunc++].val = pc;
}

UCELL funcNum(addr &a) {
    UCELL hash = 5381;
    while (*a && BetweenI(*a, 'A', 'Z')) {
        hash = ((hash << 5) + hash) + *(a++);
    }
    return hash;
}

int regNum() {
    push(0);
    while (*pc) {
        t1 = BetweenI(*pc, 'A', 'Z') ? *(pc++) - 'A' : -1;
        if (t1 < 0) { break; }
        TOS = (TOS * 26) + t1;
    }
    if (NUM_REGS <= TOS) { pop(); isError = 1; printString("-reg#-"); }
    return isError ? 0 : 1;
}

void doRegOp(int op) {
    CELL *pCell = 0;
    if (BetweenI(*pc, '0', '9')) { pCell = &locs[locBase + (*(pc++) - '0')];} 
    else { if (regNum()) { pCell = &REG[pop()]; } }
    if (!pCell) { return; }
    switch (op) {
    case 'd': (*pCell)--;         return;
    case 'i': (*pCell)++;         return;
    case 'r': push(*pCell);       return;
    case 's': *pCell = pop();     return;
    }
}

void loopBreak() {
    if (LSP == 0) { return; }
    LOOP_ENTRY_T* x = LTOS;
    int isFor = (x->from <= x->to) ? 1 : 0;
    if (isFor) { INDEX = ldrop()->from; }
    else { ldrop(); }
    if (x->end) { pc = x->end; }
    else { skipTo(isFor ? ']' : '}'); }
}

void doWhile(){
    lpush()->start = pc;
    LTOS->end = 0;
    LTOS->from = 1;
    LTOS->to = 0;
}

void doFor() {
    CELL f = (N < TOS) ? N : TOS;
    CELL t = (N < TOS) ? TOS : N;
    DROP2;
    LOOP_ENTRY_T *x = lpush();
    x->start = pc;
    x->end = 0;
    INDEX = x->from = f;
    x->to = t;
}

void doNext() {
    LOOP_ENTRY_T* x = LTOS;
    x->from = ++INDEX;
    if (x->from <= x->to) {
        x->end = pc;
        pc = x->start;
    } else {
        INDEX = ldrop()->from;
    }
}

int isNot0(int exp) {
    if (exp == 0) { isError = 1; printString("-0div-"); }
    return isError == 0;
}

void doExt() {
    ir = *(pc++);
    switch (ir) {
    case 'A': TOS = (TOS < 0) ? -TOS : TOS;                return;
    case 'R': if (seed == 0) { seed = getSeed(); }         // RAND
        seed ^= (seed << 13);
        seed ^= (seed >> 17);
        seed ^= (seed << 5);
        TOS = (TOS) ? (abs(seed) % TOS) : seed;            return;
    case 'B': ir = *(pc++);
        if (ir == 'O') { blockOpen(); }
        if (ir == 'R') { blockRead(); }
        if (ir == 'W') { blockWrite(); }
        if (ir == 'L') { blockLoad(); }
        return;
    case 'E': doEditor();                                  return;
    case 'F': ir = *(pc++);
        if (ir == 'O') { fileOpen(); }
        if (ir == 'C') { fileClose(); }
        if (ir == 'D') { fileDelete(); }
        if (ir == 'R') { fileRead(); }
        if (ir == 'W') { fileWrite(); }
        if (ir == 'L') { fileLoad(); }
        if (ir == 'S') { fileSave(); }
        return;
    case 'J': if (TOS) { pc = AOS; } DROP1;                return;
    case 'K': ir = *(pc++);
        if (ir == '?') { push(charAvailable()); }
        if (ir == '@') { push(getChar()); }
        return;
    case 'Z': printString((char *)pop());                  return;
    case 'I': ir = *(pc++);
        if (ir == 'A') {
          ir = *(pc++);
          if (ir == 'F') { push((CELL)&func[0]); }
          if (ir == 'H') { push((CELL)&HERE); }
          if (ir == 'R') { push((CELL)&REG[0]); }
          if (ir == 'S') { push((CELL)&sys); }
          if (ir == 'U') { push((CELL)&USER[0]); }
          return;
        }
        if (ir == 'C') { push(CELL_SZ); }
        if (ir == 'F') { push(NUM_FUNCS); }
        if (ir == 'H') { push((CELL)HERE); }
        if (ir == 'R') { push(NUM_REGS); }
        if (ir == 'U') { push(USER_SZ); }
        return;
    case 'S': ir = *(pc++);
        if (ir == 'R') { vmInit();   }
        if (ir == '.') { dumpStack(); }
            return;
    default:
        pc = doCustom(ir, pc);
    }
}

addr run(addr start) {
    pc = start;
    locBase = isError = 0;
    RSP = LSP = 0;
    while (!isError && pc) {
        ir = *(pc++);
        switch (ir) {
        case 0: return pc;
        case ' ': while (BetweenI(*pc, 1, 32)) { pc++; }               break;  // 32
        case '!': setCell(AOS, N); DROP2;                              break;  // 33 (STORE)
        case '"': while (*pc && (*pc != '"')) {                                // 34 PRINT
            ir = *(pc++); if (ir == '%') {
                ir = *(pc++);
                if (ir == 'c') { printChar((char)pop()); }
                else if (ir == 'd') { printStringF("%ld", pop()); }
                else if (ir == 'n') { printString("\r\n"); }
                else { printChar(ir); }
            }
            else { printChar(ir); }
        } ++pc;   break;  // 34
        case '#': push(TOS);                                           break;  // 35 (DUP)
        case '$': t1 = N; N = TOS; TOS = t1;                           break;  // 36 (SWAP)
        case '%': push(N);                                             break;  // 37 (OVER)
        case '&': if (isNot0(TOS)) { t1=TOS; TOS=N%t1; N/=t1; }        break;  // 38 (/MOD)
        case '\'': push(*(pc++));                                      break;  // 39
        case '(': if (pop() == 0) { skipTo(')'); }                     break;  // 40 (IF)
        case ')': /* endIf() */                                        break;  // 41
        case '*': t1 = pop(); TOS *= t1;                               break;  // 42
        case '+': t1 = pop(); TOS += t1;                               break;  // 43
        case ',': t1 = pop(); printChar((char)t1);                     break;  // 44
        case '-': t1 = pop(); TOS -= t1;                               break;  // 45
        case '.': t1 = pop();  printStringF("%ld", t1);                break;  // 46
        case '/': t1 = pop(); if (isNot0(t1)) { TOS /= t1; }           break;  // 47
        case '0': case '1': case '2': case '3': case '4':                      // 48-57
        case '5': case '6': case '7': case '8': case '9':
            push(ir - '0'); ir = *(pc);
            while (BetweenI(ir, '0', '9')) {
                TOS = (TOS * 10) + (ir - '0');
                ir = *(++pc);
            } break;
        case ':': addFunc(funcNum(pc)); skipTo(';'); HERE = pc;        break;  // 58
        case ';': pc = rpop(); locBase -= 10;                          break;  // 59
        case '<': t1 = pop(); TOS = TOS < t1 ? 1 : 0;                  break;  // 60
        case '=': t1 = pop(); TOS = TOS == t1 ? 1 : 0;                 break;  // 61
        case '>': t1 = pop(); TOS = TOS > t1 ? 1 : 0;                  break;  // 62
        case '?': /* FREE */                                           break;  // 63
        case '@': TOS = getCell(AOS);                                  break;  // 64
        case 'A': case 'B': case 'C': case 'D': case 'E':                      // 65-90
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': 
        case 'Z': --pc;
                t1 = (CELL)findFunc(funcNum(pc), 0);
                if (t1) {
                    if (*pc != ';') { locBase += 10; rpush(pc); }
                    pc = (addr)t1;
                } break;
        case '[': doFor();                                             break;  // 91
        case '\\': DROP1;                                              break;  // 92 (DROP)
        case ']': doNext();                                            break;  // 93
        case '^': t1 = pop(); if (isNot0(t1)) { TOS %= t1; }           break;  // 94 (MODULO)
        case '_': TOS = -TOS;                                          break;  // 95 (NEGATE)
        case '`': push(TOS);                                                   // 96
            while (*pc && (*pc != ir)) { *(AOS++) = *(pc++); }
            *(AOS++) = 0; ++pc;                                        break;
        case 'a': case 'e': case 'f': case 'g': case 'j':  /* FREE */  break;  // 97-122
        case 'k': case 'l': case 'm': case 'n': case 'o':  /* FREE */  break;
        case 'p': case 'q': case 't': case 'u': case 'v':  /* FREE */  break;
        case 'b': ir = *(pc++);                                                // BIT ops
            if (ir == '&') { N &= TOS; DROP1; }                                // AND
            if (ir == '|') { N |= TOS; DROP1; }                                // OR
            if (ir == '^') { N ^= TOS; DROP1; }                                // XOR
            if (ir == '~') { TOS = ~TOS; }                             break;  // NOT
        case 'c': ir = *(pc++);                                                // c! / c@
            if (ir == '!') { *AOS = (byte)N; DROP2; }
            if (ir == '@') { TOS = *AOS; }                             break;
        case 'h': push(0); t1 = 1; while (0 <= t1) {
                t1 = BetweenI(*pc, '0', '9') ? (*pc - '0') : -1;
                t1 = BetweenI(*pc, 'A', 'F') ? (*pc - 'A') + 10 : t1;
                if (0 <= t1) { TOS = (TOS * 0x10) + t1; ++pc; }
            } break;
        case 'd': case 'i': case 'r': case 's': doRegOp(ir);           break;
        case 'w': ir = *(pc++);                                              // w! / w@
            if (ir == '!') { *AOS = N & 255; *(AOS+1) = (N/256)&255; DROP2;}
            if (ir == '@') { TOS = (*(AOS+1)*256) | *(AOS); }
            break;
        case 'x': doExt();                                             break;
        case 'y':  case 'z':                                           break;
        case '{': if (TOS) { doWhile(); }                                      // 123
                else { DROP1;  skipTo('}'); }                          break;
        case '|': loopBreak();                                         break;  // 124
        case '}': if (!TOS) { ldrop(); DROP1; }                                // 125
                else { LTOS->end = pc; pc = LTOS->start; }             break;
        case '~': TOS = (TOS) ? 0 : 1;                                 break;  // 126
        }
        #ifdef __WATCHDOG__
        feedWatchDog();
        #endif
    }
    return pc;
}
