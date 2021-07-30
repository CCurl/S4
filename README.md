# S4 - A small and fast stack machine VM

S4 is an simple interpreted system where the source code IS the executable code. There is no compilation needed.

Why S4? There are multiple reasons:

1. Many programming environments use tokens and a large SWITCH statement in a loop to run the user program. In those systems, the tokens (the cases in the SWITCH) are often arbitrarily assigned and are not human-readable, so they have no meaning to the programmer when reading the machine code. Additionally there is a compiler, often something similar to Forth, to work in that environment. In these enviromnents, there is a steep learning curve. The programmer needs to learn the user environment and the hundreds or thousands of user functions in the libraries (or "words" in Forth). In S4, there is only one thing to learn; the source code is the machine code, there is no a compiler, and there aren't thousands of functions/words to learn.

2. I wanted an extremely simple interactive programming environment that could be easily understood and modified. My mantra is that a program should be "intuitively obvious upon casual inspection".

3. I wanted to avoid the need for using a multiple gigabyte tool chain and the edit/compile/run paradigm for developing programs.

4. I wanted a minimal program with a small footprint that could be easily configured and deployed to different development boards via the Ardiuino IDE.

S4 is the result of my work towards those goals.

- The entire system is implemented 4 files: S4.h, S4.cpp, pc-main.cpp, and S4.ino.
- The same code runs on both a Windows PC or development board via the Arduino IDE. 

The reference for S4 is here: https://github.com/CCurl/S4/blob/main/reference.txt

# Building S4

- On the PC, I use Microsoft's Visual Studio (Community edition). 
- For Development boards, I use the Arduino IDE. 
- I do not have an Apple or Linux system, so I haven't tried to compile the program for those environments
- However, being such a simple C program, it should not be difficult to port S4 to those environments.

S4 was inspired by STABLE. See https://w3group.de/stable.html for details on STABLE.
A big thanks to Sandor Schneider for the inspiration for this project.
