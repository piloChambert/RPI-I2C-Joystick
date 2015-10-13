RPI-I2C-Joystick
==========
I2C Arduino based joystick for Raspberry Pi in order to be used with [RetroPie](https://github.com/RetroPie/RetroPie-Setup).
The Arduino is also used to control audio volume and power management (switch the Raspberry Pi on and off).

Aruino
---------------

![](https://github.com/piloChambert/RPI-I2C-Joystick/documentation/arduino_RPi_Joystick.png)

The arduino (an ATMega328 running with its internal 8Mhz clock) is used to control the Raspberry Pi. It's used to read joystick buttons and analog axis. 

The [arduino_hardware](https://github.com/piloChambert/RPI-I2C-Joystick/tree/master/arduino_hardware) directory contains the definition for the ATMega328 without quartz configuration (and PINB6 and PINB7 enable).

The [board](https://github.com/piloChambert/RPI-I2C-Joystick/board) directory contains the schematic (and pcb), compatible with raspberry Pi GPIO. THIS IS A WORK IN PROGRESS!!! By now it's tested on a breadboard.

The [i2c_gamepad](https://github.com/piloChambert/RPI-I2C-Joystick/i2c_gamepad) directory contains the actual arduino code. 

Raspberry Pi
------------

In order to know when the raspberry pi has been shutdown, the gpio-poweroff overlay is used. This is the /kill signal going to the arduino (HIGH when the raspberry pi is halted).

In /boot/config.txt:
```
dtoverlay=gpio-poweroff,gpiopin=4,active_low="y"
```

The [driver](https://github.com/piloChambert/RPI-I2C-Joystick/driver) directory contains the code of the user space uinput driver for the arduino I2C joystick.

Display Configuration
---------------------

I use a 4.3" 480x272 TFT display using composite video (maybe the worst quality possible). It would be better to use a DPI interface (with the GPIO, an HDMI to DPI converter, or a DSI to DPI converter).

Here's my video and overscan settings in `/boot/config.txt`
```
disable_overscan=1

overscan_left=34
overscan_right=20
overscan_top=16
overscan_bottom=20
overscan_scale=1

# set the frame buffer to match the tft resolution
framebuffer_width=480
framebuffer_height=272
framebuffer_depth=24

# use PAL, it seems way better than NTSC!!
sdtv_mode=2

# 16:9 aspect
sdtv_aspect=3

```

In order to setup the overscan settings, I used https://github.com/popcornmix/set_overscan, and then tweaked the values by hand.
