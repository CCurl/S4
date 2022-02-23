// editor.cpp - A simple block editor
//
// NOTE: A huge thanks to Alain Theroux. This editor was inspired by
//       his editor and is a shameful reverse-engineering of it. :D

#include "S4.h"

#ifndef __EDITOR__
void doEditor() { printString("-noEdit-"); }
#else
#define LLEN        100
#define MAX_X       (LLEN-1)
#define MAX_Y       20
#define BLOCK_SZ    (LLEN)*(MAX_Y+1)
#define MAX_CUR     (BLOCK_SZ-1)
int line, off, blkNum;
int cur, isDirty = 0;
char theBlock[BLOCK_SZ];
const char *msg = NULL;

int edGetChar() {
    int c = getChar();
    // in PuTTY, cursor keys are <esc>, '[', [A..D]
    // other keys are <esc>, '[', [1..6] , '~'
    if (c == 27) {
        c = getChar();
        if (c == '[') {
            c = getChar();
            if (c == 'A') { return 'w'; } // up
            if (c == 'B') { return 's'; } // down
            if (c == 'C') { return 'd'; } // right
            if (c == 'D') { return 'a'; } // left
            if (c == '1') { if (getChar() == '~') { return 'q'; } } // home
            if (c == '4') { if (getChar() == '~') { return 'e'; } } // end
            if (c == '5') { if (getChar() == '~') { return 't'; } } // top (pgup)
            if (c == '6') { if (getChar() == '~') { return 'l'; } } // last (pgdn)
            if (c == '3') { if (getChar() == '~') { return 'x'; } } // del
        }
        return c;
    } else {
        // in Windows, cursor keys are 224, [HPMK]
        // other keys are 224, [GOIQS]
        if (c == 224) {
            c = getChar();
            if (c == 'H') { return 'w'; } // up
            if (c == 'P') { return 's'; } // down
            if (c == 'M') { return 'd'; } // right
            if (c == 'K') { return 'a'; } // left
            if (c == 'G') { return 'q'; } // home
            if (c == 'O') { return 'e'; } // end
            if (c == 'I') { return 't'; } // top pgup (pgup)
            if (c == 'Q') { return 'l'; } // last (pgdn)
            if (c == 'S') { return 'x'; } // del
            return c;
        }
    }
    return c;
}

void clearBlock() {
    for (int i = 0; i < BLOCK_SZ; i++) {
        theBlock[i] = 10;
    }
}

void edRdBlk() {
    clearBlock();
    push(blkNum);
    push((CELL)theBlock);
    push(BLOCK_SZ);
    blockRead();
    msg = (pop()) ? "-loaded-" : "-noFile-";
    cur = isDirty = 0;
}

void edSvBlk() {
    push(blkNum);
    push((CELL)theBlock);
    push(BLOCK_SZ);
    blockWrite();
    msg = (pop()) ? "-saved-" : "-errWrite-";
    cur = isDirty = 0;
}

void GotoXY(int x, int y) { printStringF("\x1B[%d;%dH", y, x); }
void CLS() { printString("\x1B[2J"); GotoXY(1, 1); }
void CursorOn() { printString("\x1B[?25h"); }
void CursorOff() { printString("\x1B[?25l"); }

void showGuide() {
    printString("\r\n    +"); 
    for (int i = 0; i <= MAX_X; i++) { printChar('-'); } 
    printChar('+');
}

void showFooter() {
    printString("\r\n     (q)home (w)up (e)end (a)left (s)down (d)right (t)top (l)last");
    printString("\r\n     (x)del char (i)insert char (r)replace char (I)Insert (R)Replace");
    printString("\r\n     (n)LF (S)Save (L)reLoad (+)next (-)prev (Q)quit");
    printString("\r\n-> \x8");
}

void showEditor() {
    int cp = 0;
    CursorOff();
    GotoXY(1, 1);
    printString("     Block Editor v0.1 - ");
    printStringF("Block# %03d %c", blkNum, isDirty ? '*' : ' ');
    printStringF(" %-20s", msg ? msg : "");
    msg = NULL;
    showGuide();
    for (int i = 0; i <= MAX_Y; i++) {
        printStringF("\r\n %2d |", i);
        for (int j = 0; j <= MAX_X; j++) {
            if (cur == cp) {
                printChar((char)178);
            }
            unsigned char c = theBlock[cp++];
            if (c == 13) { c = 174; }
            if (c == 10) { c = 241; }
            if (c ==  9) { c = 242; }
            if (c <  32) { c = '.'; }
            printChar((char)c);
        }
        printString("| ");
    }
    showGuide();
    CursorOn();
}

void deleteChar() {
    for (int i = cur; i < MAX_CUR; i++) { theBlock[i] = theBlock[i + 1]; }
    theBlock[MAX_CUR - 1] = 32;
    theBlock[MAX_CUR] = 10;
}

void insertChar(char c) {
    for (int i = MAX_CUR; cur < i; i--) { theBlock[i] = theBlock[i - 1]; }
    theBlock[cur] = c;
    theBlock[MAX_CUR] = 10;
}

void doType(int isInsert) {
    CursorOff();
    while (1) {
        char c= getChar();
        if (c == 27) { --cur;  return; }
        int isBS = ((c == 127) || (c == 8));
        if (isBS) {
            if (cur) {
                theBlock[--cur] = ' ';
                if (isInsert) { deleteChar(); }
                showEditor();
            }
            continue;
        }
        if (isInsert) { insertChar(' '); }
        if (c == 13) { c = 10; }
        theBlock[cur++] = c;
        if (MAX_CUR < cur) { cur = MAX_CUR; }
        showEditor();
        CursorOff();
    }
}

int processEditorChar(char c) {
    printChar(c);
    switch (c) {
    case 'Q': return 0;                                  break;
    case 'a': --cur;                                     break;
    case 'd': ++cur;                                     break;
    case 'w': cur -= LLEN;                               break;
    case 's': cur += LLEN;                               break;
    case 'q': cur -= (cur % LLEN);                       break;
    case 'e': cur -= (cur % LLEN); cur += MAX_X;         break;
    case 't': cur = 0;                                   break;
    case 'l': cur = MAX_CUR - MAX_X;                     break;
    case 'r': isDirty = 1; theBlock[cur++] = getChar();  break;
    case 'I': isDirty = 1; doType(1);                    break;
    case 'R': isDirty = 1; doType(0);                    break;
    case 'n': isDirty = 1; theBlock[cur++] = 10;         break;
    case 'x': isDirty = 1; deleteChar();                 break;
    case 'i': isDirty = 1; insertChar(' ');              break;
    case 'L': edRdBlk();                                 break;
    case 'S': edSvBlk();                                 break;
    case '+': if (isDirty) { edSvBlk(); }
            ++blkNum;
            edRdBlk();
            break;
    case '-': if (isDirty) { edSvBlk(); }
            blkNum -= (blkNum) ? 1 : 0;
            edRdBlk();
            break;
    }
    return 1;
}

void doEditor() {
    blkNum = pop();
    blkNum = (0 <= blkNum) ? blkNum : 0;
    CLS();
    edRdBlk();
    showEditor();
    showFooter();
    while (processEditorChar(edGetChar())) {
        if (cur < 0) { cur = 0; }
        if (MAX_CUR < cur) { cur = MAX_CUR; }
        showEditor();
        showFooter();
    }
}
#endif
