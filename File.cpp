#include "s4.h"

#ifndef __FILES__
void noFile() { printString("-noFile-"); }
void fileInit() { }
void fileOpen() { noFile(); }
void fileClose() { noFile(); }
void fileDelete() { noFile(); }
void fileRead() { noFile(); }
void fileWrite() { noFile(); }
void fileLoad() { noFile(); }
void fileSave() { noFile(); }
void blockOpen() { noFile(); }
void blockRead() { noFile(); }
void blockWrite() { noFile(); }
void blockLoad() { noFile(); }
#else
#if __BOARD__ == PC
void fileInit() {}

void fileOpen() { 
    char* md = (char *)pop();
    char* fn = (char *)TOS;
    TOS = (CELL)fopen(fn, md);
}

void fileClose() {
    FILE* fh = (FILE*)pop();
    fclose(fh);
}

void fileDelete() {
    char* fn = (char*)TOS;
    TOS = remove(fn) == 0 ? 1 : 0;
}

void fileRead() {
    FILE* fh = (FILE*)TOS;
    TOS = 0;
    push(0);
    if (fh) {
        char c;
        TOS = fread(&c, 1, 1, fh);
        N = TOS ? c : 0;
    }
}

void fileWrite() {
    FILE* fh = (FILE*)pop();
    char c = (char)TOS;
    TOS = 0;
    if (fh) {
        TOS = fwrite(&c, 1, 1, fh);
    }
}

void fileLoad() {
    FILE *fh = fopen("Code.S4", "rt");
    if (fh) {
        vmInit();
        int num = fread(HERE, 1, USER_SZ, fh);
        HERE += num;
        fclose(fh);
        *(HERE) = 0;
        run(USER);
        printStringF("-loaded, (%d)-", num);
    }
    else {
        printString("-loadFail-");
    }
}

void fileSave() {
    FILE* fh = fopen("Code.S4", "wt");
    if (fh) {
        int count = HERE - USER;
        fwrite(USER, 1, count, fh);
        fclose(fh);
        printStringF("-saved (%d)-", count);
    }
    else {
        printString("-saveFail-");
    }
}

// (n m--fh)
void blockOpen() {
    CELL m = pop(), n = TOS;
    char fn[24];
    sprintf(fn, "block-%03d.s4", (int)n);
    TOS = (CELL)fopen(fn, m ? "wb" : "rb");
}

// (n a sz--flag)
void blockRead() {
    CELL sz = pop(), a = pop();
    push(0); blockOpen();
    FILE* fh = (FILE *)pop();
    push(0);
    if (fh) {
        TOS = fread((byte *)a, 1, sz, fh);
        fclose(fh);
    }
}

// (n a sz--flag)
void blockWrite() {
    CELL sz = pop(), a = pop();
    push(1); blockOpen();
    FILE* fh = (FILE *)pop();
    push(0);
    if (fh) {
        if (sz == 0) { sz = strlen((char*)a); }
        fwrite((byte *)a, 1, sz, fh);
        fclose(fh);
        TOS = 1;
    }
}

extern FILE* input_fp;
extern FILE* fpop();
extern void fpush(FILE*);
void blockLoad() {
    char fn[24];
    if (input_fp) { fpush(input_fp); }
    sprintf(fn, "block-%03d.s4", (int)pop());
    input_fp = fopen(fn, "rb");
    if (!input_fp) { input_fp = fpop(); }
}

#endif // PC

#ifdef __LITTLEFS__
#include "LittleFS.h"

#define MAX_FILES 10
File files[MAX_FILES+1];
int numFiles = 0;
File f;

int freeFile() {
  for (int i = 1; i <= MAX_FILES; i++) {
    if (!files[i]) { return i; }
  }
  isError = 1;
  printString("-fileFull-");
  return 0;
}

void fileInit() {
    // for (int i = 0; i <= MAX_FILES; i++) { files[i] = File(); }
    // LittleFS.begin(true);
    // printString("\r\nLittleFS: initialized");
    LittleFS.begin();
    FSInfo fs_info;
    LittleFS.info(fs_info);
    printStringF("\r\nLittleFS: Total: %ld", fs_info.totalBytes);
    printStringF("\r\nLittleFS: Used: %ld", fs_info.usedBytes);
}

void fileOpen() {
    char* md = (char*)pop();
    char* fn = (char*)pop();
    int i = freeFile();
    if (i) {
        f = LittleFS.open(fn, md);
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
    TOS = LittleFS.remove(fn) ? 1 : 0;
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

void fileLoad() {
    File f = LittleFS.open("/Code.S4", "r");
    if (f) {
        vmInit();
        int num = f.read(HERE, USER_SZ);
        f.close();
        HERE += num;
        *(HERE+1) = 0;
        run(USER);
        printStringF("-loaded, (%d)-", num);
    }
    else {
        printString("-loadFail-");
    }
}

void fileSave() {
    File f = LittleFS.open("/Code.S4", "w");
    if (f) {
        int count = HERE - USER;
        f.write(USER, count);
        f.close();
        printString("-saved-");
    }
    else {
        printString("-saveFail-");
    }
}

// (n--)
char *blockFN() {
    static char fn[24];
    CELL n = pop();
    sprintf(fn, "/block-%03d.s4", (int)n);
    return fn;
}

// (n m--f)
void blockOpen() {
    CELL m = pop();
    int fn = freeFile();
    if (fn == 0) { return; }
    File f = LittleFS.open(blockFN(), m ? "w" : "r");
    if (f) {
        files[fn] = f;
    }
}

// (n a sz--f)
void blockRead() {
    CELL sz = pop(), a = pop();
    File f = LittleFS.open(blockFN(), "r");
    push(0);
    if (f) {
        TOS = f.read((addr)a, sz);
        f.close();
    }
}

// (n--)
void blockLoad() {
    printString("-noBL-");
    pop();
}

// (n a sz--f)
void blockWrite() {
    CELL sz = pop(), a = pop();
    File f = LittleFS.open(blockFN(), "w");
    push(0);
    if (f) {
        if (sz == 0) { sz = strlen((char*)a); }
        f.write((addr)a, sz);
        f.close();
        TOS = 1;
    }
}
#endif // __LITTLEFS__
#endif // __FILES__
