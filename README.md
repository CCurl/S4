# S4 - A minimal and extendable stack machine VM/CPU

S4 is a minimal, extendable, and interactive environment where the source code IS the machine code. There is no compilation in S4.

# What is S4?

I think of S4 as a stack-based, RPN, Forth-like, virtual CPU/VM that can have as many registers, functions, and amount of user ram as the system supports.

A register is identified by up to 3 UPPER-case characters, so there is a maximum or (26x26x26) = 17576 registers available. I tend to think of registers as built-in variables. Reading, setting, incrementing or decrementing a register is a single operation.

A function is identified by any number of UPPER-case characters. The maximum number of functions is set in the config.h file.

The number of registers, function vectors, and user memory can be scaled as desired to fit into a system of any size. For example, on an ESP-8266 board, a typical configuration might be 576 (26\*26) registers, 5000 functions, and 24K of user ram. In such a system, register names would be in the range of [aa.zz]. For a RPI Pico, I use 576 registers, 1000 functions, and 128K USER RAM. On a Arduino Leonardo, you might configure the system to have 26 registers and functions, and 1K USER. On a Windows or Linux system, I use 17576 registers (26\*26\*26), 5000 functions, and 1MB USER.

- Example 1: "Hello World!"            - the typical "hello world" program.
- Example 2: 1sA 2sB 3sC rA rB rC ++ . - would print 6.
- Example 4: 32 126\[13,10,rI#."-",\]  - would print the ASCII table
- Example 3: 1000sS 13xPO 1{1 13 xPWD rS xW 0 13 xPWD rS xW} - the typical Arduino "blink" program.

The reference for S4 is here:   https://github.com/CCurl/S4/blob/main/doc/reference.md

Examples for S4 are here: https://github.com/CCurl/S4/blob/main/doc/examples.txt

# Why S4?

There were multiple reasons:

1. Many interpreted environments use tokens and a large SWITCH statement in a loop to execute the user's program. In these systems, the "machine code" (i.e. - byte-code ... the cases in the SWITCH statement) are often arbitrarily assigned and are not human-readable, so they have no meaning to the programmer when looking at the code that is actually being executed. Additionally there is a compiler and/or interpreter, often something similar to Forth, that is used to create the programs in that environment. For these enviromnents, there is a steep learning curve ... the programmer needs to learn the user environment and the hundreds or thousands of user functions in the libraries (or "words" in Forth). I wanted to avoid as much as that as possible, and have only one thing to learn: the machine code.

2. I wanted to be free of the need for a multiple gigabyte tool chain and the edit/compile/run paradigm for developing everyday programs.

3. I wanted a simple, minimal, and interactive programming environment that I could modify easily.

4. I wanted an environment that could be easily configured for and deployed to many different types of development boards via the Arduino IDE.

5. I wanted to be able to use the same environment on my personal computer as well as development boards.

6. I wanted short commands so there was not a lot of typing needed.

S4 is the result of my work towards those goals.

# The implementation of S4

- The entire system is implemented in a few files. The engine is in S4.cpp.
- - There are a few additional files to support optional functionality (e.g - WiFi and File access).
- The same code runs on Windows, Linux, and multiple development boards (via the Arduino IDE).
- See the file "config.h" for system configuration settings.

# WiFi support

Some boards, for example the ESP32 or ESP8266, support WiFi. For those boards, the __WIFI__ directive can be #defined to enable the boards WiFi.
Note that those boards usually also have watchdogs that should also be enabled via the __WATCHDOG__ #define.

# LittleFS support

Some boards support LittleFS. For those boards, the __LITTLEFS__ directive can be #defined to save and load the defined code to the board, so that any user-defined words can be presisted and reloaded across boots. The built-in editor also uses this functionality.

# HID Gamepad/Keyboard support

Some boards support HID emulation. For those boards, the __GAMEPAD__ directive can be #defined to enable that functionality. Note that it uses the "HID-Project" from NicoHood for this.

# Building S4

- The target machine/environment is controlled by the #defines in the file "config.h"
- For Windows, I use Microsoft's Visual Studio (Community edition). I use the x86 configuration.
- For Development boards, I use the Arduino IDE. See the file "config.h" for board-specific settings.
- For Linux systems, I use vi and clang. See the "make" script for more info.
- I do not have an Apple system, so I haven't tried to build S4 for that environment.
- However, being such a simple and minimal C program, it should not be difficult to port S4 to any environment.

S4 was inspired by STABLE. See https://w3group.de/stable.html for details on STABLE.
A big thanks to Sandor Schneider for the inspiration for this project.
