#include "I2C.h"

int openI2C(int deviceIdx) {
  // open I2C device  
  int file;
  char filename[2048];
  sprintf(filename, "/dev/i2c-%d", deviceIdx);

  if ((file = open(filename, O_RDWR)) < 0) {
    /* ERROR HANDLING: you can check errno to see what went wrong */
    fprintf(stderr, "Failed to open the i2c bus");
    exit(1);
  }

  return file;
}


int readI2CSlave(int file, int slaveAddress, void *buf, size_t count) {
  // initialize communication
  if (ioctl(file, I2C_SLAVE, slaveAddress) < 0) {
    fprintf(stderr, "I2C: Failed to acquire bus access/talk to slave 0x%x\n", slaveAddress);
    return 0;
  }

  int s = 0;  
  s = read(file, buf, count);
  
  return s; // no error
}
