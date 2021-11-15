// MINT - A Minimal Interpreter - for details, see https://github.com/monsonite/MINT

#include "s4.h"

#ifdef __PC__

#ifdef __WINDOWS__
CELL millis() {
    return (CELL)GetTickCount();
}
CELL micros() {
    return (CELL)millis()*1000;
}
void delay(UCELL ms) { 
    Sleep(ms);
}
#else
CELL millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (CELL)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
}
CELL micros() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (CELL)(ts.tv_nsec);
}
void delay(UCELL ms) { 
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL); 
}
#endif

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
    case 'N': push(micros());          break;
    case 'T': push(millis());          break;
    case 'W': delay(pop());            break;
    case 'q': isBye = 1;               break;
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
