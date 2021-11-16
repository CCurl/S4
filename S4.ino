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

    loadCode(":C t@1+t! a@#*s@/c! b@#*s@/d! c@d@+k@>(j@m!;) a@b@*100/y@+b! c@d@-x@+a! j@1+j!;");
    loadCode(":L 0a!0b!0j!s@m!1{\\Cj@m@<};");
    loadCode(":O Lj@40+#126>(\\32),;");
    loadCode(":X 490`-x!1 95[  O x@ 8+x!];");
    loadCode(":Y 340`-y!1 35[N X y@20+y!];");
    loadCode(":M 0t! `T Y `T$- N t@.\" iterations, \" . \" ms\";");
    loadCode("200 s! 1000000 k!");
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
