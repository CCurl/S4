#include "s4.h"

#ifndef __FILES__
void fileInit() {}
void fileOpen(const char *fn, const char *m) {}
void fileClose() { printString("-noClose-"); }
void fileDelete() { printString("-noDelete-"); }
void fileRead() { printString("-noRead-"); }
void fileWrite() { printString("-noWrite-"); }
void fileLoad() { printString("-noLoad-"); }
void fileSave() { printString("-noSave-"); }
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
#endif // PC

#ifdef __LITTLEFS__
#include "LittleFS.h"

#define MAX_FILES 10
File *files[MAX_FILES];
int numFiles = 0;
File f;

int freeFile() {
  for (int i = 1; i <= MAX_FILES; i++) {
    if (files[i-1] == NULL) { return i; }
  }
  isError = 1;
  printString("-fileFull-");
  return 0;
}

void fileInit() {
    for (int i = 0; i < MAX_FILES; i++) { files[i] = NULL; }
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
        if (f) { files[i-1] = &f; } 
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
    if ((0 < fn) && (fn <= MAX_FILES) && (files[fn-1] != NULL)) {
        files[fn-1]->close();
        files[fn-1] = NULL;
    }
}

void fileDelete() {
    char* fn = (char*)TOS;
    printString("-notimpl-");
    TOS = 0;
}

void fileRead() {
    int fn = (int)pop();
    push(0);
    push(0);
    if ((0 < fn) && (fn <= MAX_FILES) && (files[fn-1] != NULL)) {
        byte c;
        TOS = files[fn-1]->read(&c, 1);
        N = (CELL)c;
    }
}

void fileWrite() {
    int fn = (int)pop();
    byte c = (byte)TOS;
    TOS = 0;
    if ((0 < fn) && (fn <= MAX_FILES) && (files[fn - 1] != NULL)) {
        TOS = files[fn-1]->write(&c, 1);
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

#endif // __LITTLEFS__
#endif // __FILES__
