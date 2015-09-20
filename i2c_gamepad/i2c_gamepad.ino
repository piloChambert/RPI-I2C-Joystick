#include <Wire.h>

#define I2C_ADDRESS 0x18

typedef struct {
  uint16_t buttons; // button status
} I2CGamepadStatus;

void setup()
{
  Wire.begin(I2C_ADDRESS);      // join i2c bus 
  Wire.onRequest(requestEvent); // register event

  // prints title with ending line break
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("I2C Gamepad controller");
}

void loop()
{
  delay(1000);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  static int i = 16;

  I2CGamepadStatus status;
  status.buttons = 16 + (i % 2);

  Wire.write((char *)&status, sizeof(I2CGamepadStatus)); 
  
  i++;
}
