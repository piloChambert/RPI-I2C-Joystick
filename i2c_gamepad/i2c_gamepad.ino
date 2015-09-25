#include <Wire.h>

// switch type definition
#define BTN_A           0x00
#define BTN_B           0x01
#define BTN_X           0x02
#define BTN_Y           0x03
#define BTN_LT          0x04
#define BTN_RT          0x05
#define BTN_START       0x06
#define BTN_SELECT      0x07
#define BTN_UP          0x08
#define BTN_BOTTOM      0x09
#define BTN_LEFT        0x0A
#define BTN_RIGHT       0x0B

typedef struct {
  unsigned char pin;
  unsigned char state;
  unsigned long time;
  unsigned char code;
} InputSwitch;

// I2C data definition
typedef struct {
  uint16_t buttons; // button status
  uint8_t axis0; // first axis
  uint8_t axis1; // second axis
} I2CJoystickStatus;

I2CJoystickStatus joystickStatus;


// ------------------------------------------
// Configuration
// ------------------------------------------

#define I2C_ADDRESS 0x18
#define SWITCH_DEBOUNCE_TIME 50 // ms
#define ANALOG_PIN_X 0
#define ANALOG_PIN_Y 1
#define HAT_PIN_X 2
#define HAT_PIN_Y 3

// the 8 switches 
InputSwitch switches[] = {
  {20, HIGH, 0, BTN_A},
  {21, HIGH, 0, BTN_B},
  {5, HIGH, 0, BTN_X},
  {6, HIGH, 0, BTN_Y},
  {9, HIGH, 0, BTN_LT},
  {10, HIGH, 0, BTN_RT},
  {11, HIGH, 0, BTN_START},
  {12, HIGH, 0, BTN_SELECT}
};

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
  joystickStatus.buttons = 0;
  joystickStatus.axis0 = 127;
  joystickStatus.axis1 = 127;

  // pin configuration
  for(int i = 0; i < sizeof(switches) / sizeof(InputSwitch); i++) {
    pinMode(switches[i].pin, INPUT_PULLUP);
  }
}

void scanInput() {
  for(int i = 0; i < sizeof(switches) / sizeof(InputSwitch); i++) {
    // read new state
    int newState = digitalRead(switches[i].pin);

    // if state has changed
    if(newState != switches[i].state && millis() - switches[i].time > SWITCH_DEBOUNCE_TIME) {
      // debug
      /*
      Serial.print("switch ");
      Serial.print(i);
      Serial.print("(");
      Serial.print(switches[i].code);
      Serial.print("): ");
      */

      if(newState == HIGH) {
        // button released
        joystickStatus.buttons &= ~(1 << switches[i].code);
        //Serial.println("HIGH");
      } else {
        // button pressed
        joystickStatus.buttons |= (1 << switches[i].code);
        //Serial.println("LOW");
      }
    
      switches[i].state = newState;
      switches[i].time = millis();
    }
  }

  static uint16_t oldButtons = 0;

  if(oldButtons != joystickStatus.buttons) {
    Serial.println(joystickStatus.buttons);
    oldButtons = joystickStatus.buttons;
  }
}

void scanAnalog() {
  // read analog stick values
  int x = analogRead(ANALOG_PIN_X);
  int y = analogRead(ANALOG_PIN_);Y

  // translate into a valid range
  x = min(max(((x - 240) / 10), 0) << 2, 255);
  y = min(max(((y - 200) / 10), 0) << 2, 255);

  // store them in the I2C data
  joystickStatus.axis0 = x;
  joystickStatus.axis1 = y;

  // read hat values
  int hatx = analogRead(HAT_PIN_X);
  int haty = analogRead(HAT_PIN_X);

  

}

void loop() {
  scanInput();
  scanAnalog();
/*
  if(reading == HIGH) {
    status.buttons &= ~0x01;
  } else {
    status.buttons |= 0x01;
  }*/  
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Wire.write((char *)&joystickStatus, sizeof(I2CJoystickStatus)); 
}
