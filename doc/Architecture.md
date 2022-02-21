S4 architecture:

The main interpreter loop in S4 is the run(addr start) function in the S4.cpp file. Parameter "start" is where the interpreter loop is to begin.

Variables:

pc      The 'program counter'; it points to the NEXT character in the instruction stream.
ir      The 'instruction register'; it holds the CURRENT instruction/opcode.
TOS     The 'top of stack'. This is very heavily used.
N       The 'next on stack'.

Program flow:

Function run(start) is the heart of S4. It is very simple: (1) set pc to start, (2) get the next instruction, (3) execute the instruction, (4) repeat. If there is an error or we hit a "return" (;), quit.

Adding new functionality to S4:

In run(), in the "switch" statement, there are cases available for direct support of new instructions, mostly unused lowercase characters. New functionality can be put there.

S4 also has "extended" instructions. These are triggered by the 'x' case. All extended instructions begin with 'x'. For extended instructions, function doExt() in called. It behaves in a similar way to run(), by setting 'ir = \*(pc++)' and a switch statement. Ihe "default:" case calls an external function, doCustom(ir, pc). That is where system-specific functionality should be put; (eg - pin manipulation for Arduino Boards). Function doCustom() is given the pc and ir, and needs to return the address where pc should continue.

As an example, I will add gamepad support to S4 as extended instructions.

I am using HID-Project from NicoHood (https://github.com/NicoHood/HID).

Import the library into Arduino using "Import Library".

