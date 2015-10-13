#include <Wire.h>
#include <avr/sleep.h>

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
#define BTN_DUMMY       0xFF

struct InputSwitch {
  unsigned char pin;
  unsigned char state;
  unsigned long time;
  unsigned char code;
};

// I2C data definition
struct I2CJoystickStatus {
  uint16_t buttons; // button status
  uint8_t axis0; // first axis
  uint8_t axis1; // second axis
  uint8_t powerDownRequest; // when true, request the pi to shutdown
  uint8_t audioVolume;
};

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
  {13, HIGH, 0, BTN_A},
  {12, HIGH, 0, BTN_B},
  {11, HIGH, 0, BTN_X},
  {10, HIGH, 0, BTN_Y},
  {7, HIGH, 0, BTN_LT},
  {6, HIGH, 0, BTN_RT},
  {8, HIGH, 0, BTN_START},
  {9, HIGH, 0, BTN_SELECT}
};

#define POWER_LED_PIN 5
unsigned long ledBlinkTimer = 0;
int ledStatus = LOW;
#define POWER_SWITCH_PIN 2

InputSwitch volPlusSwitch = {20, HIGH, 0, BTN_DUMMY};
InputSwitch volMinusSwitch = {21, HIGH, 0, BTN_DUMMY};

// return true if switch state has changed!
bool updateSwitch(struct InputSwitch *sw) {
  int newState = digitalRead(sw->pin);

  if(newState != sw->state && millis() - sw->time > SWITCH_DEBOUNCE_TIME) {
    // change state!
    sw->state = newState;

    // record last update
    sw->time = millis();
    
    return true;
  }
}

void pinInterrupt(void) {
  detachInterrupt(0);
}

unsigned long wakeUpTime = 0;

void sleepNow() {
  // Set sleep enable (SE) bit:
  sleep_enable();
  
  attachInterrupt(0, pinInterrupt, LOW);
  
  // Choose our preferred sleep mode:
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // turn led off
  digitalWrite(POWER_LED_PIN, LOW);
 
  // Put the device to sleep:
  sleep_mode();
 
  // Upon waking up, sketch continues from this point.
  sleep_disable();

  wakeUpTime = millis();
  Serial.println("Awake!!");
}

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
  joystickStatus.powerDownRequest = 0;
  joystickStatus.audioVolume = 100;
  
  // pin configuration
  for(int i = 0; i < sizeof(switches) / sizeof(InputSwitch); i++) {
    pinMode(switches[i].pin, INPUT_PULLUP);
  }

  pinMode(POWER_LED_PIN, OUTPUT);
  digitalWrite(POWER_LED_PIN, HIGH);

  pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);

  pinMode(volPlusSwitch.pin, INPUT_PULLUP);
  pinMode(volMinusSwitch.pin, INPUT_PULLUP);

  // turn led on
  ledStatus = HIGH;
  ledBlinkTimer = millis();
  digitalWrite(POWER_LED_PIN, ledStatus);
}

void scanInput() {
  for(int i = 0; i < sizeof(switches) / sizeof(InputSwitch); i++) {
    if(updateSwitch(&switches[i])) {
      if(switches[i].state == HIGH) // button released
        joystickStatus.buttons &= ~(1 << switches[i].code);
      else // button pressed
        joystickStatus.buttons |= (1 << switches[i].code);
    }
  }

  static uint16_t oldButtons = 0;
  if(joystickStatus.buttons != oldButtons) {
    Serial.println(joystickStatus.buttons);
    oldButtons = joystickStatus.buttons;
  }

  /* shut down!!! */
  if(digitalRead(POWER_SWITCH_PIN) == LOW)
    joystickStatus.powerDownRequest = 1;
  else
    joystickStatus.powerDownRequest = 0;

  // volume
  if(updateSwitch(&volPlusSwitch) && volPlusSwitch.state == LOW) {
    joystickStatus.audioVolume = min(joystickStatus.audioVolume + 5, 100); 
  }

  if(updateSwitch(&volMinusSwitch) && volMinusSwitch.state == LOW) {
    joystickStatus.audioVolume = max(joystickStatus.audioVolume - 5, 0); 
  }
}

void scanAnalog() {
  // read analog stick values
  int x = analogRead(ANALOG_PIN_X);
  int y = analogRead(ANALOG_PIN_Y);

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

  // test sleep
  /*
  if(millis() - wakeUpTime > 5000) {
    Serial.println("Go to sleep");
    sleepNow();
  }
  */

  // blink status led
  if(millis() - ledBlinkTimer > 100) {
    if(ledStatus == LOW) {
      ledStatus = HIGH;
    } else {
      ledStatus = LOW;
    }

    digitalWrite(POWER_LED_PIN, ledStatus);
    ledBlinkTimer = millis();
  }

}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write((char *)&joystickStatus, sizeof(I2CJoystickStatus)); 
}
