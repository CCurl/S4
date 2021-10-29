#include "config.h"
#include "s4.h"

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

int genTib() { return regVal(7) + 10; }

void loadCode(const char* src) {
    int tib = genTib();
    int i = tib;
    while (*src) { setCodeByte(i++, *(src++)); }
    setCodeByte(i, 0);
    run(tib);
}

void input_push(FILE *fp) { }
FILE *input_pop() { return NULL; }

// ********************************************
// * HERE is where you load your default code *
// ********************************************


void loadBaseSystem() {
    loadCode("`D hD0[IC@#,96=IDC@';=&(N)];`");
    loadCode("`I %%S~(\\\\~;)%<(\\;)PPGI;`");
    loadCode("`C 4c; 11 1{\\ #3 :I (c+\\) PP %% >}\\\\c ;`");
    loadCode("`B N\"# primes in \" #. \": \"T$ :C . T$- \" (\" . \" ms)\";`");
    loadCode("`L 256$ 1[#+# :B]\\;`");
    loadCode("D");
}

void ok() {
    printString("\r\ns4:"); 
    dumpStack(0); 
    printString(">");
}


void handleInput(char c) {
    static int tib = 0, HERE1 = 0;
    if (tib == 0) { 
        tib = genTib(); 
        HERE1 = tib; 
    }
    if (c == 13) {
        printString(" ");
        setCodeByte(HERE1, 0);
        run(tib);
        tib = 0;
        ok();
        return;
    }
    if ((c == 8) && (tib < HERE1)) {
        HERE1--;
        char b[] = {8, 32, 8, 0};
        printString(b);
        return;
    }
    if (c == 9) { c = 32; }
    if ((32 <= c) && (HERE1 < USER_SZ)) {
        setCodeByte(HERE1++, c);
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
