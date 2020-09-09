#include <Wire.h>
#include <avr/sleep.h>

// ------------------------------------------
// Options
// ------------------------------------------
#define USE_VOLUME_BUTTON 0
#define USE_ANALOG_HAT 0 // USE_VOLUME_BUTTON should be 0 in order to use USE_ANALOG_HAT = 0
#define USE_POWER_FUNCTION 0

// ------------------------------------------
// Configuration
// ------------------------------------------
#define I2C_ADDRESS 0x18
#define SWITCH_DEBOUNCE_TIME 50 // ms
#define ANALOG_PIN_X 0
#define ANALOG_PIN_Y 1
#define HAT_PIN_X 2
#define HAT_PIN_Y 3
#define POWER_LED_PIN 5
#define POWER_SWITCH_PIN 2
#define POWER_MOSFET_GATE 4
#define PI_KILL_PIN 3

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
#define BTN_DOWN        0x09
#define BTN_LEFT        0x0A
#define BTN_RIGHT       0x0B
#define BTN_DUMMY       0xFF

struct InputSwitch {
  unsigned char pin;
  unsigned char state;
  unsigned long time;
  unsigned char code;
};

// the 8 switches 
InputSwitch switches[] = {
  {13, HIGH, 0, BTN_A},
  {12, HIGH, 0, BTN_B},
  {11, HIGH, 0, BTN_X},
  {10, HIGH, 0, BTN_Y},
  {7, HIGH, 0, BTN_LT},
  {6, HIGH, 0, BTN_RT},
  {8, HIGH, 0, BTN_START},
  {9, HIGH, 0, BTN_SELECT},
#if USE_ANALOG_HAT == 0
  {20, HIGH, 0, BTN_UP},
  {21, HIGH, 0, BTN_DOWN},
  {16, HIGH, 0, BTN_LEFT},
  {17, HIGH, 0, BTN_RIGHT},
#endif
};

// force analog hat if using volume button
#if USE_VOLUME_BUTTON == 1
#define USE_ANALOG_HAT 1
#endif

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

  // else 
  return false;
}

// I2C data definition
struct I2CJoystickStatus {
  uint16_t buttons; // button status
  uint8_t axis0; // first axis
  uint8_t axis1; // second axis
  uint8_t powerDownRequest; // when true, request the pi to shutdown
  uint8_t audioVolume;
};

I2CJoystickStatus joystickStatus;

InputSwitch powerSwitch = {POWER_SWITCH_PIN, HIGH, 0, 0};
unsigned long powerSwitchPressTime = 0;
bool powerSwitchHasBeenReleased = true; // a flag used to know if the power switch has been released since last state change

unsigned long ledBlinkTimer = 0;
int powerLedStatus = LOW;

#if USE_VOLUME_BUTTON == 1
InputSwitch volPlusSwitch = {20, HIGH, 0, BTN_DUMMY};
InputSwitch volMinusSwitch = {21, HIGH, 0, BTN_DUMMY};
#endif

void (*stateFunction)(void);
unsigned long wakeUpTime = 0;

void startupState() {
  // blink led
  if(millis() - ledBlinkTimer > 500) {
    powerLedStatus = powerLedStatus == HIGH ? LOW : HIGH;
    
    digitalWrite(POWER_LED_PIN, powerLedStatus);
    ledBlinkTimer = millis();
  }
  
  // update power switch
  if(updateSwitch(&powerSwitch)) {
    if(powerSwitch.state == LOW) {
      Serial.println("Pressed");
      powerSwitchPressTime = millis();
    } else {
      powerSwitchHasBeenReleased = true;
    }
  }

  if(powerSwitch.state == LOW) {
    // reset sleep timer
    wakeUpTime = millis();

    // if pressing power for more the 3s, change state
    if(millis() - powerSwitchPressTime > 3000) {
      // go to run state
      stateFunction = runState;

      // reset release flag
      powerSwitchHasBeenReleased = false;
    
      // led is on
      powerLedStatus = HIGH;
      digitalWrite(POWER_LED_PIN, powerLedStatus);

      // power the pi
      digitalWrite(POWER_MOSFET_GATE, LOW);      

      Serial.println("Starting up the pi!");
    }
  }

  // if still in this state after 5s, go to sleep
  if(millis() - wakeUpTime > 5000 && powerSwitch.state == HIGH) {
    Serial.println("Go to sleep");
    delay(1000);
    sleepNow();
  }
}

