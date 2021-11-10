// MINT - A Minimal Interpreter - for details, see https://github.com/monsonite/MINT

#define __PC__
#ifdef __PC__

#ifdef _WIN32
#define __WINDOWS__
#define  _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#else
#define __LINUX__
#endif

#include "s4.h"

FILE* input_fp;
byte fdsp = 0;
FILE* fstack[STK_SZ+1];
static char buf[256];
static CELL t1, t2;

void fpush(FILE* v) { if (fdsp < STK_SZ) { fstack[++fdsp] = v; } }
FILE* fpop() { return (fdsp) ? fstack[fdsp--] : 0; }

void printChar(const char c) { printf("%c", c); }
void printString(const char* str) { printf("%s", str); }

addr doBlock(addr pc) {
    t1 = *(pc++);
    switch (t1) {
    case 'C': if (T) {
            if ((FILE*)T == input_fp) { input_fp = fpop(); }
            fclose((FILE*)pop());
        } break;
    case 'L': if (input_fp) { fpush(input_fp); }
        sprintf(buf, "block-%03ld.s4", pop());
        input_fp = fopen(buf, "rb");
        break;
    case 'O': sprintf(buf, "block-%03ld.s4", pop());
        t1 = *(pc++);
        if (t1 == 'R') {
            push((CELL)fopen(buf, "rb"));
        } else if (t1 == 'W') {
            push((CELL)fopen(buf, "wb"));
        }
        break;
    case 'R': t1 = pop();
        if (t1) {
            t2 = fread(buf, 1, 1, (FILE *)t1);
            push(t2 ? buf[0] : 0);
            push(t2);
        } else {
            push(0);
            push(0);
        } 
        break;
    case 'W': t1 = pop(); t2 = pop();
        if (t1) {
            buf[0] = (char)t2;
            t2 = fwrite(buf, 1, 1, (FILE *)t1);
            push(t2 ? 1 : 0);
        }
        break;
    }
    return pc;
}

addr doCustom(byte ir, addr pc) {
    switch (ir) {
    case 'B': pc = doBlock(pc);        break;
    //case 'L': pc = doBlock(pc);        break;
    case 'N': printString("-noNano-");
        isError = 1;                   break;
#ifdef __WINDOWS__
    case 'T': push(GetTickCount());    break;
    case 'W': Sleep(pop());            break;
#else
    case 'T': push(0);        break;
    case 'W': pop();          break;
#endif
        default:
        isError = 1;
        printString("-notExt-");
    }
    return pc;
}

void ok() {
    printString("\r\nS4:(");
    dumpStack();
    printString(")>");
}

void rtrim(char* cp) {
    char* x = cp;
    while (*x) { ++x; }
    --x;
    while (*x && (*x < 32) && (cp <= x)) { *(x--) = 0; }
}

void loadCode(const char* src) {
    addr here = (addr)HERE;
    addr tib = here;
    while (*src) {
        *(tib++) = *(src++);
    }
    *tib = 0;
    run(here);
}

void doHistory(char* str) {
    FILE* fp = fopen("history.txt", "at");
    if (fp) {
        fputs(str, fp);
        fclose(fp);
    }
}

void loop() {
    FILE* fp = (input_fp) ? input_fp : stdin;
    if (fp == stdin) { ok(); }
    if (fgets(buf, 100, fp) == buf) {
        if (fp == stdin) { doHistory(buf); }
        rtrim(buf);
        loadCode(buf);
        return;
    }
    if (input_fp) {
        fclose(input_fp);
        input_fp = fpop(); // input_pop();
    }
}

int main(int argc, char** argv) {
    vmInit();
    loadCode("1`BL");
    while (!isBye) { loop(); }
    return 0;
}

#endif
