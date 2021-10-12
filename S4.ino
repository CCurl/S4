#include "config.h"
#include "s4.h"

#if __SERIAL__
    int charAvailable() { return mySerial.available(); }
    int getChar() { 
        while (!charAvailable()) {}
        return mySerial.read();
    }
    void printString(const char* str) { mySerial.print(str); }
#else
    int charAvailable() { return 0; }
    int getChar() { return 0; }
    void printString(const char* str) { }
#endif

#define TIB       (USER_SZ - TIB_SZ - 1)

void loadCode(const char* src) {
    int i = TIB;
    while (*src) { setCodeByte(i++, *(src++)); }
    setCodeByte(i, 0);
    run(TIB);
}

// ********************************************
// * HERE is where you load your default code *
// ********************************************

void loadBaseSystem() {
    loadCode("0( MB: ManageButton )");
    loadCode("0( SC: SetContext   )");
    loadCode("0( RD: Read pin     )");
    loadCode("0( CQ: Changed?     )");
    loadCode("0( RV: RememberVal  )");
    loadCode("0( RA: todo         )");
}

void ok() {
    printString("\r\ns4:"); 
    dumpStack(0); 
    printString(">");
}

void handleInput(char c) {
    static int tibHERE = TIB;
    if (c == 13) {
        printString(" ");
        setCodeByte(tibHERE, 0);
        run(TIB);
        tibHERE = TIB;
        ok();
        return;
    }
    if ((c == 8) && (TIB < tibHERE)) {
        tibHERE--;
        char b[] = {8, 32, 8, 0};
        printString(b);
        return;
    }
    if (c == 9) { c = 32; }
    if ((32 <= c) && (tibHERE < CODE_SZ)) {
        setCodeByte(tibHERE++, c);
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
    static ulong nextBlink = 0;
    static int ledState = LOW;
    ulong curTm = millis();
    
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

    addr a = functionAddress(0);
    if (a) { run(a); }
}
