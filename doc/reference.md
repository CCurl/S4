## S4 Reference

### ARITHMETIC
| Opcode| Stack       | Description |
|:--  |:--          |:--|
| +   | (a b--n)    | n: a+b - addition
| -   | (a b--n)    | n: a-b - subtraction
| *   | (a b--n)    | n: a*b - multiplication
| /   | (a b--q)    | q: a/b - division
| ^   | (a b--r)    | r: MODULO(a, b)
| &   | (a b--q r)  | q: DIV(a,b), r: MODULO(a,b) - /MOD


### BIT MANIPULATION
| Opcode| Stack   | Description |
|:--    |:--      |:--|
| b&    | (a b--n)   | n: a AND b
| b\|   | (a b--n)   | n: a OR  b
| b^    | (a b--n)   | n: a XOR b
| b~    | (a--b)     | b: NOT a (ones-complement, e.g - 101011 => 010100)


### STACK
| Opcode| Stack         | Description |
|:--    |:--            |:--|
| #     | (a--a a)      | Duplicate TOS                    (DUP)
| \     | (a b--a)      | Drop TOS                         (DROP)
| $     | (a b--b a)    | Swap top 2 stack items           (SWAP)
| %     | (a b--a b a)  | Push 2nd                         (OVER)
| _     | (a--b)        | b: -a                            (Negate)
| xA    | (a--b)        | b: abs(a)                        (Absolute)


### MEMORY
| Opcode| Stack       | Description |
|:--    |:--          |:--|
| c@    | (a--n)      | Fetch BYTE n from address a
| @     | (a--n)      | Fetch CELL n from address a
| c!    | (n a--)     | Store BYTE n to address a
| !     | (n a--)     | Store CELL n to address a


### REGISTERS and LOCALS
NOTES:
- Register names are 1 to 3 UPPERCASE characters: [rA..rZZZ]
- LOCALS: S4 provides 10 locals per call [r0..r9].
- The number of registers is controlled by the NUM_REGS #define in "config.h".
- Register 'rI' is the FOR Loop index **special**

| Opcode| Stack   | Description |
|:--    |:--      |:--|
| rXXX  | (--n)   | read register/local XXX
| sXXX  | (n--)   | set register/local XXX to n
| iXXX  | (--)    | increment register/local XXX
| dXXX  | (--)    | decrement register/local XXX
| nXXX  | (--)    | increment register/local XXX by the size of a cell (next cell)


### WORDS/FUNCTIONS
NOTES:
- Word/Function names are ProperCase identifiers. 
- They must begin with [A..Z], and can include lowercase characters [a..z].
- The number of words is controlled by the NUM_FUNCS #define in "config.h"
- NUM_FUNCS needs to be a power of 2.
- If a word has already been defined, S4 prints "-redef-".

| Opcode| Stack   | Description |
|:--    |:--      |:--|
| :     | (--)    | Define word/function. Copy chars to (HERE++) until closing ';'.
| ABC   | (--)    | Execute/call word/function ABC
| ;     | (--)    | Return: PC = rpop()
|       |         | - Returning while inside of a loop is not supported; break out of the loop first.
|       |         | - Use '\|' to break out of a FOR or WHILE loop.


### INPUT/OUTPUT
| Opcode| Stack       | Description |
|:--    |:--          |:--|
| .     | (N--)       | Output N as decimal number.
| ,     | (N--)       | Output N as character (Forth EMIT).
| "     | (?--?)      | Output characters until the next '"'.
|       |             | - %d outputs TOS as an integer (eg - 123"x%dx" outputs x123x)
|       |             | - %c outputs TOS as a character (eg - 65"x%cx" outputs xAx)
|       |             | - %n outputs CR/LF
|       |             | - %<x> output <x> (eg - "x%" %% %"x" outputs x" % "x)
| 0..9  | (--n)       | Scan DECIMAL number. For multiple numbers, separate them by space (47 33).
|       |             | - To enter a negative number, use "negate" (eg - 490_).
| hNNN  | (--h)       | h: NNN as a HEX number (0-9, A-F, UPPERCASE only).
| 'x    | (--n)       | n: the ASCII value of x
| `XXX` | (a--a b)    | Copies XXX to address a, b is the next address after the NULL terminator.
| xZ    | (a--)       | Output the NULL terminated string starting at address a.
| xK?   | (--f)       | f: 1 if a character is waiting in the input buffer, else 0.
| xK@   | (--c)       | c: next character from the input buffer. If no character, wait.


