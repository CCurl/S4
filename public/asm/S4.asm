format PE console
include 'win32ax.inc'

.data

hStdIn      dd  0
hStdOut     dd  0
hello       db  "hello.", 0
okStr       db  13, 10, "s4:()>", 0
dotStr      db  "%ld", 0
crlf        db  13, 10
tib         db  128 dup ?
outBuf      db  256 dup ?
bytesRead   dd  0
unkOP       db  "-unk-"
isNum       db  "-num-"
isReg       db  "-reg-"
skipErr     db  "-skip-"
createErr   db  "-create-"
rDepth      dd   ?
rStackPtr   dd   ?
fStartAddr  dd   ?
fEndAddr    dd   ?
fI          dd   ?
fEndI       dd   ?


; ******************************************************************************
; CODE 
; ******************************************************************************
section '.code' code readable executable

; ------------------------------------------------------------------------------
; macros ...
; ------------------------------------------------------------------------------

CELL_SIZE equ 4
REG1 equ eax         ; Free register #1
REG2 equ ebx         ; Free register #2
REG3 equ ecx         ; Free register #3
REG4 equ edx         ; Free register #4
TOS  equ edi         ; Top-Of-Stack
PCIP equ esi         ; Program-Counter/Instruction-Pointer
STKP equ ebp         ; Stack-Pointer


macro m_setTOS val
{
       mov TOS, val
}

macro m_push val
{
       ; inc [dDepth]
       add STKP, CELL_SIZE
       mov [STKP], TOS
       m_setTOS val
}

; ------------------------------------------------------------------------------
macro m_get2ND val
{
       mov val, [STKP]
}
macro m_set2ND val
{
       mov [STKP], val
}

; ------------------------------------------------------------------------------
macro m_getTOS val
{
       mov val, TOS
}

macro m_drop
{
       ; dec [dDepth]
       mov TOS, [STKP]
       sub STKP, CELL_SIZE
}

macro m_pop val
{
       m_getTOS val
       m_drop
}

; ------------------------------------------------------------------------------
macro m_rpush reg
{
       push TOS
       add [rStackPtr], CELL_SIZE
       mov TOS, [rStackPtr]
       mov [TOS], reg
       pop TOS
}
 
macro m_rpop reg
{
       push TOS
       mov TOS, [rStackPtr]
       mov reg, [TOS]
       sub [rStackPtr], CELL_SIZE
       pop TOS
}

; ******************************************************************************
doNop:  jmp     s4Next

; ******************************************************************************
iToA:   mov     ecx, outBuf+16
        mov     ebx, 0
        mov     BYTE [ecx], 0
i2a1:   push    ebx
        mov     ebx, 10
        mov     edx, 0
        div     ebx
        add     dl, '0'
        dec     ecx 
        mov     BYTE [ecx], dl
        pop     ebx
        inc     ebx
        cmp     eax, 0
        jne     i2a1
        ret

; ******************************************************************************
; register
doRegister:
        sub     al, 'a'
        and     eax, $ff
        mov     edx, eax
reg1:   mov     al, [esi]
        mov     bx, 'az'
        call    betw
        cmp     bl, 0
        je      regN
        sub     al, 'a'
        imul    edx, edx, 26
        add     edx, eax
        inc     esi
regN:   shl     edx, 2
        add     edx, regs
        m_push  edx
regX:   jmp     s4Next

; ******************************************************************************
; function
doFunction:
        sub     al, 'A'
        mov     edx, eax
fun1:   mov     al, [esi]
        mov     bx, 'AZ'
        call    betw
        cmp     bl, 0
        je      funN
        inc     esi
        sub     al, 'A'
        imul    edx, edx, 26
        add     edx, eax
funN:   shl     edx, 2
        add     edx, funcs
        mov     ebx, [edx]
        cmp     ebx, 0
        je      funX
        m_rpush esi
        mov     esi, ebx
funX:   jmp     s4Next


; ******************************************************************************
; skip to BL
skipTo:
        mov     al, [esi]
        cmp     al, 32
        jl      skpE
        inc     esi
        cmp     al, bl
        je      skpX
sk0:    cmp     al, '('
        jne     sk1
        mov     al, ')'
        jmp     skpRec
sk1:    cmp     al, '['
        jne     sk2
        mov     al, ']'
        jmp     skpRec
