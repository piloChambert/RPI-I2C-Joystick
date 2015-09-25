RPI-I2C-Joystick
==========
I2C Arduino based joystick for Raspberry Pi in order to be used with [RetroPie](https://github.com/RetroPie/RetroPie-Setup).

Aruino
---------------

The arduino (an ATMega328 running with its internal 8Mhz clock) is used to control the Raspberry Pi. It's used to read joystick buttons and analog axis. 

The [arduino_hardware](https://github.com/piloChambert/RPI-I2C-Joystick/tree/master/arduino_hardware) directory contains the definition for the ATMega328 without quartz configuration (and PINB6 and PINB7 enable).

The [board](https://github.com/piloChambert/RPI-I2C-Joystick/board) directory contains the schematic (and pcb), compatible with raspberry Pi GPIO. THIS IS A WORK IN PROGRESS!!! By now it's tested on a breadboard.

The [i2c_gamepad](https://github.com/piloChambert/RPI-I2C-Joystick/i2c_gamepad) directory contains the actual arduino code. 

Raspberry Pi
------------

The [blob](https://github.com/piloChambert/RPI-I2C-Joystick/blob) directory contains the blob file to be compiled in the boot folder of the pi, in order to activate a /KILL signal when the pi is halted. (see https://www.raspberrypi.org/forums/viewtopic.php?f=41&t=114975 post).

The [driver](https://github.com/piloChambert/RPI-I2C-Joystick/driver) directory contains the code of the user space uinput driver for the arduino I2C joystick.
