// http://elinux.org/Interfacing_with_I2C_Devices

#include <stdio.h>
#include <stdlib.h>

#include "I2C.h"
#include "JoystickDevice.h"

#define I2C_GAMEPAD_ADDRESS 0x18
#define UPDATE_FREQ 5000 // ms (200Hz)

typedef struct {
  uint16_t buttons; // button status
  uint8_t axis0; // first axis
  uint8_t axis1; // second axis
  uint8_t powerDownRequest; // when true, request the pi to shutdown
} I2CJoystickStatus;

int readI2CJoystick(int file, I2CJoystickStatus *status) {
  int s = readI2CSlave(file, I2C_GAMEPAD_ADDRESS, status, sizeof(I2CJoystickStatus));
  if(s != sizeof(I2CJoystickStatus))
    return -1; // error

  return 0; // no error
}

#define TestBitAndSendKeyEvent(oldValue, newValue, bit, event) if((oldValue & (1 << bit)) != (newValue & (1 << bit))) sendInputEvent(UInputFIle, EV_KEY, event, (newValue & (1 << bit)) == 0 ? 0 : 1);

void updateUInputDevice(int UInputFIle, I2CJoystickStatus *newStatus, I2CJoystickStatus *status) {
  // update button event
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0, BTN_A);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 1, BTN_B);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 2, BTN_X);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 3, BTN_Y);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 4, BTN_TL);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 5, BTN_TR);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 6, BTN_START);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 7, BTN_SELECT);

  // joystick axis 
  if(newStatus->axis0 != status->axis0) {
    // send event
    int val = newStatus->axis0;
    val = (val - 128) * 4;
    sendInputEvent(UInputFIle, EV_ABS, ABS_X, val);
  }

  if(newStatus->axis1 != status->axis1) {
    // send event
    int val = newStatus->axis1;
    val = (val - 128) * 4;
    sendInputEvent(UInputFIle, EV_ABS, ABS_Y, val);
  }

  status = newStatus;
}

int main(int argc, char *argv[]) {
  // open I2C device  
  int I2CFile = openI2C(1);

  // current joystick status
  I2CJoystickStatus status;
  status.buttons = 0;
  status.axis0 = 127;
  status.axis1 = 127;
  status.powerDownRequest = 0;

  // create uinput device
  int UInputFIle = createUInputDevice();

  printf("Driver ready\n");

  while(1) {
    // read new status from I2C
    I2CJoystickStatus newStatus;
    if(readI2CJoystick(I2CFile, &newStatus) != 0) {
      printf("can't read I2C device!\n");
    } else {
      // everything is ok
      updateUInputDevice(UInputFIle, &newStatus, &status);

      if(newStatus.powerDownRequest) {
	// halt!
	//fprintf(stderr, "Halt request!\n");
	system("halt");
      }
    }

    // sleep until next update
    usleep(UPDATE_FREQ);
  }

  // close file
  close(I2CFile);
  ioctl(UInputFIle, UI_DEV_DESTROY);
}
