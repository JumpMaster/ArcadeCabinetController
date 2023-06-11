#ifndef ARCADE_CABINET_CONTROLLER_H
#define ARCADE_CABINET_CONTROLLER_H

#include <WiFi.h>
#include "secrets.h"
#include "USB.h"
#include "USBHIDKeyboard.h"
#include <ArduinoOTA.h>
#include "Logging.h"
#include <EasyButton.h>
#include <PubSubClient.h>
#include "HAMqttDevice.h"
#include <Adafruit_NeoPixel.h>
#include <ESPAsyncWebSrv.h>

const uint8_t ONBOARD_LED_PIN = 13;

const uint8_t PC_POWER_LED_SENSE_PIN = 12;
const uint8_t PLAYER1_BUTTON_INPUT_PIN = 10;
const uint8_t LED_STRIP_PIN = 8;
const uint8_t PCB_NEOPIXEL_PIN = 33; // Will be 14 on the PCB

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
Adafruit_NeoPixel marqueePixels(NUMPIXELS, LED_STRIP_PIN, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel diagnosticPixel(1, PCB_NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint8_t diagnosticPixelMaxBrightness = 100;
uint8_t diagnosticPixelBrightness = diagnosticPixelMaxBrightness;
bool diagnosticPixelBrightnessDirection = 0;
uint32_t diagnosticPixelColor = 0xFF0000;
uint32_t currentDiagnosticPixelColor = 0xFF0000;;
uint32_t nextDiagnosticPixelUpdate = 0;

LightMode lightMode = LIGHT_MODE_OFF;
uint32_t nextLedStripUpdate = 0;
const uint16_t stripUpdateInterval = 1000 / 30; // 30 FPS
uint32_t nextOnboardLedUpdate = 0;
bool onboardLedState = true;
uint8_t ledBrightness = 0;
uint8_t ledStrip_R = 255;
uint8_t ledStrip_G = 255;
uint8_t ledStrip_B = 255;

bool player1ButtonState = false;
bool resetButtonPressed = false;
bool powerButtonPressed = false;
//EasyButton player1Button(PLAYER1_BUTTON_INPUT_PIN, debounce, pullup, invert);
EasyButton player1Button(PLAYER1_BUTTON_INPUT_PIN, 60, true, true);

bool amplifierEnabled = true;
bool startupComplete = false;

WiFiClient espClient;
AsyncWebServer restAPIserver(80);
PubSubClient mqttClient(espClient);
uint32_t nextMetricsUpdate = 0;
unsigned long wifiReconnectPreviousMillis = 0;
unsigned long wifiReconnectInterval = 30000;
uint8_t wifiReconnectCount = 0;

USBHIDKeyboard Keyboard;

const char* deviceConfig = "{\"identifiers\":\"be6beb17-0012-4a70-bc76-a484d34de5cb\",\"name\":\"ArcadeCabinet\",\"sw_version\":\"1.0\",\"model\":\"ArcadeCabinet\",\"manufacturer\":\"JumpMaster\"}";

HAMqttDevice mqttPowerButton("ArcadeCabinet Power Button", HAMqttDevice::BUTTON, "homeassistant");
HAMqttDevice mqttPowerState("ArcadeCabinet Power State", HAMqttDevice::BINARY_SENSOR, "homeassistant");
HAMqttDevice mqttParentalMode("ArcadeCabinet Parental Mode", HAMqttDevice::SWITCH, "homeassistant");
HAMqttDevice mqttAmplifierEnabledSwitch("ArcadeCabinet Amplifier", HAMqttDevice::SWITCH, "homeassistant");
HAMqttDevice mqttVolumeMuteButton("ArcadeCabinet Mute Button", HAMqttDevice::BUTTON, "homeassistant");
HAMqttDevice mqttVolumeUpButton("ArcadeCabinet Volume Up Button", HAMqttDevice::BUTTON, "homeassistant");
HAMqttDevice mqttVolumeDownButton("ArcadeCabinet Volume Down Button", HAMqttDevice::BUTTON, "homeassistant");

uint32_t nextMqttConnectAttempt = 0;
const uint32_t mqttReconnectInterval = 10000;

volatile bool parentalMode = false;
volatile bool cabinetPowerState = LOW;
bool managedPowerState = LOW;

#endif