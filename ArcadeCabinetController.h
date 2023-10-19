#ifndef ARCADE_CABINET_CONTROLLER_H
#define ARCADE_CABINET_CONTROLLER_H

#include "USB.h"
#include "USBHIDKeyboard.h"
#include <EasyButton.h>
#include "HAMqttDevice.h"
#include <Adafruit_NeoPXL8.h>
#include <ESPAsyncWebSrv.h>
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

bool player1ButtonState = false;
bool resetButtonPressed = false;
bool powerButtonPressed = false;
//EasyButton player1Button(PLAYER1_BUTTON_INPUT_PIN, debounce, pullup, invert);
EasyButton player1Button(PLAYER1_BUTTON_INPUT_PIN, 60, true, true);

bool amplifierEnabled = true;
bool startupComplete = false;

AsyncWebServer restAPIserver(80);

USBHIDKeyboard Keyboard;

const char* deviceConfig = "{\"identifiers\":\"be6beb17-0012-4a70-bc76-a484d34de5cb\",\"name\":\"ArcadeCabinet\",\"sw_version\":\"1.0\",\"model\":\"ArcadeCabinet\",\"manufacturer\":\"JumpMaster\"}";

HAMqttDevice mqttPowerButton("Power Button", HAMqttDevice::BUTTON, "3cabc108-a89c-4a93-9816-0610bff44197", "homeassistant");
HAMqttDevice mqttPowerState("Power State", HAMqttDevice::BINARY_SENSOR, "b420a188-9f30-419d-b7ca-738531a59823", "homeassistant");
HAMqttDevice mqttParentalMode("Parental Mode", HAMqttDevice::SWITCH, "980483e9-5c6c-439c-aa43-f50fb9096bc1", "homeassistant");
HAMqttDevice mqttAmplifierEnabledSwitch("Amplifier", HAMqttDevice::SWITCH, "86c73b4a-b2b5-4113-bcd6-ac7fe0625f6f", "homeassistant");
HAMqttDevice mqttVolumeMuteButton("Mute Button", HAMqttDevice::BUTTON, "4e3b7725-32a9-46b7-a30b-21293d7342b7", "homeassistant");
HAMqttDevice mqttVolumeUpButton("Volume Up Button", HAMqttDevice::BUTTON, "092b3588-ca1a-4e20-8e56-cf78fc21cea8", "homeassistant");
HAMqttDevice mqttVolumeDownButton("Volume Down Button", HAMqttDevice::BUTTON,"039fc8f8-29e2-4da4-b162-0c5cb90316c8", "homeassistant");

bool parentalMode = false;
bool cabinetPowerState = LOW;
bool managedPowerState = LOW;
bool reportedPowerState = cabinetPowerState;

#endif
