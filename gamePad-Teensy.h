/*
 * gamepad-Teensy.h - support for HID devices gamepad / joystick
 */

#include "s4.h"

void gpInit() {
    Joystick.useManualSend(false);
}

addr doGamePad(byte ir, addr pc) {
    ir = *(pc++);
    CELL pin = pop(), val, b = 0;
    switch (ir) {
    case 'P': val = pop();
        if ((BetweenI(pin, 1, GP_BUTTONS)) && (buttonVal[pin] != val)) {
            Joystick.button(pin, val);
            buttonVal[pin] = val;
        }                                                break;
    case 'B': b = pop(); val = DR(pin) ? 0 : 1;
        if ((BetweenI(b, 1, GP_BUTTONS)) && (buttonVal[b] != val)) {
            buttonVal[b] = val;
            Joystick.button(b, 1);
        }
        else { b = 0; }                                   break;
    case 'H': Joystick.hat(AR(pin));                      break;
    case 'L': Joystick.sliderLeft(AR(pin));               break;
    case 'R': Joystick.sliderRight(AR(pin));              break;
    case 'M': Joystick.useManualSend(pin != 0);           break;
    case 'X': Joystick.X(AR(pin));                        break;
    case 'Y': Joystick.Y(AR(pin));                        break;
    case 'Z': Joystick.Z(AR(pin));                        break;
    case 'z': Joystick.Zrotate(AR(pin));                  break;
    case 'W': Joystick.send_now();                        break;
    default:
        isError = 1;
        printString("-notGamepad-");
    }
    if (b) {
        delay(200);
        Joystick.button(b, 0);
    }
    return pc;
}
