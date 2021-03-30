// https://w3group.de/stable.html
// Props go to Sandor Schneider for this.
// This is my personal reverse-engineered implementation.

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

typedef unsigned char BYTE;

#define CODE_SZ  1024
#define STK_SZ     63
#define NUM_VARS   32

#define T dstack[dsp]
#define N dstack[dsp-1]

long regs[26];
int funcs[26];
int extFuncs[100];
long vars[NUM_VARS];
long dstack[STK_SZ + 1]; int dsp;
int rstack[STK_SZ + 1]; int rsp;
BYTE code[CODE_SZ];
int here = 0;
int curReg = 0;

void push(long v) { if (dsp < STK_SZ) { dstack[++dsp] = v; } }
long pop() { return (dsp > 0) ? dstack[dsp--] : 0; }

void rpush(int v) { if (rsp < STK_SZ) { rstack[++rsp] = v; } }
long rpop() { return (rsp > 0) ? rstack[rsp--] : -1; }

void parse(BYTE* src) {
    while ((*src) && (here < CODE_SZ)) {
        code[here++] = *(src++);
    }
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
    int fn = code[pc++];
    if (fn == 'X') {
        pc = number(pc);
        long num = pop();
        if ((0 <= num) && (num < 100)) { extFuncs[num] = pc; } 
        else { 
            printf("-invalid function number: %d-", fn); exit(1); 
        }
    } else {
        fn -= 'A';
        if ((0 <= fn) || (fn < 26)) { funcs[fn] = pc; }
        else { 
            printf("-invalid function number: %d-", fn); exit(1); 
        }
    }
    while ((pc < here) && (code[pc++] != '}'));
    return pc;
}

void dumpStack() {
    printf("(");
    for (int i = 1; i <= dsp; i++) { printf(" %d", dstack[i]); }
    printf(" )");
}

int ext(int pc) {
    pc = number(pc);
    long num = pop();
    if ((0 <= num) && (num < 100) && (extFuncs[num])) {
        rpush(pc);
        pc = extFuncs[num];
    }
    return pc;
}

int run(int pc) {
    long t1 = 0, t2 = 0;
    BYTE ir;
    while (rsp >= 0) {
        if ((pc < 0) || (CODE_SZ <= pc)) { return 0; }
        ir = code[pc++];
        // printf("\npc:%04d, ir:%03d [%c] ", pc - 1, ir, ir); dumpStack();
        switch (ir) {
        case '{': pc = defineFunc(pc); break;
        case '}': pc = rpop(); break;
        case '#': push(T); break;
        case '$': t1 = pop(); t2 = pop(); push(t1); push(t2); break;
        case '@': push(N); break;
        case '\\': pop(); break;
        case ';': push(regs[curReg]); break;
        case ':': regs[curReg] = pop(); break;
        case '!': t1 = regs[curReg]; ((0 <= t1) && (t1 < NUM_VARS)) ? vars[t1] = pop() : pop(); break;
        case '?': t1 = regs[curReg]; ((0 <= t1) && (t1 < NUM_VARS)) ? push(vars[t1]) : push(0); break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            pc = number(pc - 1); break;
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
            curReg = ir - 'a';
            if (code[pc] == '+') { ++pc; regs[curReg]++; }
            if (code[pc] == '-') { ++pc; regs[curReg]--; }
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'Y': case 'Z':
            t1 = ir - 'A';
            if (funcs[t1]) { rpush(pc); pc = funcs[t1]; }
            break;
        case '+': if (dsp > 1) { t1 = pop(); T += t1; } break;
        case '-': if (dsp > 1) { t1 = pop(); T -= t1; } break;
        case '*': if (dsp > 1) { t1 = pop(); T *= t1; } break;
        case '/': if (dsp > 1) { t1 = pop(); T /= t1; } break;
        case '%': if (dsp > 1) { t1 = pop(); T %= t1; } break;
        case '|': if (dsp > 1) { t1 = pop(); T |= t1; } break;
        case '&': if (dsp > 1) { t1 = pop(); T &= t1; } break;
        case '_': if (dsp > 0) { T = -T; } break;
        case '~': if (dsp > 0) { T = ~T; } break;
        case '.': printf("%ld", pop());  break;
        case ',': printf("%c", (char)pop());  break;
        case '"': while ((code[pc] != '"') && (pc < here)) { printf("%c", code[pc]); pc++; } pc++; break;
        case '=': if (dsp > 1) { t1 = pop(); T = (T == t1) ? -1 : 0; } break;
        case '<': if (dsp > 1) { t1 = pop(); T = (T < t1) ? -1 : 0; } break;
        case '>': if (dsp > 1) { t1 = pop(); T = (T > t1) ? -1 : 0; } break;
        case '^':  push(_getch()); break;
        case '[': rpush(pc); if (T == 0) { while ((pc < here) && (code[pc] != ']')) { pc++; } } break;
        case ']': if (pop()) { pc = rstack[rsp]; } else { rpop(); } break;
        case '(': if (pop() == 0) { while ((pc < here) && (code[pc] != ')')) { pc++; } } break;
        case 'X': pc = ext(pc); break;
        default: break;
        }
    }
    return 0;
}

void dumpCode() {
    char* txt = (char*)&code[here + 10]; int ti = 0;
    for (int i = 0; i < here; i++) {
        if ((i % 20) == 0) {
            if (ti) { txt[ti] = 0;  printf(" ; %s", txt); ti = 0; }
            printf("\n%04d: ", i);
        }
        txt[ti++] = (code[i] < 32) ? '.' : code[i];
        printf(" %02x", code[i]);
    }
    if (ti) { txt[ti] = 0;  printf(" ; %s", txt); }
}

int main(int argc, char **argv) {
    rsp = 0;
    dsp = 0;
    here = 0;
    curReg = 0;
    for (int i = 0; i < CODE_SZ; i++) { code[i] = 0; }
    /* CR */ parse((BYTE*)"{R\"\r\n\"}R");
    // parse((BYTE *)"{S32,}{DS.}{A123a:456b:}A{Ba;b;}BDD{CB>D}C{CB<D}C{CB+D}CS65,66,334#=D");
    /* bench */    parse((BYTE*)"{K1000*}{X4[1-#]\\}R10KK\"S\"X4\"E\"}");
    /* if/else */  // parse((BYTE*)"0#(\"yes\"\\1_)~(\"no\")");
    /* fib */      // parse((BYTE*)"20a:0 1[#.9,$@+a-;]\\\\");
    /* count up */ // parse((BYTE*)"10[#10$-.32,1-#]\\");
    /* count dn */ // parse((BYTE*)"10[#.32,1-#]\\");
    parse((BYTE*)"R\"All done!\"R");
    dumpCode();
    run(0);
    dumpStack();
}
