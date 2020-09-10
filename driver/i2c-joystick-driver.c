// http://elinux.org/Interfacing_with_I2C_Devices

#include <stdio.h>
#include <stdlib.h>

#include "I2C.h"
#include "JoystickDevice.h"

#define I2C_GAMEPAD_ADDRESS 0x18
#define UPDATE_FREQ 5000 // ms (200Hz)

#define USE_ANALOG_DPAD 0 // use ABS_HAT0X and ABS_HAT0Y instead of BTN_DPAD_* events

typedef struct {
  uint16_t buttons; // button status
  uint8_t axis0; // first axis
  uint8_t axis1; // second axis
  uint8_t powerDownRequest; // when true, request the pi to shutdown
  uint8_t audioVolume;
} I2CJoystickStatus;

int readI2CJoystick(int file, I2CJoystickStatus *status) {
  int s = readI2CSlave(file, I2C_GAMEPAD_ADDRESS, status, sizeof(I2CJoystickStatus));
  if(s != sizeof(I2CJoystickStatus))
    return -1; // error

  return 0; // no error
}

#define TestBitAndSendKeyEvent(oldValue, newValue, bit, event) if((oldValue & (1 << bit)) != (newValue & (1 << bit))) sendInputEvent(UInputFIle, EV_KEY, event, (newValue & (1 << bit)) == 0 ? 0 : 1);

void updateUInputDevice(int UInputFIle, I2CJoystickStatus *newStatus, I2CJoystickStatus *status) {
  // update button event
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x00, BTN_A);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x01, BTN_B);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x02, BTN_X);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x03, BTN_Y);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x04, BTN_TL);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x05, BTN_TR);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x06, BTN_START);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x07, BTN_SELECT);

#if USE_ANALOG_DPAD == 0
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x08, BTN_DPAD_UP);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x09, BTN_DPAD_DOWN);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x0A, BTN_DPAD_LEFT);
  TestBitAndSendKeyEvent(status->buttons, newStatus->buttons, 0x0B, BTN_DPAD_RIGHT);
#else
  // HAT0Y (up/down)
  if((status->buttons & (1 << 0x08)) != (newStatus->buttons & (1 << 0x08)) || (status->buttons & (1 << 0x09)) != (newStatus->buttons & (1 << 0x09))) {
    int32_t val = (newValue & (1 << 0x08)) != 0 ? -1 : ((newValue & (1 << 0x09)) != 0 ? 1 : 0);
    sendInputEvent(UInputFIle, EV_ABS, ABS_HAT0Y, val);  
  }

  // HAT0X (left/right)
  if((status->buttons & (1 << 0x0A)) != (newStatus->buttons & (1 << 0x0A)) || (status->buttons & (1 << 0x0B)) != (newStatus->buttons & (1 << 0x0B))) {
    int32_t val = (newValue & (1 << 0x0A)) != 0 ? -1 : ((newValue & (1 << 0x0B)) != 0 ? 1 : 0);
    sendInputEvent(UInputFIle, EV_ABS, ABS_HAT0X, val);
  }
#endif

  // joystick axis 
  if(newStatus->axis0 != status->axis0) {
    // send event
    int val = newStatus->axis0;
    val = (val - 128) * 4;
    sendInputEvent(UInputFIle, EV_ABS, ABS_X, val);
  }

  if(newStatus->axis1 != status->axis1) {
    // send event
    int val = newStatus->axis1;
    val = (val - 128) * 4;
    sendInputEvent(UInputFIle, EV_ABS, ABS_Y, val);
  }
}

#include <alsa/asoundlib.h>
// input volume : [0..100]
// XXX maybe need some tweak!
// input value should be a dB value, not a % of the dB range
void SetAlsaMasterVolume(long volume)
{
    long min, max;
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    const char *card = "default";
    const char *selem_name = "PCM";

    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, selem_name);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    float v = (float)volume * 0.01 * (float)(max - min) + min;
    snd_mixer_selem_set_playback_volume_all(elem, v);
    snd_mixer_close(handle);
}

int main(int argc, char *argv[]) {
  // open I2C device  
  int I2CFile = openI2C(1);

  // current joystick status
  I2CJoystickStatus status;
  status.buttons = 0;
  status.axis0 = 127;
  status.axis1 = 127;
  status.powerDownRequest = 0;
  status.audioVolume = -1; // so it will be set at first request

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
      updateUInputDevice(UInputFIle, &newStatus, &status);

      if(newStatus.powerDownRequest) {
	// halt!
	system("halt");
      }

      if(newStatus.audioVolume != status.audioVolume) {
	SetAlsaMasterVolume(newStatus.audioVolume);
      }

      status = newStatus;
    }

    // sleep until next update
    usleep(UPDATE_FREQ);
  }

  // close file
  close(I2CFile);
  ioctl(UInputFIle, UI_DEV_DESTROY);
}
