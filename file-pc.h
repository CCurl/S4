#include "s4.h"

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

void blockLoad() {
    char fn[24];
    if (input_fp) { fpush((CELL)input_fp); }
    sprintf(fn, "block-%03d.s4", (int)pop());
    input_fp = (CELL)fopen(fn, "rb");
    if (!input_fp) { input_fp = fpop(); }
}
