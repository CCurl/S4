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

CELL getSeed() { return millis(); }

addr doPinRead(addr pc) {
    byte ir = *(pc++);
    CELL pin = pop();
    switch (ir) {
    case 'A': push(analogRead(pin));          break;
    case 'D': push(digitalRead(pin));         break;
    default:
        isError = 1;
        printString("-pinRead-");
    }
    return pc;
}

addr doPinWrite(addr pc) {
    byte ir = *(pc++);
    CELL pin = pop();
    CELL val = pop();
    switch (ir) {
    case 'A': analogWrite(pin, val);          break;
    case 'D': digitalWrite(pin, val);         break;
    default:
        isError = 1;
        printString("-pinWrite-");
    }
    return pc;
}

addr doPin(addr pc) {
    CELL t1 = *(pc++);
    switch (t1) {
    case 'I': pinMode(pop(), INPUT);          break;
    case 'O': pinMode(pop(), OUTPUT);         break;
    case 'U': pinMode(pop(), INPUT_PULLUP);   break;
    case 'R': pc = doPinRead(pc);             break;
    case 'W': pc = doPinWrite(pc);            break;
    default:
        isError = 1;
        printString("-notPin-");
    }
    return pc;
}

addr doCustom(byte ir, addr pc) {
    switch (ir) {
    case 'N': push(micros());          break;
    case 'P': pc = doPin(pc);          break;
    case 'T': push(millis());          break;
    case 'W': delay(pop());            break;
    default:
        isError = 1;
        printString("-notExt-");
    }
    return pc;
}

void loadCode(const char* src) {
    addr here = HERE;
    addr here1 = HERE;
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

#define SOURCE_STARTUP \
    X(1000, ":C N`iAU`iAH@1-[i@`@#,';=(i@1+`@':=(N))];") \
    X(1001, ":N 13,10,;:B 32,;:Q @.B;:U `iH`iAU-1-.;") \
    X(1002, ":R 0`iR1-[i@4*`iAR+@#(Ni@26`/$26`/$'a+,'a+,'a+,\": \".1)\\];") \
    X(2000, "N\"This system has \"`iR.\" registers, \"`iF.\" functions, and \"`iU.\" bytes user memory.\"N")

#ifdef __ESP8266__
#define X(num, val) const char str ## num[] = val;
#else
#define X(num, val) const PROGMEM char str ## num[] = val;
#endif
SOURCE_STARTUP

#undef X
#define X(num, val) str ## num,
const char *bootStrap[] = {
    SOURCE_STARTUP
    NULL
};

void loadBaseSystem() {
    for (int i = 0; bootStrap[i] != NULL; i++) {
        loadCode(bootStrap[i]);
    }
}

void ok() {
    printString("\r\ns4:("); 
    dumpStack(); 
    printString(")>");
}

// NB: tweak this depending on what your terminal window sends for [back-space]
// PuTTY sends a 127 for back-space
int isBackSpace(char c) { 
  // printStringF("(%d)",c);
  return (c == 127) ? 1 : 0; 
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

    if (isBackSpace(c) && (here < here1)) {
        here1--;
        char b[] = {8, 32, 8, 0};
        printString(b);
        return;
    }
    if (c == 9) { c = 32; }
    if (BetweenI(c, 32, 126)) {
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
    wifiConnect();
    ok();
}

void loop() {
    static int iLed = 0;
    static long nextBlink = 0;
    static int ledState = LOW;
    long curTm = millis();
    
    if (iLed == 0) {
        loadBaseSystem();
        iLed = LED_BUILTIN;
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