void runState() {
#if USE_POWER_FUNCTION == 1
  // update power switch
  if(updateSwitch(&powerSwitch)) {
    if(powerSwitch.state == LOW) {
      Serial.println("Pressed");
      powerSwitchPressTime = millis();
    } else {
      powerSwitchHasBeenReleased = true;
    }
  }

  // if the switch has been released since we are in this state
  // if it's pressed and pressed for more than 4s, then change state
  if(powerSwitchHasBeenReleased && powerSwitch.state == LOW && millis() - powerSwitchPressTime > 4000) {
    // go to halt state
    stateFunction = haltState;

    // reset power switch timer to highest value
    powerSwitchHasBeenReleased = false;

    // stoping the pi
    joystickStatus.powerDownRequest = 1;

    Serial.println("Stoping the pi!");
  }
#endif

  // update switch etc
  scanAnalog();
  scanInput();
}

void haltState() {
  // blink status led
  if(millis() - ledBlinkTimer > 100) {
    if(powerLedStatus == LOW) {
      powerLedStatus = HIGH;
    } else {
      powerLedStatus = LOW;
    }

    digitalWrite(POWER_LED_PIN, powerLedStatus);
    ledBlinkTimer = millis();
  }  

  // wait for pi kill signal
  if(digitalRead(PI_KILL_PIN) == LOW) {
    // shut down the power
    digitalWrite(POWER_MOSFET_GATE, HIGH);

    // go back to start state
    stateFunction = startupState;
  }
}

void pinInterrupt(void) {
  detachInterrupt(0);
}

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
  delay(1000);
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
  pinMode(PI_KILL_PIN, INPUT_PULLUP);

#if USE_VOLUME_BUTTON == 1
  pinMode(volPlusSwitch.pin, INPUT_PULLUP);
  pinMode(volMinusSwitch.pin, INPUT_PULLUP);
#endif

  pinMode(POWER_MOSFET_GATE, OUTPUT);

  // switch to startup state
#if USE_POWER_FUNCTION == 1
  stateFunction = startupState;
#else
  stateFunction = runState;
#endif

  // turn led on
  powerLedStatus = HIGH;
  ledBlinkTimer = millis();
  digitalWrite(POWER_LED_PIN, powerLedStatus);
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

#if USE_VOLUME_BUTTON == 1
  // volume
  if(updateSwitch(&volPlusSwitch) && volPlusSwitch.state == LOW) {
    joystickStatus.audioVolume = min(joystickStatus.audioVolume + 5, 100); 
  }

  if(updateSwitch(&volMinusSwitch) && volMinusSwitch.state == LOW) {
    joystickStatus.audioVolume = max(joystickStatus.audioVolume - 5, 0); 
  }
#endif
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
#if USE_ANALOG_HAT == 1
  int hatx = analogRead(HAT_PIN_X);
  if(hatx > 300 && hatx < 400)
    joystickStatus.buttons |= (1 << BTN_LEFT);
  else
    joystickStatus.buttons &= ~(1 << BTN_LEFT);

  if(hatx > 950)
    joystickStatus.buttons |= (1 << BTN_RIGHT);
  else
    joystickStatus.buttons &= ~(1 << BTN_RIGHT);
  
  int haty = analogRead(HAT_PIN_Y);
  if(haty > 300 && haty < 400)
    joystickStatus.buttons |= (1 << BTN_UP);
  else
    joystickStatus.buttons &= ~(1 << BTN_UP);

  if(haty > 950)
    joystickStatus.buttons |= (1 << BTN_DOWN);
  else
    joystickStatus.buttons &= ~(1 << BTN_DOWN);

  /*
  Serial.print("x : "); Serial.print(hatx);
  Serial.print("y : "); Serial.print(haty);
  Serial.println("");*/
#endif
}

void loop() {
  // execute state function
  stateFunction();
  delay(10);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  Wire.write((char *)&joystickStatus, sizeof(I2CJoystickStatus)); 
}
