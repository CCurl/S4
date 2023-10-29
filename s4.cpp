// S4 - A Minimal Interpreter

#include "s4.h"

SYS_T sys;
byte ir, isBye = 0, isError = 0;
short locBase, lastFunc;
static char buf[64];
addr pc, HERE;
CELL seed, t1;
addr func[MAX_HASH+1]; 
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
    for (int i = 0; i < NUM_FUNCS; i++) { func[i] = 0; }
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
        if ((to == '`') && (ir != to)) { continue; }
        if (ir == to) { return; }
        if (ir == '\'') { ++pc; continue; }
        if (ir == '(') { skipTo(')'); continue; }
        if (ir == '[') { skipTo(']'); continue; }
        if (ir == '"') { skipTo('"'); continue; }
        if (ir == '`') { skipTo('`'); continue; }
    }
    PERR("-skip-");
}

UCELL doHash(addr &a) {
    UCELL hash = 5381;
    while (BetweenI(*a, 'A', 'Z') || BetweenI(*a, 'a', 'z') || (*a == '_')) {
        hash = ((hash << 5) + hash) + *(a++);
    }
    return hash & MAX_HASH;
}

int regNum() {
    push(0);
    while (*pc) {
        t1 = BetweenI(*pc, 'A', 'Z') ? *(pc++) - 'A' : -1;
        if (t1 < 0) { break; }
        TOS = (TOS * 26) + t1;
    }
    if (NUM_REGS <= TOS) { pop(); PERR("-reg#-"); }
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
    case 'n': *pCell += CELL_SZ;  return;
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
    if (exp == 0) PERR("-0div-");
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
    next:
    if (isError || (!pc)) { return pc; }
#ifdef __WATCHDOG__
    feedWatchDog();
#endif
    ir = *(pc++);
    switch (ir) {
        case 0: return pc;
        NCASE ' ': while (BetweenI(*pc, 1, 32)) { pc++; }                 // 32
        NCASE '!': setCell(AOS, N); DROP2;                                // 33 (STORE)
        NCASE '"': while (*pc && (*pc != '"')) {                          // 34 PRINT
            ir = *(pc++); if (ir == '%') {
                ir = *(pc++);
                if (ir == 'c') { printChar((char)pop()); }
                else if (ir == 'd') { printStringF("%ld", pop()); }
                else if (ir == 'e') { printChar(27); }
                else if (ir == 'n') { printString("\r\n"); }
                else if (ir == 'q') { printChar('"'); }
                else if (ir == 'x') { printStringF("%lx", pop()); }
                else { printChar(ir); }
            }
            else { printChar(ir); }
        } ++pc;
        NCASE '#': push(TOS);                                             // 35 (DUP)
        NCASE '$': t1 = N; N = TOS; TOS = t1;                             // 36 (SWAP)
        NCASE '%': push(N);                                               // 37 (OVER)
        NCASE '&': if (isNot0(TOS)) { t1=TOS; TOS=N%t1; N/=t1; }          // 38 (/MOD)
        NCASE '\'': push(*(pc++));                                        // 39
        NCASE '(': if (pop() == 0) { skipTo(')'); }                       // 40 (IF)
        NCASE ')': /* endIf() */                                          // 41
        NCASE '*': t1 = pop(); TOS *= t1;                                 // 42
        NCASE '+': t1 = pop(); TOS += t1;                                 // 43
        NCASE ',': t1 = pop(); printChar((char)t1);                       // 44
        NCASE '-': t1 = pop(); TOS -= t1;                                 // 45
        NCASE '.': t1 = pop();  printStringF("%ld", t1);                  // 46
        NCASE '/': t1 = pop(); if (isNot0(t1)) { TOS /= t1; }             // 47
        NCASE '0': case '1': case '2': case '3': case '4':                // 48-57
        case  '5': case '6': case '7': case '8': case '9':
            push(ir - '0'); ir = *(pc);
            while (BetweenI(ir, '0', '9')) {
                TOS = (TOS * 10) + (ir - '0');
                ir = *(++pc);
            }
        NCASE ':': if (!BetweenI(*pc, 'A', 'Z')) { PERR("-FN-"); }             // 58
                t1=doHash(pc); if (func[t1]) { printStringF("-redef (%ld)-", t1); }
                func[t1] = pc; skipTo(';'); HERE = pc;
        NCASE ';': pc = rpop(); locBase -= 10;                            // 59
        NCASE '<': t1 = pop(); TOS = TOS < t1 ? 1 : 0;                    // 60
        NCASE '=': t1 = pop(); TOS = TOS == t1 ? 1 : 0;                   // 61
        NCASE '>': t1 = pop(); TOS = TOS > t1 ? 1 : 0;                    // 62
        NCASE '?': /* FREE */                                             // 63
        NCASE '@': TOS = getCell(AOS);                                    // 64
        NCASE 'A': case 'B': case 'C': case 'D': case 'E':                // 65-90
        case  'F': case 'G': case 'H': case 'I': case 'J':
        case  'K': case 'L': case 'M': case 'N': case 'O':
        case  'P': case 'Q': case 'R': case 'S': case 'T':
        case  'U': case 'V': case 'W': case 'X': case 'Y': 
        case  'Z': --pc; t1 = (CELL)doHash(pc);
                if (func[t1]) {
                    if (*pc != ';') { locBase += 10; rpush(pc); }
                    pc = (addr)func[t1];
                }
        NCASE '[': doFor();                                               // 91
        NCASE '\\': DROP1;                                                // 92 (DROP)
        NCASE ']': doNext();                                              // 93
        NCASE '^': t1 = pop(); if (isNot0(t1)) { TOS %= t1; }             // 94 (MODULO)
        NCASE '_': TOS = -TOS;                                            // 95 (NEGATE)
        NCASE '`': push(TOS);                                             // 96
            while (*pc && (*pc != ir)) { *(AOS++) = *(pc++); }
            *(AOS++) = 0; ++pc;
        NCASE 'a': NCASE 'e': NCASE 'f': NCASE 'g': NCASE 'j':  /* FREE */
        NCASE 'k': NCASE 'l': NCASE 'm': NCASE 'o':             /* FREE */
        NCASE 'p': NCASE 'q': NCASE 't': NCASE 'u': NCASE 'v':  /* FREE */
        NCASE 'y': NCASE 'z': /* FREE */
        NCASE 'b': ir = *(pc++);                                           // BIT ops
            if (ir == '&') { N &= TOS; DROP1; }                            // AND
            else if (ir == '|') { N |= TOS; DROP1; }                       // OR
            else if (ir == '^') { N ^= TOS; DROP1; }                       // XOR
            else if (ir == '~') { TOS = ~TOS; }                            // NOT
        NCASE 'c': ir = *(pc++);                                           // c! / c@
            if (ir == '!') { *AOS = (byte)N; DROP2; }
            if (ir == '@') { TOS = *AOS; }
        NCASE 'h': push(0); t1 = 1; while (0 <= t1) {
                t1 = BetweenI(*pc, '0', '9') ? (*pc - '0') : -1;
                t1 = BetweenI(*pc, 'A', 'F') ? (*pc - 'A') + 10 : t1;
                if (0 <= t1) { TOS = (TOS * 0x10) + t1; ++pc; }
            }
        NCASE 'd': case 'i': case 'r': case 's': case 'n': doRegOp(ir);
        NCASE 'w': ir = *(pc++);                                                // w! / w@
            if (ir == '!') { *AOS = N & 255; *(AOS+1) = (N/256)&255; DROP2;}
            if (ir == '@') { TOS = (*(AOS+1)*256) | *(AOS); }
        NCASE 'x': doExt();
        NCASE '{': if (TOS) {                                                   // 123
                lpush()->start = pc;
                LTOS->end = 0;
                LTOS->from = 1;
                LTOS->to = 0;
            } else { DROP1;  skipTo('}'); }
        NCASE '|': loopBreak();                                                 // 124
        NCASE '}': if (!TOS) { ldrop(); DROP1; }                                // 125
            else { LTOS->end = pc; pc = LTOS->start; }
        NCASE '~': TOS = (TOS) ? 0 : 1;                                         // 126
            goto next;
        default: printStringF("-ir:[%d]?-", ir);
    }
    return pc;
}
