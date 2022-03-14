#include "s4.h"

#ifdef __FILES__

byte fdsp = 0;
CELL input_fp, fstack[FSTK_SZ + 1];
void fpush(CELL v) { if (fdsp < FSTK_SZ) { fstack[++fdsp] = v; } }
CELL fpop() { return (fdsp) ? fstack[fdsp--] : 0; }

#if __BOARD__ == PC
#include "file-pc.h"
#endif // PC

#ifdef __LITTLEFS__
#include "file-dev.h"
#endif // __LITTLEFS__

#else // __FILES__ NOT defined

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
CELL input_fp = 0;
void fpush(CELL x) { noFile(); };
CELL fpop() { noFile(); return 0; };

#endif // __FILES__