sk2:    cmp     al, '"'
        je      skpRec
sk3:    cmp     al, 39
        jne     sk4
        inc     esi
        jmp     skipTo
sk4:    cmp     al, '{'
        jne     sk5
        mov     al, '}'
        jmp     skpRec
sk5:    jmp     skipTo

skpRec: push    ebx
        mov     bl, al
        call    skipTo
        pop     ebx

skpA:   jmp     skipTo
        
skpE:   invoke  WriteConsole, [hStdOut], skipErr, 6, NULL, NULL
skpX:   ret

; ******************************************************************************
; create function: :XX ...;
doCreate:
        lodsb
        and     eax, $ff
        mov     bx, 'AZ'
        call    betw
        cmp     bl, 0
        je      crE
        sub     al, 'A'
        inc     esi
        mov     edx, eax
        mov     al, [esi]
        mov     bx, 'AZ'
        call    betw
        cmp     bl, 0
        je      crN
        sub     al, 'A'
        inc     esi
        imul    edx, edx, 26
        add     edx, eax
crN:    shl     edx, 2
        add     edx, funcs
        mov     [edx], esi
        mov     bl, ';'
        call    skipTo
        jmp     s4Next
        
crE:    invoke  WriteConsole, [hStdOut], createErr, 8, NULL, NULL
        jmp     s4Next

; ******************************************************************************
doReturn:
        m_rpop  esi
        jmp     s4Next

; ******************************************************************************
; Number input
num:    sub     al, '0'
        and     eax, $FF
        mov     edx, eax
n1:     mov     al, [esi]
        mov     bx, '09'
        call    betw
        cmp     bl, 0
        je      nx
        sub     al, '0'
        imul    edx, edx, 10
        add     edx, eax
        inc     esi
        jmp     n1
nx:     m_push  edx
        jmp     s4Next

; ******************************************************************************
; Quote
qt:     lodsb
        cmp     al, '"'
        je      qx
        call    p1
        jmp     qt
qx:     inc     esi
        jmp     s4Next

; ******************************************************************************
betw:   cmp     al, bl
        jl      betF
        cmp     al, bh
        jg      betF
        mov     bl, 1
        ret
betF:   mov     bl, 0
        ret

; ******************************************************************************
doFor:  m_pop   REG2
        m_pop   REG1
        .if     REG2 < REG1
            push    REG1
            mov     REG1, REG2
            pop     REG2
        .endif
        mov     [fEndAddr], 0
        mov     [fStartAddr], esi
        mov     [fEndI], REG2
        mov     [fI], REG1
        jmp     s4Next

; ******************************************************************************
doNext: mov     REG1, [fI]
        mov     REG2, [fEndI]
        .if REG1 < REG2
            inc     DWORD [fI]
            mov     esi, [fStartAddr]
            mov     [fEndAddr], esi
        .endif
        jmp     s4Next

doI:    m_push  [fI]
        jmp     s4Next

; ******************************************************************************
s4Next: lodsb
        cmp     al, $7E
        jg      s0
        ; call    p1
        movzx   ecx, al
        shl     ecx, 2
        mov     ebx, [jmpTable+ecx]
        jmp     ebx

; ******************************************************************************
f_UnknownOpcode:
        ; invoke WriteConsole, [hStdOut], unkOP, 5, NULL, NULL
        jmp s0

; ******************************************************************************
bye:    invoke  ExitProcess, 0

; ******************************************************************************
ok:
        invoke WriteConsole, [hStdOut], okStr, 8, NULL, NULL
        ret

; ******************************************************************************
doMult: m_pop   eax
        imul    TOS, eax
        jmp     s4Next

; ******************************************************************************
doSub:  m_pop   eax
        sub     TOS, eax
        jmp     s4Next

; ******************************************************************************
doAdd:  m_pop   eax
        add     TOS, eax
        jmp     s4Next

; ******************************************************************************
emit:   m_pop   eax
        call    p1
        jmp     s4Next

; ******************************************************************************
doBlank: mov    al, 32
        call    p1
        jmp     s4Next

; ******************************************************************************
doDot:  m_pop   eax
        call    iToA
        invoke  WriteConsole, [hStdOut], ecx, ebx, NULL, NULL
        jmp     s4Next

