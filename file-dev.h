#include "s4.h"

#include "LittleFS.h"

#define MAX_FILES 10
File files[MAX_FILES + 1];
int numFiles = 0;
File f;

#if __BOARD__ == TEENSY4
LittleFS_Program myFS;
void fileInit() {
    myFS.begin(1 * 1024 * 1024);
    printString("\r\nLittleFS: initialized");
    printStringF("\r\nBytes Used: %llu, Bytes Total:%llu", myFS.usedSize(), myFS.totalSize());
}
#else
#define FILE_READ "r"
#define FILE_WRITE "w"
#define myFS LittleFS
void fileInit() {
    myFS.begin();
    printString("\r\nLittleFS: initialized");
    FSInfo fs_info;
    myFS.info(fs_info);
    printStringF("\r\nLittleFS: Total: %ld", fs_info.totalBytes);
    printStringF("\r\nLittleFS: Used: %ld", fs_info.usedBytes);
}
#endif

int freeFile() {
    for (int i = 1; i <= MAX_FILES; i++) {
        if (!files[i]) { return i; }
    }
    isError = 1;
    printString("-fileFull-");
    return 0;
}

void fileOpen() {
    char* md = (char*)pop();
    char* fn = (char*)pop();
    int i = freeFile();
    if (i) {
        f = myFS.open(fn, (*md == 'w') ? FILE_WRITE : FILE_READ);
        if (f) { files[i] = f; }
        else {
            i = 0;
            isError = 1;
            printString("-openFail-");
        }
    }
    push(i);
}

void fileClose() {
    int fn = (int)pop();
    if (BetweenI(fn, 1, MAX_FILES) && (files[fn])) {
        files[fn].close();
    }
}

void fileDelete() {
    char* fn = (char*)TOS;
    TOS = myFS.remove(fn) ? 1 : 0;
}

void fileRead() {
    int fn = (int)pop();
    push(0);
    push(0);
    if (BetweenI(fn, 1, MAX_FILES) && (files[fn])) {
        byte c;
        TOS = files[fn].read(&c, 1);
        N = (CELL)c;
    }
}

void fileWrite() {
    int fn = (int)pop();
    byte c = (byte)TOS;
    TOS = 0;
    if (BetweenI(fn, 1, MAX_FILES) && (files[fn])) {
        TOS = files[fn].write(&c, 1);
    }
}

// (n--)
char* blockFN() {
    static char fn[24];
    CELL n = pop();
    sprintf(fn, "/block-%03d.s4", (int)n);
    return fn;
}

// (num mode--f)
void blockOpen() {
    CELL m = pop();
    int fn = freeFile();
    if (fn == 0) { return; }
    File f = myFS.open(blockFN(), m ? FILE_WRITE : FILE_READ);
    if (f) {
        files[fn] = f;
    }
}

// (num addr sz--f)
void blockRead() {
    CELL sz = pop(), a = pop();
    File f = myFS.open(blockFN(), FILE_READ);
    push(0);
    if (f) {
        TOS = f.read((addr)a, sz);
        f.close();
    }
}

// (num--)
File loadFile;
void blockLoad() {
    addr h = HERE;
    if (loadFile) { f.close(); }
    loadFile = myFS.open(blockFN(), FILE_READ);
    while (loadFile) {
        char c = 0;
        int num = loadFile.read(&c, 1);
        if (num == 0) { loadFile.close(); c = 0; }
        *(h++) = c;
        if ((c < 32) || (126 < c)) {
            *(h-1) = 0;
            run(HERE); 
            h = HERE;
        }
    }
}

// (num addr sz--f)
void blockWrite() {
    CELL sz = pop(), a = pop();
    char *fn = blockFN();
    myFS.remove(fn);
    File f = myFS.open(fn, FILE_WRITE);
    push(0);
    if (f) {
        if (sz == 0) { sz = strlen((char*)a); }
        f.write((addr)a, sz);
        f.close();
        TOS = 1;
    }
}
