// S4 - A Minimal Interpreter

#include "s4.h"

CELL millis() {
    return (CELL)clock()/1000;
}
CELL micros() {
    return (CELL)clock();
}

#if __BOARD__ == PC

#ifdef __WINDOWS__
void delay(UCELL ms) { 
    Sleep((DWORD)ms);
}

int charAvailable() { return _kbhit(); }
int getChar() { return _getch(); }

#else
int charAvailable() { return 0; }
int getChar() { return 0; }

void delay(UCELL ms) { 
    usleep(ms*1000); 
}
#endif

static char buf[256];
static CELL t1, t2;

void printChar(const char c) { printf("%c", c); }
void printString(const char* str) { printf("%s", str); }


CELL getSeed() { return millis(); }

addr doCustom(byte ir, addr pc) {
    switch (ir) {
    case 'N': push(micros());          break;
    case 'T': push(millis());          break;
    case 'W': delay(pop());            break;
    case 'Q': isBye = 1;               break;
    default:
        isError = 1;
        printString("-notExt-");
    }
    return pc;
}

void ok() {
    printString("\r\nS4:");
    dumpStack();
    printString(">");
}

void rtrim(char* cp) {
    char* x = cp;
    while (*x) { ++x; }
    --x;
    while (*x && (*x < 32) && (cp <= x)) { *(x--) = 0; }
}

void loadCode(const char* src) {
    addr here = HERE;
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
    FILE* fp = stdin;
    if (input_fp) { fp = (FILE *)input_fp; }
    if (fp == stdin) { ok(); }
    if (fgets(buf, sizeof(buf), fp) == buf) {
        if (fp == stdin) { doHistory(buf); }
        rtrim(buf);
        loadCode(buf);
        return;
    }
    if (input_fp) {
        fclose((FILE *)input_fp);
        input_fp = fpop();
    }
}

int main(int argc, char** argv) {
    vmInit();
    loadCode("0xBL");
    while (!isBye) { loop(); }
    return 0;
}

#endif