; ******************************************************************************
doTimer: invoke GetTickCount
        m_push  eax
        jmp     s4Next

; ******************************************************************************
doDup:  m_push  TOS
        jmp     s4Next

; ******************************************************************************
doSwap: m_get2ND    eax
        m_set2ND    TOS
        m_setTOS    eax
        jmp     s4Next

; ******************************************************************************
doOver: m_get2ND    eax
        m_push      eax
        jmp         s4Next

; ******************************************************************************
doDrop: m_drop
        jmp         s4Next

; ******************************************************************************
p1:     push    eax
        mov     [outBuf], al
        invoke  WriteConsole, [hStdOut], outBuf, 1, NULL, NULL
        pop     eax
        ret

; ******************************************************************************
s_SYS_INIT:
        ; Return stack
        mov     eax, rStack - CELL_SIZE
        mov     [rStackPtr], eax
        mov     [rDepth], 0
        ; Data stack
        mov     STKP, dStack
        ; PCIP = IP/PC
        mov     PCIP, THE_MEMORY
        ret        

; ******************************************************************************
start:
        invoke  GetStdHandle, STD_INPUT_HANDLE
        mov     [hStdIn], eax
        
        invoke  GetStdHandle, STD_OUTPUT_HANDLE
        mov     [hStdOut], eax
    
        invoke  WriteConsole, [hStdOut], hello, 6, NULL, NULL

s0:     call    ok
        mov     eax, rStack - CELL_SIZE
        mov     [rStackPtr], eax
        mov     [rDepth], 0
        cmp     STKP, dStack
        jl      s0R
        mov     STKP, dStack
s0R:    invoke  ReadConsole, [hStdIn], tib, 128, bytesRead, 0
        cld
        mov     esi, tib
        jmp     s4Next
    
        invoke  ExitProcess, 0


; ******************************************************************************
section '.mem' data readable writable

