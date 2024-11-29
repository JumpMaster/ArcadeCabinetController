#ifndef ARCADE_CABINET_CONTROLLER_H
#define ARCADE_CABINET_CONTROLLER_H

#define DIAGNOSTIC_PIXEL
#define DIAGNOSTIC_LED

#include "StandardFeatures.h"
#include "secrets.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "Button2.h"
#include "HAMqttDevice.h"
#include <Adafruit_NeoPXL8.h>
#include <Preferences.h>

StandardFeatures standardFeatures;

Preferences preferences;
const char *KEY_POWER_STATE = "power-state";
const char *KEY_AMPLIFIER_ENABLED = "amp-enabled";
const char *KEY_PARENTAL_MODE = "parental-mode";
const char *KEY_MARQUEE_MODE = "marquee-mode";
const char *KEY_MARQUEE_R = "marquee-r";
const char *KEY_MARQUEE_G = "marquee-g";
const char *KEY_MARQUEE_B = "marquee-b";

const uint8_t PC_POWER_LED_SENSE_PIN = 12;
const uint8_t PLAYER1_BUTTON_INPUT_PIN = 10;
const uint8_t LED_STRIP_PIN = 8;

const uint8_t PC_POWER_SWITCH_PIN = 5;
const uint8_t PC_RESET_SWITCH_PIN = 9;
const uint8_t AMP_POWER_ENABLE_PIN = 18;

typedef enum
{
    LIGHT_MODE_OFF = 0,
    LIGHT_MODE_SOLID = 1,
    LIGHT_MODE_RAINBOW = 2
} LightMode;

const uint8_t NUMPIXELS = 34;
int8_t ledPins[8] = { LED_STRIP_PIN, -1, -1, -1, -1, -1, -1, -1 };
Adafruit_NeoPXL8 marqueePixels(NUMPIXELS, ledPins, NEO_RGB);

LightMode lightMode = LIGHT_MODE_OFF;
uint32_t nextLedStripUpdate = 0;
const uint16_t stripUpdateInterval = 1000 / 120; // 120 FPS
uint8_t ledBrightness = 0;
uint8_t ledMarqueeColor[3] = { 255, 255, 255 };
uint8_t ledMarqueeRequestedColor[3] = { 255, 255, 255 };

Button2 player1Button;
uint32_t longClickTimePowerOn = 300UL;
uint32_t longClickTimeReset = 10000UL;

bool amplifierEnabled = true;
bool startupComplete = false;

USBHIDKeyboard Keyboard;

const char *device_name = "Arcade Cabinet Controller";
const char *device_id = "1488a590-049a-4bec-9716-5d0374d64684";
const char *device_manufacturer = "Kevin Electronics";
const char *device_hardware = "Feather ESP32-S3";
const char *device_version = appVersion;

HAMqttParent parentMQTTDevice(device_name,
                              device_id,
                              device_manufacturer,
                              device_hardware,
                              device_version);

HAMqttDevice mqttPowerButton("Power Button", "8b9ba715-5f64-43f9-99f9-3ec8ba58ef9b", HAMqttDevice::BUTTON);
HAMqttDevice mqttPowerState("Power State", "bd1f76a1-ddbc-4e7a-910b-1b13906dda3c", HAMqttDevice::BINARY_SENSOR);
HAMqttDevice mqttParentalMode("Parental Mode", "38f6a6c0-3e8a-4408-b715-d1509239441f", HAMqttDevice::SWITCH);
HAMqttDevice mqttAmplifierEnabledSwitch("Amplifier Power", "919d534c-ec88-4f01-8804-04b235636bed", HAMqttDevice::SWITCH);
HAMqttDevice mqttVolumeMuteButton("Mute Button", "bd90446a-de23-496a-9a3a-0ca6abc0083c", HAMqttDevice::BUTTON);
HAMqttDevice mqttVolumeUpButton("Volume Up Button", "27e5ef9f-4db3-4eca-a1fd-ee5f6b6c03df", HAMqttDevice::BUTTON);
HAMqttDevice mqttVolumeDownButton("Volume Down Button", "53cf17e7-1d86-4680-b4c5-54804eb641e8", HAMqttDevice::BUTTON);
HAMqttDevice mqttRebootButton("Reboot Controller", "139884f5-e005-40ea-b051-b5151587e7e8", HAMqttDevice::BUTTON);

bool parentalMode = false;
bool cabinetPowerState = LOW;
bool managedPowerState = LOW;
bool reportedPowerState = cabinetPowerState;

#endif
