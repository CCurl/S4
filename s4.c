// https://w3group.de/stable.html
//
// A big thanks to to Sandor Schneider for this.
// This is my personal reverse-engineered implementation.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

typedef unsigned short ushort;
typedef unsigned char byte;

#define CODE_SZ  1024
#define STK_SZ     63
#define NUM_VARS   32
#define NUM_FUNCS  52

long   dstack[STK_SZ + 1];
ushort rstack[STK_SZ + 1];
ushort dsp, rsp;

#define T dstack[dsp]
#define N dstack[dsp-1]

char input_fn[24];
long reg[26];
ushort func[NUM_FUNCS];
long var[NUM_VARS];
byte code[CODE_SZ];
ushort here = 0;
ushort curReg = 0;

void push(long v) { if (dsp < STK_SZ) { dstack[++dsp] = v; } }
long pop() { return (dsp > 0) ? dstack[dsp--] : 0; }

void rpush(ushort v) { if (rsp < STK_SZ) { rstack[++rsp] = v; } }
ushort rpop() { return (rsp > 0) ? rstack[rsp--] : -1; }

void parse(const char *src) {
    while ((*src) && (here < CODE_SZ)) {
        code[here++] = *(src++);
    }
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
    int ck = code[pc++];
    int fn = alpha(code[pc++]);
    int v = ((ck == 'f') && (0 <= fn) && (fn < NUM_FUNCS)) ? 1 : 0;
    if (v) { func[fn] = pc; }
    else { printf("-invalid function number (A:%c)-", 'a'-1+(NUM_FUNCS-26)); }
    while ((pc < here) && (code[pc++] != '}'));
    return pc;
}

int doFunc(int pc) {
    int f = alpha(code[pc++]);
    if ((0 <= f) && (f < NUM_FUNCS) && (func[f])) {
        rpush(pc);
        pc = func[f];
    }
    return pc;
}

void dumpStack() {
    printf("(");
    for (int i = 1; i <= dsp; i++) { printf(" %d", dstack[i]); }
    printf(" )");
}

int run(int pc) {
    long t1 = 0, t2 = 0;
    byte ir;
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
        case ']': if (pop()) { pc = rstack[rsp]; }
                else { rpop(); } break;
        case '(': if (pop() == 0) { while ((pc < here) && (code[pc] != ')')) { pc++; } } break;
        default: break;
        }
    }
    return 0;
}

void dumpCode() {
    printf("\nhere: %04d", here);
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

void process_arg(char* arg)
{
    if ((*arg == 'i') && (*(arg+1) == ':') )
    {
        arg = arg + 2;
        strcpy_s(input_fn, sizeof(input_fn), arg);
    }
    else if (*arg == '?')
    {
        printf("usage s4 [args] [source-file]\n");
        printf("  -i:file\n");
        printf("  -? - Prints this message\n");
        exit(0);
    }
    else
    {
        printf("unknown arg '-%s'\n", arg);
    }
}

int main(int argc, char** argv) {
    dsp = rsp = here = curReg = 0;
    for (int i = 0; i < CODE_SZ; i++) { code[i] = '}'; }
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
            here = (ushort)fread(code, 1, CODE_SZ, fp);
            fclose(fp);
        }
    }

    run(0);
    dumpCode();
    printf("\nstack: "); dumpStack();
}
