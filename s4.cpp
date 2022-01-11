// S4 - A Minimal Interpreter

#include "s4.h"

SYS_T sys;
byte ir, isBye = 0, isError = 0;
static char buf[64];
addr pc, HERE;
CELL t1;

void push(CELL v) { if (sys.dsp < STK_SZ) { sys.dstack[++sys.dsp] = v; } }
CELL pop() { return (sys.dsp) ? sys.dstack[sys.dsp--] : 0; }

inline void rpush(addr v) { if (sys.rsp < STK_SZ) { sys.rstack[++sys.rsp] = v; } }
inline addr rpop() { return (sys.rsp) ? sys.rstack[sys.rsp--] : 0; }

inline LOOP_ENTRY_T* lpush() { if (LSP < STK_SZ) { ++LSP; } return LTOS; }
inline LOOP_ENTRY_T *ldrop() { if (0 < LSP) { --LSP; } return LTOS; }

void vmInit() {
    sys.dsp = sys.rsp = sys.lsp = 0;
    for (int i = 0; i < NUM_REGS; i++) { REG[i] = 0; }
    for (int i = 0; i < USER_SZ; i++) { USER[i] = 0; }
    for (int i = 0; i < NUM_FUNCS; i++) { FUNC[i] = 0; }
    HERE = USER;
}