### CONDITIONS/LOOPS/FLOW CONTROL
| Opcode| Stack       | Description |
|:--    |:--          |:--|
| <     | (a b--f)    | f: (a < b) ? 1 : 0;
| =     | (a b--f)    | f: (a = b) ? 1 : 0;
| >     | (a b--f)    | f: (a > b) ? 1 : 0;
| ~     | (n -- f)    | f: (a = 0) ? 1 : 0; (Logical NOT)
| (     | (f--)       | IF: if (f != 0), continue into '()', else skip to matching ')'
| [     | (F T--)     | FOR: start a For/Next loop. if (T < F), swap T and F
| rI    | (--n)       | n: the index of the current FOR loop
| ]     | (--)        | NEXT: increment index (rI) and restart loop if (rI <= T)
|       |             | NOTE: A FOR loop always runs at least one iteration.
|       |             | - It can be put it inside a '()' to keep it from running.
| {     | (f--f)      | WHILE: if (f == 0) skip to matching '}'
| }     | (f--f?)     | WHILE: if (f != 0) jump to matching '{', else drop f and continue
| uL    | (--)        | Unwind the loop stack. Use with ';'.eg = (uL;)
| uF    | (--)        | Exit FOR. Continue after the next ']'.
| uW    | (--)        | Exit WHILE. Continue after the next '}'.
| uC    | (--)        | Continue. Jump to the beginning of the current loop.


### OTHER
| Opcode| Stack       | Description |
|:--    |:--          |:--|
| xBO   | (n m--fh)   | File: Block Open (block-nnn.s4, m: 0=>read, 1=write)
| xBR   | (n a sz--)  | File: Block Read (block-nnn.s4, max sz bytee).
| xBW   | (n a sz--)  | File: Block Write (block-nnn.s4, sz bytes).
| xBL   | (n--)       | File: Load code from file (block-nnn.src). This can be nested.
| xFL   | (--)        | File: Load code from ./Code.S4.
| xFS   | (--)        | File: Save code  to  ./Code.S4.
| xFO   | (n m--h)    | File: Open   - n: file name, m: mode, h: file handle (0 means not opened)
| xFC   | (h--)       | File: Close  - h: file handle
| xFD   | (n--)       | File: Delete - n: file name
| xFR   | (h--c f)    | File: Read   - h: file handle, c: char, n: success?
| xFW   | (c h--f)    | File: Write  - h: file handle, c: char, n: success?
|       |             | NOTE: File operations are enabled by #define __FILES__
| xPI   | (p--)       | Arduino: Pin Input  (pinMode(p, INPUT))
| xPU   | (p--)       | Arduino: Pin Pullup (pinMode(p, INPUT_PULLUP))
| xPO   | (p--)       | Arduino: Pin Output (pinMode(p, OUTPUT)
| xPRA  | (p--n)      | Arduino: Pin Read Analog  (n = analogRead(p))
| xPRD  | (p--n)      | Arduino: Pin Read Digital (n = digitalRead(p))
| xPWA  | (n p--)     | Arduino: Pin Write Analog  (analogWrite(p, n))
| xPWD  | (n p--)     | Arduino: Pin Write Digital (digitalWrite(p, n))
| xR    | (n--r)      | r: a random number in the range [0..(n-1)]
|       |             | NB: if n=0, r is the entire 32-bit random number
| xT    | (--n)       | Milliseconds (Arduino: millis(), Windows: GetTickCount())
| xN    | (--n)       | Microseconds (Arduino: micros(), Windows: N/A)
| xW    | (n--)       | Wait (Arduino: delay(),  Windows: Sleep())
| xIAF  | (--a)       | INFO: a: address of first function vector
| xIAH  | (--a)       | INFO: a: address of HERE variable
| xIAR  | (--a)       | INFO: a: address of first register vector
| xIAS  | (--a)       | INFO: a: address of beeginning of system structure
| xIAU  | (--a)       | INFO: a: address of beeginning of user area
| xIF   | (--n)       | INFO: n: number of words/functions
| xIH   | (--n)       | INFO: n: value of HERE variable
| xIR   | (--n)       | INFO: n: number of registers
| xIU   | (--n)       | INFO: n: number of bytes in the USER area
| xSR   | (--)        | S4 system reset
| xQ    | (--)        | PC: Exit S4
