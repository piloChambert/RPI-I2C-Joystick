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

#define SLAVE_ADDRESS 0x18
#define UPDATE_FREQ 5000 // ms (200Hz)

typedef struct {
  uint16_t buttons; // button status
} I2CGamepadStatus;

int main(int argc, char *argv[]) {
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

  // read I2C
  int s = 0;
  I2CGamepadStatus status;

  while(1) {
    // read gamepad status
    if((s = read(file, &status, sizeof(I2CGamepadStatus))) == sizeof(I2CGamepadStatus)) {
      printf("%d\n", status.buttons);
    } else {
      printf("can't read : %d\n", s);
    }

    usleep(UPDATE_FREQ);
  }

  // close file
  close(file);
}