jmpTable:
dd f_UnknownOpcode            ; # 00 ()
dd f_UnknownOpcode            ; # 01 (☺)
dd f_UnknownOpcode            ; # 02 (☻)
dd f_UnknownOpcode            ; # 03 (♥)
dd f_UnknownOpcode            ; # 04 (♦)
dd f_UnknownOpcode            ; # 05 (♣)
dd f_UnknownOpcode            ; # 06 (♠)
dd f_UnknownOpcode            ; # 07 ()
dd f_UnknownOpcode            ; # 08 ()
dd doNop                      ; # 09 ()
dd f_UnknownOpcode            ; # 010 ()
dd f_UnknownOpcode            ; # 011 ()
dd f_UnknownOpcode            ; # 012 ()
dd f_UnknownOpcode            ; # 013 ()
dd f_UnknownOpcode            ; # 014 ()
dd f_UnknownOpcode            ; # 015 ()
dd f_UnknownOpcode            ; # 016 (►)
dd f_UnknownOpcode            ; # 017 (◄)
dd f_UnknownOpcode            ; # 018 (↕)
dd f_UnknownOpcode            ; # 019 (‼)
dd f_UnknownOpcode            ; # 020 (¶)
dd f_UnknownOpcode            ; # 021 (§)
dd f_UnknownOpcode            ; # 022 (▬)
dd f_UnknownOpcode            ; # 023 (↨)
dd f_UnknownOpcode            ; # 024 (↑)
dd f_UnknownOpcode            ; # 025 (↓)
dd f_UnknownOpcode            ; # 026 (→)
dd f_UnknownOpcode            ; # 027 (
dd f_UnknownOpcode            ; # 028 (∟)
dd f_UnknownOpcode            ; # 029 (↔)
dd f_UnknownOpcode            ; # 030 (▲)
dd f_UnknownOpcode            ; # 031 (▼)
dd doNop                      ; # 032 ( )
dd f_UnknownOpcode            ; # 033 (!)
dd qt                         ; # 034 (")
dd doDup                      ; # 035 (#)
dd doSwap                     ; # 036 ($)
dd doOver                     ; # 037 (%)
dd f_UnknownOpcode            ; # 038 (&)
dd f_UnknownOpcode            ; # 039 (')
dd f_UnknownOpcode            ; # 040 (()
dd f_UnknownOpcode            ; # 041 ())
dd doMult                     ; # 042 (*)
dd doAdd                      ; # 043 (+)
dd emit                       ; # 044 (,)
dd doSub                      ; # 045 (-)
dd doDot                      ; # 046 (.)
dd f_UnknownOpcode            ; # 047 (/)
dd num                        ; # 048 (0)
dd num                        ; # 049 (1)
dd num                        ; # 050 (2)
dd num                        ; # 051 (3)
dd num                        ; # 052 (4)
dd num                        ; # 053 (5)
dd num                        ; # 054 (6)
dd num                        ; # 055 (7)
dd num                        ; # 056 (8)
dd num                        ; # 057 (9)
dd doCreate                   ; # 058 (:)
dd doReturn                   ; # 059 (;)
dd f_UnknownOpcode            ; # 060 (<)
dd f_UnknownOpcode            ; # 061 (=)
dd f_UnknownOpcode            ; # 062 (>)
dd f_UnknownOpcode            ; # 063 (?)
dd f_UnknownOpcode            ; # 064 (@)
dd doFunction                 ; # 065 (A)
dd doFunction                 ; # 066 (B)
dd doFunction                 ; # 067 (C)
dd doFunction                 ; # 068 (D)
dd doFunction                 ; # 069 (E)
dd doFunction                 ; # 070 (F)
dd doFunction                 ; # 071 (G)
dd doFunction                 ; # 072 (H)
dd doFunction                 ; # 073 (I)
dd doFunction                 ; # 074 (J)
dd doFunction                 ; # 075 (K)
dd doFunction                 ; # 076 (L)
dd doFunction                 ; # 077 (M)
dd doFunction                 ; # 078 (N)
dd doFunction                 ; # 079 (O)
dd doFunction                 ; # 080 (P)
dd doFunction                 ; # 081 (Q)
dd doFunction                 ; # 082 (R)
dd doFunction                 ; # 083 (S)
dd doFunction                 ; # 084 (T)
dd doFunction                 ; # 085 (U)
dd doFunction                 ; # 086 (V)
dd doFunction                 ; # 087 (W)
dd doFunction                 ; # 088 (X)
dd doFunction                 ; # 089 (Y)
dd doFunction                 ; # 090 (Z)
dd doFor                      ; # 091 ([)
dd doDrop                     ; # 092 (\)
dd doNext                     ; # 093 (])
dd f_UnknownOpcode            ; # 094 (^)
dd f_UnknownOpcode            ; # 095 (_)
dd f_UnknownOpcode            ; # 096 (x)
dd doRegister                 ; # 097 (a)
dd doRegister                 ; # 098 (b)
dd doRegister                 ; # 099 (c)
dd doRegister                 ; # 100 (d)
dd doRegister                 ; # 101 (e)
dd doRegister                 ; # 102 (f)
dd doRegister                 ; # 103 (g)
dd doRegister                 ; # 104 (h)
dd doRegister                 ; # 105 (i)
dd doRegister                 ; # 106 (j)
dd doRegister                 ; # 107 (k)
dd doRegister                 ; # 108 (l)
dd doRegister                 ; # 109 (m)
dd doRegister                 ; # 110 (n)
dd doRegister                 ; # 111 (o)
dd doRegister                 ; # 112 (p)
dd doRegister                 ; # 113 (q)
dd doRegister                 ; # 114 (r)
dd doRegister                 ; # 115 (s)
dd doRegister                 ; # 116 (t)
dd doRegister                 ; # 117 (u)
dd doRegister                 ; # 118 (v)
dd doRegister                 ; # 119 (w)
dd doRegister                 ; # 120 (x)
dd doRegister                 ; # 121 (y)
dd doRegister                 ; # 122 (z)
dd f_UnknownOpcode            ; # 123 ({)
dd f_UnknownOpcode            ; # 124 (|)
dd f_UnknownOpcode            ; # 125 (})
dd f_UnknownOpcode            ; # 126 (~)

dstackB     dd    ?           ; Buffer between stack and regs
dStack      dd   32 dup 0
dstackE     dd    ?           ; Buffer between stacks
rStack      dd   32 dup 0
tmpBuf3     dd    ?           ; Buffer for return stack

funcs       dd  676 dup 0
regs        dd  676 dup 0

THE_MEMORY  rb 1024*1024
MEM_END:

.end start
