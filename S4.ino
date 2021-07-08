#include <Arduino.h>

extern void printString(const char *);
extern void loadCode(const char *);
extern void vmInit();
extern int run(int);
extern void s4();

extern int ihere;
extern byte code[];
extern long reg[];

// ********************************************
// * HERE is where you load your default code *
// ********************************************

#define iLed 13
ushort ihere = 0;
ulong nextBlink = 0;
int ledState = 0;

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
    ihere = TIB;
    pinMode(iLed, OUTPUT);
    loadBaseSystem();
    s4();
}

void loop() {
    ulong curTm = millis();
    if (nextBlink < curTm) {
        ledState = (ledState == LOW) ? HIGH : LOW;
        digitalWrite(iLed, ledState);
        nextBlink = curTm + 777;
    }

    while (mySerial.available()) {
        char c = mySerial.read();
        if (c == 13) {
            code[ihere] = (char)0;
            printString(" ");
            run(TIB);
            s4();
            ihere = TIB;
        }
        else {
            if (ihere < CODE_SZ) {
                code[ihere++] = c;
                char b[2]; b[0] = c; b[1] = 0;
                printString(b);
            }
        }
    }
    if (reg[25]) { run((int)reg[25]); }    // autorun
}