void setCell(byte* to, CELL val) {
#ifdef _NEEDS_ALIGN_
    *(to++) = (byte)val; 
    for (int i = 1; i < CELL_SZ; i++) {
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
    for (int i = 0; i < CELL_SZ; i++) {
        val = (val << 8) + *(from--);
    }
#else
    val = *((CELL *)from);
#endif
    return val;
}

void dumpStack() {
    for (UCELL i = 1; i <= sys.dsp; i++) {
        printStringF("%s%ld", (i > 1 ? " " : ""), (CELL)sys.dstack[i]);
    }
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

int regFuncNum(int isReg) {
    push(0);
    while (*pc) {
        if (isReg) { t1 = BetweenI(*pc, 'a', 'z') ? *pc - 'a' : -1; }
        else { t1 = BetweenI(*pc, 'A', 'Z') ? *pc - 'A' : -1; }
        if (t1 < 0) { break; }
        TOS = (TOS*26) + t1;
        ++pc;
    }
    if ( isReg && (NUM_REGS  <= TOS)) { isError = 1; printString("-reg#-"); }
    if (!isReg && (NUM_FUNCS <= TOS)) { isError = 1; printString("-func#-"); }
    return isError ? 0 : 1;
}

void doFor() {
    CELL f = (N < TOS) ? N : TOS;
    CELL t = (N < TOS) ? TOS : N;
    DROP2;
    LOOP_ENTRY_T *x = lpush();
    x->start = pc;
    INDEX = x->from = f;
    x->to = t;
    x->end = 0;
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

void doRand(int modT) {
    static CELL seed = 0;
    if (seed == 0) { seed = getSeed(); }
    seed ^= (seed << 13);
    seed ^= (seed >> 17);
    seed ^= (seed << 5);
    TOS = (modT && TOS) ? (abs(seed) % TOS) : seed;
}

void doExt() {
    ir = *(pc++);
    switch (ir) {
    case '!': *(byte*)TOS = (byte)N; DROP2;                return;
    case '-': TOS = -TOS;                                  return;
    case '/': if (TOS) { t1 = TOS; TOS = N % t1; N /= t1; }
        else { isError = 1; printString("-0div-"); }       return;
    case '%': if (TOS) { N %= TOS; DROP1; }
        else { isError = 1; printString("-0div-"); }       return;
    case '@': TOS = *(byte *)TOS;                          return;
    case 'A': TOS = (TOS < 0) ? -TOS : TOS;                return;
    case 'R': doRand(1);                                   return;
    case 'F': ir = *(pc++);
        if (ir == 'O') { fileOpen();  }
        if (ir == 'C') { fileClose(); }
        if (ir == 'R') { fileRead();  }
        if (ir == 'W') { fileWrite(); }
        if (ir == 'L') { fileLoad();  }
        if (ir == 'S') { fileSave();  }
        return;
    case 'C': if (TOS) { rpush(pc); }                   // fall thru to 'J'
    case 'J': if (TOS) { pc = (addr)TOS; } DROP1;          return;
    case 'K': ir = *(pc++);
        if (ir == '?') { push(charAvailable()); }
        if (ir == '@') { push(getChar()); }
        return;
    case 'Z': printString((char *)pop());                  return;
    case 'i': ir = *(pc++);
        if (ir == 'A') {
          ir = *(pc++);
          if (ir == 'F') { push((CELL)&FUNC[0]); }
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
    case 's': if (*(pc++) == 'R') { vmInit();   }          return;
    default:
        pc = doCustom(ir, pc);
    }
}

addr run(addr start) {
    pc = start;
    isError = 0;
    RSP = LSP = 0;
    while (!isError && pc) {
        ir = *(pc++);
        switch (ir) {
        case 0: return pc;
        case ' ': while (*(pc) == ' ') { pc++; }           break;  // 32
        case '!': setCell((byte*)TOS, N); DROP2;           break;  // 33
        case '"': while (*(pc) != ir) { printChar(*(pc++)); };      // 34
                ++pc; break;
        case '#': push(TOS);                               break;  // 35 (DUP)
        case '$': t1 = N; N = TOS; TOS = t1;               break;  // 36 (SWAP)
        case '%': push(N);                                 break;  // 37 (OVER)
        case '&': t1 = pop(); TOS &= t1;                   break;  // 38
        case '\'': push(*(pc++));                          break;  // 39
        case '(': if (pop() == 0) { skipTo(')'); }         break;  // 40 (IF)
        case ')': /* endIf() */                            break;  // 41
        case '*': t1 = pop(); TOS *= t1;                   break;  // 42
        case '+': t1 = pop(); TOS += t1;                   break;  // 43
        case ',': printChar((char)pop());                  break;  // 44
        case '-': t1 = pop(); TOS -= t1;                   break;  // 45
        case '.': printStringF("%ld", (CELL)pop());        break;  // 46
        case '/': if (TOS) { N /= TOS; DROP1; }                    // 47
                else { isError = 1;  printString("-0div-"); }
                break;
        case '0': case '1': case '2': case '3': case '4':          // 48-57
        case '5': case '6': case '7': case '8': case '9':
            push(ir - '0'); ir = *(pc);
            while (BetweenI(ir, '0', '9')) {
                TOS = (TOS * 10) + (ir - '0');
                ir = *(++pc);
            } break;
        case ':': if (regFuncNum(0)) {
            FUNC[pop()] = pc; skipTo(';'); HERE = pc;
        }; break;
        case ';': pc = rpop();                             break;  // 59
        case '<': t1 = pop(); TOS = TOS < t1 ? 1 : 0;      break;  // 60
        case '=': t1 = pop(); TOS = TOS == t1 ? 1 : 0;     break;  // 61
        case '>': t1 = pop(); TOS = TOS > t1 ? 1 : 0;      break;  // 62
        case '?': /* FREE */                               break;  // 63
        case '@': TOS = getCell((byte*)TOS);               break;  // 64
        case 'A': case 'B': case 'C': case 'D': case 'E':          // 65-90
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': 
        case 'Z': --pc;
            if (regFuncNum(0) && FUNC[TOS]) {
                if (*pc != ';') { rpush(pc); }
                pc = (addr)FUNC[TOS];
            }  pop(); break;
        case '[': doFor();                                 break;  // 91
        case '\\': DROP1;                                  break;  // 92
        case ']': doNext();                                break;  // 93
        case '^': t1 = pop(); TOS ^= t1;                   break;  // 94
        case '_': t1 = TOS;
            while (*pc && *pc != '_') { *((addr)t1++) = *(pc++); }
            *((addr)t1++) = 0; push(t1);
            ++pc;                                          break;  // 95
        case '`': doExt();                                 break;  // 96
        case 'a': case 'b': case 'c': case 'd': case 'e':          // 97-122
        case 'f': case 'g': case 'h': case 'i': case 'j': 
        case 'k': case 'l': case 'm': case 'n': case 'o': 
        case 'p': case 'q': case 'r': case 's': case 't': 
        case 'u': case 'v': case 'w': case 'x': case 'y': 
        case 'z': --pc;
            if (regFuncNum(1)) { TOS = (CELL)&REG[TOS]; }  break;
        case '{': if (TOS) { lpush()->start = pc; }                // 123
                else { DROP1;  skipTo('}'); }              break;
        case '|': t1 = pop(); TOS |= t1;                   break;  // 124
        case '}': if (!TOS) { ldrop(); DROP1; }                    // 125
                else { LTOS->end = pc; pc = LTOS->start; }
            break;
        case '~': TOS = ~TOS;                              break;  // 126
        }
        #ifdef __WATCHDOG__
        feedWatchDog();
        #endif
    }
    return pc;
}
