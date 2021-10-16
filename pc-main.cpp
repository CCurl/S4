// pc-main.cpp - Main program for S4 when on a PC (Windows or Linux)

#include "config.h"

#ifdef __PC__
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "s4.h"

#ifdef _WIN32
long millis() { return (long)GetTickCount(); }
void delay(unsigned long ms) { Sleep(ms); }
#endif

#ifdef __LINUX__
#include <termios.h>
#include <sys/ioctl.h>
long millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000));
}

void delay(UCELL ms) { 
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL); 
}

int _kbhit() {
    static const int STDIN = 0;
    static bool initialized = false;

    if (! initialized) {
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        initialized = true;
    }
  
    int bytesWaiting;
    ioctl(STDIN, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}

int _getch() {
    static int init = 0;
    static struct termios cur_t;

    if (!init) {
        init = 1;
        tcgetattr( STDIN_FILENO, &cur_t);
        cur_t.c_lflag &= ~(ICANON);          
        tcsetattr( STDIN_FILENO, TCSANOW, &cur_t);

    }
    return getchar();
}
#endif

// These are used only be the PC version
#define INPUT_STACK_SZ 15
static char input_fn[32];
int input_sp = 0;
FILE *input_stack[INPUT_STACK_SZ+1];

// These are in the <Arduino.h> file
int analogRead(int pin) { printStringF("-AR(%d)-", pin); return 0; }
void analogWrite(int pin, int val) { printStringF("-AW(%d,%d)-", pin, val); }
int digitalRead(int pin) { printStringF("-DR(%d)-", pin); return 0; }
void digitalWrite(int pin, int val) { printStringF("-DW(%d,%d)-", pin, val); }
void pinMode(int pin, int mode) { printStringF("-pinMode(%d,%d)-", pin, mode); }
int getChar() { return _getch(); }
int charAvailable() { return _kbhit(); }
void printString(const char* str) { fputs(str, stdout); }

void input_push(FILE *fp) {
    if (input_sp < INPUT_STACK_SZ) { input_stack[++input_sp] = fp; }
    else { printString("-input stack-full-"); isError = 1; }
}

FILE *input_pop() {
    if (0 < input_sp) { return input_stack[input_sp--]; }
    else { return NULL; }
}

void printChar(const char ch) {
    char buf[] = {ch, 0};
    printString(buf);
}

void ok() {
    printString("\r\nS4:"); 
    dumpStack(0); 
    printString(">");
}

void doHistory(const char* txt) {
    FILE* fp = fopen("history.txt", "at");
    if (fp) {
        fprintf(fp, "%s", txt);
        fclose(fp);
    }
}

void executeString(addr loc, const char *txt) {
    addr x = loc;
    while (*txt) {
        setCodeByte(x++, *(txt++));
    }
    setCodeByte(x, 0);
    run(loc);
}

void loop() {
    char tib[100];
    addr nTib = USER_SZ - 100;
    FILE* fp = (input_fp) ? input_fp : stdin;
    if (fp == stdin) { ok(); }
    if (fgets(tib, 100, fp) == tib) {
        if (fp == stdin) { doHistory(tib); }
        executeString(nTib, tib);
        return;
    }
    if (input_fp) {
        fclose(input_fp);
        input_fp = input_pop();
    }
}

void process_arg(char* arg)
{
    if ((*arg == 'i') && (*(arg + 1) == ':')) {
        arg = arg + 2;
        strcpy(input_fn, arg);
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
    vmInit();
    // 0(Y: dump all code currently defined)
    executeString(100, "`DC XIF NNhQ0[I C@,I QC@';=I C@96=&(N)];`");

    input_fn[0] = 0;
    input_fp = NULL;

    for (int i = 1; i < argc; i++)
    {
        char* cp = argv[i];
        if (*cp == '-') { process_arg(++cp); }
        else { strcpy(input_fn, cp); }
    }

    if (strlen(input_fn) > 0) {
        input_fp = fopen(input_fn, "rt");
    }

    while (isBye == 0) { loop(); }
}

#endif // #define _WIN32
