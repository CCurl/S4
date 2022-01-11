#include "s4.h"
#ifndef __FILES__
void fileInit() {}
void fileOpen(const char *fn, const char *m) {}
void fileClose(CELL fh) { printString("-noLoad-"); }
void fileRead(CELL fh) { printString("-noSave-"); }
void fileWrite(CELL fh, byte ch) { printString("-noSave-"); }
void fileLoad(CELL fh) { printString("-noSave-"); }
void fileSave(CELL fh, byte ch) { printString("-noSave-"); }
#else
#if __BOARD__ == PC
void fileInit() {}

void fileOpen() { 
    char* md = (char *)pop();
    char* fn = (char *)TOS;
    TOS = (CELL)fopen(fn, md);
}

void fileClose() {
    FILE *fh = (FILE*)pop();
    fclose(fh);
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
#else
#include "LittleFS.h"

void fileInit() {
    LittleFS.begin();
    FSInfo fs_info;
    LittleFS.info(fs_info);
    printStringF("\r\nFS: Total: %ld", fs_info.totalBytes);
    printStringF("\r\nFS: Used: %ld", fs_info.usedBytes);
}

void fileOpen() {
    char* md = (char*)pop();
    char* fn = (char*)pop();
    push((CELL)fopen(fn, md));
}

void fileClose() {
    FILE* fh = (FILE*)pop();
    fclose(fh);
}

void fileRead() {
    FILE* fh = (FILE*)pop();
    push(0);
    push(0);
}

void fileWrite() {
    FILE* fh = (FILE*)pop();
    char c = (char)pop();
    push(0);
}

void fileLoad() {
    int tot = 0;
    File f = LittleFS.open("/Code.S4", "r");
    printStringF("-File f: %ld-", (CELL)f);
    if (f) {
        vmInit();
        while (1) {
            int num = f.read(HERE, 256);
            tot += num;
            HERE += num;
            if (num < 256) { break; }
        }
        f.close();
        *HERE = ';';
        *(HERE+1) = 0;
        run(USER);
        printStringF("-loaded, (%d)-", tot);
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

#endif
#endif
