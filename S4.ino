#include "s4.h"

int _getch() { return (mySerial.available()) ? mySerial.read() : 0; }
void printString(const char* str) { mySerial.print(str); }
int doFile(int pc) { return pc; }

extern void loadCode(const char *);
extern void vmInit();
extern int run(int);
extern void s4();

extern byte code[];
extern long reg[];

#define CODE_SZ     (MEM_SZ*4)
#define NUM_REGS     26
#define NUM_FUNCS   (26*36)
#define TIB         (CODE_SZ-TIB_SZ-4)

typedef union {
    byte code[CODE_SZ];
    long mem[MEM_SZ];
} MEMORY_T;

extern MEMORY_T memory;
#define CODE memory.code

// ********************************************
// * HERE is where you load your default code *
// ********************************************

void loadCode(const char* src) {
    char* tgt = (char*)&CODE[TIB];
    while (*src) { *(tgt++) = *(src++); }
    *tgt = 0;
    run(TIB);
}

void loadBaseSystem() {
    loadCode("{MB 0(p--) :SC :RD :CQ (:RV :RA)}");
    loadCode("{SC 0(p--) p! p@ 100+ m! m@ M@ s! 0(todo xxx r!)}");
    loadCode("{RD 0(--)  p@ DR v!}");
    loadCode("{CQ 0(--f) m@ M@ v@ =}");
    loadCode("{RV 0(--)  v@ m@ M!}");
    loadCode("{RA 0(--)  todo}");
}

void setup() {
    mySerial.begin(19200);
    // while (!mySerial) {}
    // while (mySerial.available()) {}
    vmInit();
    loadBaseSystem();
    s4();
}

void loop() {
    static int iLed = 0;
    static ulong nextBlink = 0;
    static int ledState = LOW;
    static int tibEnd = TIB;
    ulong curTm = millis();
    
    if (iLed == 0) {
        iLed = 13;
        pinMode(iLed, OUTPUT);
    }
    if (nextBlink < curTm) {
        ledState = (ledState == LOW) ? HIGH : LOW;
        digitalWrite(iLed, ledState);
        nextBlink = curTm + 777;
    }

    while (mySerial.available()) {
        char c = mySerial.read();
        if (c == 13) {
            CODE[tibEnd] = (char)0;
            printString(" ");
            run(TIB);
            s4();
            tibEnd = TIB;
        }
        else {
            if (tibEnd < CODE_SZ) {
                CODE[tibEnd++] = (char)0;
                char b[2]; b[0] = c; b[1] = 0;
                printString(b);
            }
        }
    }
    if (reg[25]) { run((int)reg[25]); }    // autorun
}
