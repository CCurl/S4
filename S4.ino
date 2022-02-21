#include "s4.h"

#if __SERIAL__
    int charAvailable() { return mySerial.available(); }
    int getChar() { 
        while (!charAvailable()) {}
        return mySerial.read();
    }
    void printSerial(const char* str) { mySerial.print(str); }
#else
    int charAvailable() { return 0; }
    int getChar() { return 0; }
    void printSerial(const char* str) { }
#endif

int isOTA = 0;

void printString(const char* str) { 
    if (isOTA) { printWifi(str); }
    else { printSerial(str); }
}

void printChar(const char ch) { 
    char b[2] = { ch, 0 };
    printString(b);
}

CELL getSeed() { return millis(); }

addr doPin(addr pc) {
    CELL pin = pop();
    byte ir = *(pc++);
    switch (ir) {
    case 'I': pinMode(pin, INPUT);          break;
    case 'O': pinMode(pin, OUTPUT);         break;
    case 'U': pinMode(pin, INPUT_PULLUP);   break;
    case 'R': ir = *(pc++);
        if (ir == 'A') { push(analogRead(pin));  }
        if (ir == 'D') { push(digitalRead(pin)); }
        break;
    case 'W': ir = *(pc++);
        if (ir == 'A') { analogWrite(pin,  (int)pop()); }
        if (ir == 'D') { digitalWrite(pin, (int)pop()); }
        break;
    default:
        isError = 1;
        printString("-notPin-");
    }
    return pc;
}

#ifdef __GAMEPAD__
#include <HID-Project.h>
#include <HID-Settings.h>
addr doGamePad(byte ir, addr pc) {
    ir = *(pc++);
    switch (ir) {
    case 'X': Gamepad.xAxis(pop());          break;
    case 'Y': Gamepad.yAxis(pop());          break;
    case 'P': Gamepad.press(pop());          break;
    case 'R': Gamepad.release(pop());        break;
    case 'A': Gamepad.dPad1(pop());          break;
    case 'B': Gamepad.dPad2(pop());          break;
    case 'L': Gamepad.releaseAll();          break;
    case 'W': Gamepad.write();               break;
    default:
        isError = 1;
        printString("-notGamepad-");
    }
    return pc;
  
}
#else
addr doGamePad(addr pc) { printString("-noGamepad-"); return pc; }
#endif

addr doCustom(byte ir, addr pc) {
    switch (ir) {
    case 'G': pc = doGamePad(ir, pc);       break;
    case 'N': push(micros());               break;
    case 'P': pc = doPin(pc);               break;
    case 'T': push(millis());               break;
    case 'W': delay(pop());                 break;
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
    X(1000, ":CODE CR xIAU xIH 1-[rI c@ #,';=(rI 1+ c@':=(CR))];") \
    X(1001, ":CR 13,10,;:U xIHxIAU-;") \
    X(1002, ":REGS 0 xIR 1-[rI xIC* xIAR+@ #s1(CR\"r\" rI 26&$ 26&$ 'A+,'A+,'A+,\": \"r1.)];") \
    X(2000, "CR\"This system has \"xIR.\" registers, \"xIF.\" functions, and \"xIU.\" bytes user memory.\"CR")

//#if __BOARD__ == ESP8266
#define X(num, val) const char str ## num[] = val;
//#else
//#define X(num, val) const PROGMEM char str ## num[] = val;
//#endif
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
#ifdef __LITTLEFS__
    loadCode("xFL");
#endif
}

void ok() {
    printString("\r\ns4:("); 
    dumpStack(); 
    printString(")>");
}

// NB: tweak this depending on what your terminal window sends for [Backspace]
// E.G. - PuTTY sends a 127 for Backspace
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
        if (!isOTA) {
          char b[] = {8, 32, 8, 0};
          printString(b);
        }
        return;
    }
    if (c == 9) { c = 32; }
    if (BetweenI(c, 32, 126)) {
        *(here1++) = c;
        if (!isOTA) { printChar(c); }
    }
}

void setup() {
#ifdef __SERIAL__
    mySerial.begin(19200);
    while (!mySerial) {}
    delay(500);
    // while (mySerial.available()) { char c = mySerial.read(); }
    ok();
#endif
    vmInit();
    wifiStart();
    fileInit();
}

void do_autoRun() {
    const char *cp = "AUTORUN";
    addr a = (addr)cp;
    addr fa = findFunc(funcNum(a));
    if (fa) { run(fa); }
}

void loop() {
    static int iLed = 0;
    static long nextBlink = 0;
    static int ledState = LOW;
    long curTm = millis();

    if (iLed == 0) {
        loadBaseSystem();
        ok();
        iLed = 13;
        // iLed = LED_BUILTIN;
        pinMode(iLed, OUTPUT);
    }
    if (nextBlink < curTm) {
        ledState = (ledState == LOW) ? HIGH : LOW;
        digitalWrite(iLed, ledState);
        nextBlink = curTm + 1000;
        // if (ledState == HIGH) { nextBlink += 1000; }
    }

    while (charAvailable()) { 
        isOTA = 0;
        handleInput(getChar()); 
    }
    while (wifiCharAvailable()) { 
        isOTA = 1;
        handleInput(wifiGetChar()); 
    }
    // do_autoRun();
}
