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

TaskHandle_t loop0Handle;

const uint8_t ONBOARD_LED_PIN = 13;

const uint8_t LED_STRIP_PIN = 1;
const uint8_t PC_POWER_LED_SENSE_PIN = 11;
const uint8_t PLAYER1_BUTTON_INPUT_PIN = 10;
const uint8_t PLAYER1_BUTTON_OUTPUT_PIN = 7;

const uint8_t relay_PIN1 = 38; // PC POWER SWITCH
const uint8_t relay_PIN2 = 33; // PC RESET SWITCH
const uint8_t relay_PIN3 = 9; // TEMP AMPLIFIER
const uint8_t relay_PIN4 = 8; // UNUSED

const uint8_t NUMPIXELS = 34;
Adafruit_NeoPixel pixels(NUMPIXELS, LED_STRIP_PIN, NEO_RGB + NEO_KHZ800);

bool player1ButtonState;
EasyButton player1Button(PLAYER1_BUTTON_INPUT_PIN, 35, true, true);

bool startupComplete = false;

WiFiClient espClient;
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
HAMqttDevice mqttVolumeMuteButton("ArcadeCabinet Mute Button", HAMqttDevice::BUTTON, "homeassistant");
HAMqttDevice mqttVolumeUpButton("ArcadeCabinet Volume Up Button", HAMqttDevice::BUTTON, "homeassistant");
HAMqttDevice mqttVolumeDownButton("ArcadeCabinet Volume Down Button", HAMqttDevice::BUTTON, "homeassistant");

uint32_t nextMqttConnectAttempt = 0;
const uint32_t mqttReconnectInterval = 10000;

volatile bool parentalMode = false;
volatile bool cabinetPowerState = LOW;
bool reportedPowerState = LOW;

#endif