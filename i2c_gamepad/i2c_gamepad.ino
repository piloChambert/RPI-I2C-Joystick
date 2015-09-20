#include <Wire.h>

#define I2C_ADDRESS 0x18

#define START_BUTTON (1 << 0)


typedef struct {
  uint16_t buttons; // button status
  uint16_t axis0; // first axis
  uint16_t axis1; // second axis
} I2CJoystickStatus;

I2CJoystickStatus status;

int state = HIGH;      // the current state of the output pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin

long time = 0;         // the last time the output pin was toggled
long debounce = 200;   // the debounce time, increase if the output flickers

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

  // default status
  status.buttons = 0;
  status.axis0 = 127;
  status.axis1 = 127;

  // pin configuration
  pinMode(2, INPUT);
}

void loop()
{
  reading = digitalRead(2);

  /*
  if(previous != reading) {
    if(reading == HIGH) {
      Serial.println("HIGH");
    } else {
      Serial.println("LOW");
    }
  }*/

  previous = reading;

  if(reading == HIGH) {
    status.buttons &= ~0x01;
  } else {
    status.buttons |= 0x01;
  }
  
  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  /*
  if (reading == HIGH && previous == LOW && millis() - time > debounce) {
    if (state == HIGH) {
      state = LOW;
      Serial.println("LOW");
    }
    else {
      state = HIGH;
      Serial.println("HIGH");
    }

    time = millis();    
  }

  previous = reading;*/
  
  //delay(1000);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Wire.write((char *)&status, sizeof(I2CJoystickStatus)); 
}
