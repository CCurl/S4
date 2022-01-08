# S4 - A small and fast stack machine VM/CPU

S4 is an simple, fast, minimal, and interactive environment where the source code IS the machine code. There is no compilation in S4.

# What is S4?

I think of S4 as a stack-based, RPN, Forth-like, virtual CPU/VM that has many registers, function vectors, and any amount of user ram.

A register is identified by up to 3 lower-case characters, so there is a maximum or (26x26x26) = 17576 registers available. I tend to think of registers as built-in variables.

Similarly, a function is identified by up to 3 UPPER-case characters, so there is a maximum or (26x26x26) = 17576 functions available.

The number of registers, function vectors, and user memory can be scaled as necessary to fit into a system of any size. For example, on an ESP-8266 board, a typical configuration might be 676 (26*26) registers and functions, and 24K of user ram. In such a system, register names would be in the range of [aa.zz], and function names would be in the range of [AA..ZZ]. On a Arduino Leonardo, you might configure the system to have 26 registers and functions, and 1K user ram.

- Example 1: "Hello World!" - the typical "hello world" program.
- Example 2: 1a! 2b! 3c! a@ b@ c@ ++ . - would print 6.
- Example 4: 32 126\[13,10,i@#."-",\] - would print the ASCII table
- Example 3: 1000s! 13\`PO 1{1 13\`PWD s@\`W 0 13\`PWD s@`W} - the typical Arduino "blink" program.

The reference for S4 is here:   https://github.com/CCurl/S4/blob/main/reference.txt

Examples for S4 are here: https://github.com/CCurl/S4/blob/main/examples.txt

# Why S4?

There were multiple reasons:

1. Many programming environments use tokens and a large SWITCH statement in a loop to execute the user's program. In those systems, the machine code (aka - byte-code ... the cases in the SWITCH statement) are often arbitrarily assigned and are not human-readable, so they have no meaning to the programmer when looking at the code that is actually being executed. Additionally there is a compiler and/or interpreter, often something similar to Forth, that is used to work in that environment. For these enviromnents, there is a steep learning curve ... the programmer needs to learn the user environment and the hundreds or thousands of user functions in the libraries (or "words" in Forth). I wanted to avoid as much as that as possible, and have only one thing to learn: the machine code.

2. I wanted to be free of the need for a multiple gigabyte tool chain and the edit/compile/run paradigm for developing everyday programs.

3. I wanted a simple, minimal, and interactive programming environment that I could modify easily.

4. I wanted an environment that could be easily configured for and deployed to many different types of development boards via the Arduino IDE.

5. I wanted to be able to use the same environment on my personal computer as well as development boards.

6. I wanted short commands so there was not a lot of typing needed.

S4 is the result of my work towards those goals.

# The implementation of S4

- The entire system is implemented in 5 files: config.h, S4.h, S4.cpp, pc-main.cpp, and S4.ino.
- The same code runs on Windows, Linux, and multiple development boards (via the Arduino IDE).
- See the file "config.h" for system configuration settings.

# WiFi support

Some boards, for example the ESP8266, support WiFi. For those boards, the __HASWIFI__ directive can be #defined to enable the boards WiFi.
Note that those boards usually also have watchdogs that need to be enabled via the __WATCHDOG__ #define.

# LittleFS support

Some boards support LittleFS. For those boards, the __LITTLEFS__ directive can be #defined to save and load the defined code to the board, so that any user-defined words can be reloaded across boots.

# Building S4

- The target machine/environment is controlled by the #defined in the file "config.h"
- For Windows, I use Microsoft's Visual Studio (Community edition). I use the x86 configuration.
- For Development boards, I use the Arduino IDE. See the file "config.h" for board-specific settings.
- For Linux systems, I use vi and clang. See the "make" script for more info.
- I do not have an Apple system, so I haven't tried to build S4 for that environment.
- However, being such a simple and minimal C program, it should not be difficult to port S4 to any environment.

S4 was inspired by STABLE. See https://w3group.de/stable.html for details on STABLE.
A big thanks to Sandor Schneider for the inspiration for this project.
