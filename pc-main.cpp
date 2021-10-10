#ifdef _WIN32

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "s4.h"

typedef unsigned char byte;

// These are used only be the PC version
static HANDLE hStdOut = 0;
static char input_fn[32];

// These are in the <Arduino.h> file
long millis() { return GetTickCount(); }
int analogRead(int pin) { printStringF("-AR(%d)-", pin); return 0; }
void analogWrite(int pin, int val) { printStringF("-AW(%d,%d)-", pin, val); }
int digitalRead(int pin) { printStringF("-DR(%d)-", pin); return 0; }
void digitalWrite(int pin, int val) { printStringF("-DW(%d,%d)-", pin, val); }
void pinMode(int pin, int mode) { printStringF("-pinMode(%d,%d)-", pin, mode); }
void delay(DWORD ms) { Sleep(ms); }

void printString(const char* str) {
    DWORD n = 0, l = strlen(str);
    if (l) { WriteConsoleA(hStdOut, str, l, &n, 0); }
}

void ok() {
    printString("\r\nS4:"); dumpStack(0); printString(">");
}

void doHistory(const char* txt) {
    FILE* fp = NULL;
    fopen_s(&fp, "history.txt", "at");
    if (fp) {
        fprintf(fp, "%s", txt);
        fclose(fp);
    }
}

void strToTIB(addr loc, const char *txt) {
    while (*txt) {
        setCodeByte(loc++, *(txt++));
    }
    setCodeByte(loc++, 0);
}

void loop() {
    char tib[100];
    addr nTib = CODE_SZ - 100;
    FILE* fp = (input_fp) ? input_fp : stdin;
    if (fp == stdin) { ok(); }
    if (fgets(tib, 100, fp) == tib) {
        if (fp == stdin) { doHistory(tib); }
        strToTIB(nTib, tib);
        run(nTib);
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
    // 0(111: dump all code currently defined)
    strToTIB(100, "{a IFNNhQ0[XID@#,125=(N)]}");
    run(100);

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

#endif // #define _WIN32
