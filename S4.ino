#include "s4.h"

#define mySerial    Serial
#define __SERIAL__   1
#define ILED        13

#if __SERIAL__
    int charAvailable() { return mySerial.available(); }
    int getChar() { 
        while (!charAvailable()) {}
        return mySerial.read();
    }
    void printChar(char c) { mySerial.print(c); }
    void printString(const char* str) { mySerial.print(str); }
#else
    int charAvailable() { return 0; }
    int getChar() { return 0; }
    void printString(const char* str) { }
    void printChar(char c) { }
#endif

addr doCustom(byte ir, addr pc) {
  return pc;
}

void loadCode(const char* src) {
    addr here = (addr)HERE;
    addr here1 = here;
    while (*src) {
        *(here1++) = *(src++);
    }
    *here1 = 0;
    run(here);
}

void input_push(FILE *fp) { }
FILE *input_pop() { return NULL; }

// ********************************************
// * HERE is where you load your default code *
// ********************************************


void loadBaseSystem() {
    loadCode(":D u@h@1-[i@`@#58=(N),];");
    loadCode(":N 13,10,;:B32,;");
    loadCode(":R 0 25[Ni@#'a+,B4*r@+@.];");
}

void ok() {
    printString("\r\ns4:("); 
    dumpStack(); 
    printString(")>");
}


void handleInput(char c) {
    static addr here = (addr)NULL;
    static addr here1 = (addr)NULL;
    if (here == NULL) { 
        here = (addr)HERE; 
        here1 = here; 
    }
    if (c == 13) {
        printString(" ");
        *(here1) = 0;
        run(here);
        here = (addr)NULL;
        ok();
        return;
    }
    if ((c == 8) && (here < here1)) {
        here1--;
        char b[] = {8, 32, 8, 0};
        printString(b);
        return;
    }
    if (c == 9) { c = 32; }
    if (32 <= c) {
        *(here1++) = (byte)c;
        char b[] = {c, 0};
        printString(b);
    }
}

void setup() {
#ifdef __SERIAL__
    while (!mySerial) {}
    mySerial.begin(19200);
    while (mySerial.available()) { char c = mySerial.read(); }
#endif
    vmInit();
    loadBaseSystem();
    ok();
}

void loop() {
    static int iLed = 0;
    static long nextBlink = 0;
    static int ledState = LOW;
    long curTm = millis();
    
    if (iLed == 0) {
        iLed = ILED;
        pinMode(iLed, OUTPUT);
    }
    if (nextBlink < curTm) {
        ledState = (ledState == LOW) ? HIGH : LOW;
        digitalWrite(iLed, ledState);
        nextBlink = curTm + 1111;
    }

    while ( charAvailable() ) { handleInput(getChar()); }

    // addr a = functionAddress("R");
    // if (a) { run(a); }
}
