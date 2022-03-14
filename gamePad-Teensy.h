/*
 * gamepad-Teensy.h - support for HID devices gamepad / joystick
 */

#include "s4.h"

void gpInit() {
    Joystick.useManualSend(false);
}

addr doGamePadOld(byte ir, addr pc) {
    ir = *(pc++);
    CELL button, val;
    switch (ir) {
    case 'b': button = pop(); 
        val = pop();
        Joystick.Button(button, val ? 0 : 1);
        button = 0;
        break;
    case 'B': button = pop(); val = pop();
        val = (val) ? 0 : 1;
        if ((BetweenI(button, 1, 32)) && (oButtonVal[button] != val)) {
            oButtonVal[button] = val;
            Joystick.Button(button, 1);
        } else { button = 0; }
        break;
    case 'X': Joystick.X(pop());                        break;
    case 'Y': Joystick.Y(pop());                        break;
    case 'Z': Joystick.Z(pop());                        break;
    case 'z': Joystick.Zrotate(pop());                  break;
    case 'L': Joystick.sliderLeft(pop());               break;
    case 'H': Joystick.hat(pop());                      break;
    case 'R': Joystick.sliderRight(pop());              break;
    case 'W': Joystick.send_now();                      break;
    default:
        isError = 1;
        printString("-notGamepad-");
    }
    if (button) {
        delay(200);
        Joystick.Button(button, 0);
    }
    return pc;
}


addr doGamePad(byte ir, addr pc) {
    ir = *(pc++);
    CELL pin = pop(), val, button = 0;
    switch (ir) {
    case 'p': val = DR(pin);
        Joystick.Button(pop(), val ? 0 : 1);
        break;
    case 'P': button = pop(); val = DR(pin) ? 0 : 1;
        if ((BetweenI(button, 1, 32)) && (oButtonVal[button] != val)) {
            oButtonVal[button] = val;
            Joystick.Button(button, 1);
        }
        else { button = 0; }
        break;
    case 'X': Joystick.X(AR(pin));                        break;
    case 'Y': Joystick.Y(AR(pin));                        break;
    case 'Z': Joystick.Z(AR(pin));                        break;
    case 'z': Joystick.Zrotate(AR(pin));                  break;
    case 'L': Joystick.sliderLeft(AR(pin));               break;
    case 'H': Joystick.hat(AR(pin));                      break;
    case 'R': Joystick.sliderRight(AR(pin));              break;
    case 'W': Joystick.send_now();                        break;
    default:
        isError = 1;
        printString("-notGamepad-");
    }
    if (button) {
        delay(200);
        Joystick.Button(button, 0);
    }
    return pc;
}
