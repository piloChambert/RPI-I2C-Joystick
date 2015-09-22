// http://elinux.org/Interfacing_with_I2C_Devices

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <linux/input.h>
#include <linux/uinput.h>

#define SLAVE_ADDRESS 0x18
#define UPDATE_FREQ 5000 // ms (200Hz)

typedef struct {
  uint16_t buttons; // button status
  uint16_t axis0; // first axis
  uint16_t axis1; // second axis
} I2CJoystickStatus;

uint16_t switchCode[] = {
  BTN_A,
  BTN_B,
  BTN_X,
  BTN_Y,
  BTN_TL, 
  BTN_TR,
  BTN_SELECT,
  BTN_START
};

int openI2C() {
  // open I2C device  
  int file;
  char *filename = "/dev/i2c-1";
  if ((file = open(filename, O_RDWR)) < 0) {
    /* ERROR HANDLING: you can check errno to see what went wrong */
    fprintf(stderr, "Failed to open the i2c bus");
    exit(1);
  }

  // initialize communication
  if (ioctl(file, I2C_SLAVE, SLAVE_ADDRESS) < 0) {
    fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x\n", SLAVE_ADDRESS);
    exit(1);
  }

  return file;
}

int readI2CJoystick(int file, I2CJoystickStatus *status) {
  int s = 0;
  
  s = read(file, status, sizeof(I2CJoystickStatus));

  if(s != sizeof(I2CJoystickStatus))
    return -1; // error

  return 0; // no error
}

int createUInputDevice() {
  int fd;

  fd = open("/dev/uinput", O_WRONLY | O_NDELAY);
  if(fd < 0) {
    fprintf(stderr, "Can't open uinput device!\n");
    exit(1);
  }
  
    // device structure
  struct uinput_user_dev uidev;
  memset(&uidev, 0, sizeof(uidev));

  // init event  
  int ret = 0;
  ret |= ioctl(fd, UI_SET_EVBIT, EV_KEY);
  ret |= ioctl(fd, UI_SET_EVBIT, EV_REL);

  // bouton
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_A);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_B);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_X);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_Y);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_TL);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_TR);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);
  ret |= ioctl(fd, UI_SET_KEYBIT, BTN_START);
  
  // axis
  ret |= ioctl(fd, UI_SET_EVBIT, EV_ABS);
  ret |= ioctl(fd, UI_SET_ABSBIT, ABS_X);
  uidev.absmin[ABS_X] = -512;
  uidev.absmax[ABS_X] = 511;
  
  ret |= ioctl(fd, UI_SET_ABSBIT, ABS_Y);
  uidev.absmin[ABS_Y] = -512;
  uidev.absmax[ABS_Y] = 511;

  ret |= ioctl(fd, UI_SET_ABSBIT, ABS_HAT0X);
  uidev.absmin[ABS_HAT0X] = -1;
  uidev.absmax[ABS_HAT0X] = 1;

  ret |= ioctl(fd, UI_SET_ABSBIT, ABS_HAT0Y);
  uidev.absmin[ABS_HAT0Y] = -1;
  uidev.absmax[ABS_HAT0Y] = 1;
  
  if(ret) {
    fprintf(stderr, "Error while configuring uinput device!\n");
    exit(1);
  }

  snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "I2C Arduino Joystick");
  uidev.id.bustype = BUS_USB;
  uidev.id.vendor  = 1;
  uidev.id.product = 1;
  uidev.id.version = 1;

  ret = write(fd, &uidev, sizeof(uidev));
  if(ioctl(fd, UI_DEV_CREATE)) {
    fprintf(stderr, "Error while creating uinput device!\n");
    exit(1);    
  }

  return fd;
}

void sendButtonEvent(int fd, uint16_t type, uint16_t code, int32_t value) {
  struct input_event ev;

  memset(&ev, 0, sizeof(ev));

  ev.type = type;
  ev.code = code;
  ev.value = value;
  
  if(write(fd, &ev, sizeof(ev)) < 0) {
    fprintf(stderr, "Error while sending event to uinput device!\n");
  }

  // need to send a sync event
  ev.type = EV_SYN;
  ev.code = SYN_REPORT;
  ev.value = 0;
  write(fd, &ev, sizeof(ev));
  if (write(fd, &ev, sizeof(ev)) < 0) {
    fprintf(stderr, "Error while sending event to uinput device!\n");
  }
}

#define TestBitAndSendKeyEvent(oldValue, newValue, bit, event) if((oldValue & (1 << bit)) != (newValue & (1 << bit))) sendButtonEvent(UInputFIle, EV_KEY, event, (newValue & (1 << bit)) == 0 ? 0 : 1);

int main(int argc, char *argv[]) {
  // open I2C device  
  int I2CFile = openI2C();

  // current joystick status
  I2CJoystickStatus status;
  status.buttons = 0;
  status.axis0 = 127;
  status.axis1 = 127;

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

      // update button event
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 0, BTN_A);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 1, BTN_B);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 2, BTN_X);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 3, BTN_Y);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 4, BTN_TL);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 5, BTN_TR);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 6, BTN_START);
      TestBitAndSendKeyEvent(status.buttons, newStatus.buttons, 7, BTN_SELECT);

      status = newStatus;
    }

    // sleep until next update
    usleep(UPDATE_FREQ);
  }

  // close file
  close(I2CFile);
  ioctl(UInputFIle, UI_DEV_DESTROY);
}
