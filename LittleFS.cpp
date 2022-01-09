#include "s4.h"
#ifdef __LITTLEFS__
#include "LittleFS.h"
void lfsBegin() {
    LittleFS.begin();
    FSInfo fs_info;
    LittleFS.info(fs_info);
    printStringF("\r\nFS: Total: %ld", fs_info.totalBytes);
    printStringF("\r\nFS: Used: %ld", fs_info.usedBytes);
}

void lfsLoad() {
    int tot = 0;
    File f = LittleFS.open("/source", "r");
    printStringF("-File f: %ld-", (CELL)&f);
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

void lfsSave() {
    File f = LittleFS.open("/source", "w");
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

#else
void lfsBegin() {}
void lfsLoad() { printString("-noLoad-"); }
void lfsSave() { printString("-noSave-"); }
#endif
