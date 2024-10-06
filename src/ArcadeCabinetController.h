#ifndef ARCADE_CABINET_CONTROLLER_H
#define ARCADE_CABINET_CONTROLLER_H

#include "USB.h"
#include "USBHIDKeyboard.h"
#include "Button2.h"
#include "HAMqttDevice.h"
#include <Adafruit_NeoPXL8.h>
#include <Preferences.h>

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

const char* deviceConfig = "{\"identifiers\":\"be6beb17-0012-4a70-bc76-a484d34de5cb\",\"name\":\"ArcadeCabinet\",\"sw_version\":\"2024.10.0\",\"model\":\"ArcadeCabinet\",\"manufacturer\":\"JumpMaster\"}";

HAMqttDevice mqttPowerButton("Power Button", HAMqttDevice::BUTTON);
HAMqttDevice mqttPowerState("Power State", HAMqttDevice::BINARY_SENSOR);
HAMqttDevice mqttParentalMode("Parental Mode", HAMqttDevice::SWITCH);
HAMqttDevice mqttAmplifierEnabledSwitch("Amplifier", HAMqttDevice::SWITCH);
HAMqttDevice mqttVolumeMuteButton("Mute Button", HAMqttDevice::BUTTON);
HAMqttDevice mqttVolumeUpButton("Volume Up Button", HAMqttDevice::BUTTON);
HAMqttDevice mqttVolumeDownButton("Volume Down Button", HAMqttDevice::BUTTON);

bool parentalMode = false;
bool cabinetPowerState = LOW;
bool managedPowerState = LOW;
bool reportedPowerState = cabinetPowerState;

#endif
