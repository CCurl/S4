# S4
An extremely small and fast stack machine VM, inspired by STABLE. 

See https://w3group.de/stable.html for details on STABLE.

A big thanks to Sandor Schneider for the inspiration for this.

## VM operations:

arithmetic
+  ( a b--a+b)  addition
-  ( a b--a-b)  subtraction
*  ( a b--a*b)  multiplication
/  ( a b--a/b)  division
%  ( a b--a%b)  modulo (division remainder)
_  (   n-- -n)  negate

bit manipulation
& ( a b--a&b)   32 bits and
|  ( a b--a|b)  32 bits or
~  (  n -- n')  not, all bits inversed (0=>1 1=>0)

stack
#  ( a--a a)      duplicate TOS (dup)
\  ( a b--a)      drop TOS (drop)
$  ( a b--b a)    swap top 2 stack items (swap)
@  ( a b--a b a)  push next (over)
++ ( a--a+1)      increment TOS
-- ( a--a-1)      decrement TOS

registers
x   ( --)       select register x (x: [a..z])
;   ( --value)  fetch from selected register
:   ( value--)  store into selected register
?   ( --value)  selected register contains an address. Fetch variable from there.
!   ( value--)  selected register contains an address. Store variable there.
x+  ( --)       select and increment register by 1
x-  ( --)       select and decrement register by 1

functions
{X  ( --)  define function X (X: [A..Z]|[a..z])
}   ( --)  end of definition
fX  ( --)  call function <X>

input/output
.    ( a--)    print TOS as decimal number
,    ( a--)    write TOS (27 is ESC, 10 is newline, etc) (emit)
^    ( --key)  read key, don't wait for newline.
"    ( --)     write string until next " to terminal
0..9 ( --a)    scan decimal number until non digit. 
                  to push multiple values, separate them by space (4711 3333)
                  to enter a negative number use _ (negate) after the number

conditions
<  ( a b--f)  true (-1) if b is < a, false (0) otherwise
>  ( a b--f)  true (-1) if b is > a, false (0) otherwise
=  ( a b--f)  true (-1) if a is queal to b, flase (0) otherwise
(  ( f--)     if f is true, execute content until ), if false skip code until )
[  ( f--f)    begin while loop if f is true. leave f on stack. If f is false, skip code until ]
]  ( f--)     repeat the loop if f is true.

other
B    ( --)     output a space
IA   ( --)     Info: All (CARSV)
IC   ( --)     Info: Code
IA   ( --)     Info: Functions
IR   ( --)     Info: Registers
IS   ( --)     Info: Stack
IV   ( --)     Info: Variables
K    ( a--a1)  multiply TOS by 1000
M    ( --a)    push current time (Arduino millis())
R    ( --)     output a CR/LF
SS   ( --)     Easter egg for Sandor!
XX   ( --)     Reset S4 to initial state
bye  ( --)     exit S4
