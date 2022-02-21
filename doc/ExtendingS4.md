# Adding new functionality to S4:

In run(), in the "switch" statement, there are cases available for direct support of new instructions, mostly unused lowercase characters. New functionality can be put there.

S4 also has "extended" instructions. These are triggered by the 'x' case. All extended instructions begin with 'x'. For extended instructions, function doExt() in called. It behaves in a similar way to run(), by setting 'ir = \*(pc++)' and a switch statement. Ihe "default:" case calls an external function, doCustom(ir, pc). That is where system-specific functionality should be put; (eg - pin manipulation for Arduino Boards). Function doCustom() is given the pc and ir, and needs to return the address where pc should continue.

As an example, I will add Gamepad/Joystick simulation to S4 as extended instructions.

I am using HID-Project from NicoHood (https://github.com/NicoHood/HID).

Import the library into Arduino using "Import Library".

In the doCustom(ir, pc) function (S4.ino), add a new case for ir. In this case, I am adding case 'G'.

```
addr doCustom(byte ir, addr pc) {
    ...
    case 'G': pc = doGamePad(ir, pc);        break;
    ...
}
```

Then it is a simple matter of implementing doGamePad(ir, pc):

```
\#ifdef __GAMEPAD__
\#include <HID-Project.h>
\#include <HID-Settings.h>
addr doGamePad(byte ir, addr pc) {
    ir = *(pc++);
    switch (ir) {
    case 'X': Gamepad.xAxis(pop());          break;
    case 'Y': Gamepad.yAxis(pop());          break;
    case 'P': Gamepad.press(pop());          break;
    case 'R': Gamepad.release(pop());        break;
    case 'A': Gamepad.dPad1(pop());          break;
    case 'B': Gamepad.dPad2(pop());          break;
    case 'L': Gamepad.releaseAll();          break;
    case 'W': Gamepad.write();               break;
    default:
        isError = 1;
        printString("-notGamepad-");
    }
    return pc;
  
}
\#else
addr doGamePad(addr pc) { printString("-noGamepad-"); return pc; }
\#endif
```

The last thing to do is \#define \_\_GAMEPAD\_\_. This is best done in "config.h".

```
...
#elif __BOARD__ == XIAO
  #define __GAMEPAD__
  #define STK_SZ          8
  #define LSTACK_SZ       4
  #define USER_SZ        (22*1024)
  #define NUM_REGS       (26*26)
 ...
```


Now in S4, you can do things like: 

3 xGP xGW  - Press button 3
3 xGR xGW  - Release button 3
