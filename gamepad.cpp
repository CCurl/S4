/*
 * gamepad.cpp - support for HID devices gamepad / joystick
 */

#include "s4.h"

#ifdef __GAMEPAD__
#include <HID-Project.h>
#include <HID-Settings.h>
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
#else
addr doGamePad(byte ir, addr pc) { printString("-noGamepad-"); return pc; }
#endif
