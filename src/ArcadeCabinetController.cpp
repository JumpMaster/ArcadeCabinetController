#include "ArcadeCabinetController.h"

void player1ButtonHandler(Button2& btn);
void switchAmplifier(bool state);
void monitorPowerState();

void togglePowerSwitch()
{
    digitalWrite(PC_POWER_SWITCH_PIN, HIGH); // Power Switch
    delay(100);
    digitalWrite(PC_POWER_SWITCH_PIN, LOW);  // Power Switch
}

void toggleResetSwitch()
{
    digitalWrite(PC_RESET_SWITCH_PIN, HIGH); // Reset Switch
    delay(100);
    digitalWrite(PC_RESET_SWITCH_PIN, LOW);  // Reset Switch
}

void setLightMode(LightMode mode, bool saveValue = true)
{
    lightMode = mode;
    if (saveValue)
        preferences.putUChar(KEY_MARQUEE_MODE, lightMode);

    if (mode == LIGHT_MODE_OFF)
    {
        ledMarqueeColor[0] = 0;
        ledMarqueeColor[1] = 0;
        ledMarqueeColor[2] = 0;
    }
}

void setMarqueeColor(uint8_t R, uint8_t G, uint8_t B)
{
    ledMarqueeRequestedColor[0] = R;
    ledMarqueeRequestedColor[1] = G;
    ledMarqueeRequestedColor[2] = B;
    setLightMode(LIGHT_MODE_SOLID);
    preferences.putUChar(KEY_MARQUEE_R, R);
    preferences.putUChar(KEY_MARQUEE_G, G);
    preferences.putUChar(KEY_MARQUEE_B, B);
}

void setParentalMode(bool state)
{
    parentalMode = state;
    preferences.putBool(KEY_PARENTAL_MODE, parentalMode);
    standardFeatures.mqttPublish(mqttParentalMode.getStateTopic().c_str(), parentalMode ? "ON" : "OFF", true);
    Log.printf("Parental mode turned %s\n", parentalMode ? "on" : "off");
}

void setAmplifierState(bool state)
{
    amplifierEnabled = state;

    preferences.putBool(KEY_AMPLIFIER_ENABLED, amplifierEnabled);

    Log.printf("Amp %s\n", amplifierEnabled ? "on" : "off");

    if (standardFeatures.isMQTTConnected())
    {
        standardFeatures.mqttPublish(mqttAmplifierEnabledSwitch.getStateTopic().c_str(), amplifierEnabled ? "ON" : "OFF", true);
    }

    switchAmplifier(cabinetPowerState && amplifierEnabled);
}

void switchAmplifier(bool state)
{
    digitalWrite(AMP_POWER_ENABLE_PIN, state); // Turn on/off amplifier
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    char data[length + 1];
    memcpy(data, payload, length);
    data[length] = '\0';

    //Log.printf("mqttCallback - %s : %s\n", topic, data);


    if (strcmp(mqttRebootButton.getCommandTopic().c_str(), topic) == 0) // Restart Topic
    {
        ESP.restart();
    }
    else if (strcmp(topic, mqttPowerButton.getCommandTopic().c_str()) == 0)
    {
        if (strcmp(data, "PRESS") == 0)
        {
            Log.println("Virtual power button pressed");
            standardFeatures.mqttPublish("log/mqttCallback", "Virtual power button pressed");
            togglePowerSwitch();
            if (cabinetPowerState == LOW)
            {   // Power on
                Log.println("Cabinet power on initiated");
                standardFeatures.mqttPublish("log/mqttCallback", "Cabinet power on initiated");
            }
            else
            {   // Shutdown
                Log.println("Cabinet shutdown initiated");
                standardFeatures.mqttPublish("log/mqttCallback", "Cabinet shutdown initiated");
            }
        }
    }
    else if (strcmp(mqttParentalMode.getCommandTopic().c_str(), topic) == 0)
    {
        bool mode = strcmp(data, "ON") == 0;
        setParentalMode(mode);
    }
    else if (strcmp(topic, mqttVolumeMuteButton.getCommandTopic().c_str()) == 0)
    {
        Log.println("Virtual mute");
        Keyboard.pressRaw(HID_KEY_MUTE);
        Keyboard.releaseRaw(HID_KEY_MUTE);
    }
    else if (strcmp(topic, mqttVolumeUpButton.getCommandTopic().c_str()) == 0)
    {
        Log.println("Virtual volume +");
        Keyboard.pressRaw(HID_KEY_VOLUME_UP);
        Keyboard.releaseRaw(HID_KEY_VOLUME_UP);
    }
    else if (strcmp(topic, mqttVolumeDownButton.getCommandTopic().c_str()) == 0)
    {
        Log.println("Virtual volume -");
        Keyboard.pressRaw(HID_KEY_VOLUME_DOWN);
        Keyboard.releaseRaw(HID_KEY_VOLUME_DOWN);
    }
    else if (strcmp(mqttAmplifierEnabledSwitch.getCommandTopic().c_str(), topic) == 0)
    {
        bool state = strcmp(data, "ON") == 0;
        setAmplifierState(state);
    }
}

void setupMarquee()
{
    marqueePixels.begin();
    marqueePixels.clear();
    marqueePixels.show(); 
}


void onMQTTConnect()
{
    standardFeatures.mqttPublish(parentMQTTDevice.getConfigTopic().c_str(), parentMQTTDevice.getConfigPayload().c_str(), true);

    standardFeatures.mqttSubscribe(mqttRebootButton.getCommandTopic().c_str());

    standardFeatures.mqttPublish(mqttPowerState.getStateTopic().c_str(), cabinetPowerState ? "ON" : "OFF", true);
    standardFeatures.mqttPublish(mqttParentalMode.getStateTopic().c_str(), parentalMode ? "ON" : "OFF", true);
    standardFeatures.mqttPublish(mqttAmplifierEnabledSwitch.getStateTopic().c_str(), amplifierEnabled ? "ON" : "OFF", true);

    standardFeatures.mqttSubscribe(mqttPowerButton.getCommandTopic().c_str());
    standardFeatures.mqttSubscribe(mqttParentalMode.getCommandTopic().c_str());
    standardFeatures.mqttSubscribe(mqttVolumeMuteButton.getCommandTopic().c_str());
    standardFeatures.mqttSubscribe(mqttVolumeUpButton.getCommandTopic().c_str());
    standardFeatures.mqttSubscribe(mqttVolumeDownButton.getCommandTopic().c_str());
    standardFeatures.mqttSubscribe(mqttAmplifierEnabledSwitch.getCommandTopic().c_str());
}

void setupLocalMQTT()
{
    parentMQTTDevice.addHAMqttDevice(&mqttPowerButton);
    parentMQTTDevice.addHAMqttDevice(&mqttPowerState);
    parentMQTTDevice.addHAMqttDevice(&mqttParentalMode);
    parentMQTTDevice.addHAMqttDevice(&mqttAmplifierEnabledSwitch);
    parentMQTTDevice.addHAMqttDevice(&mqttVolumeMuteButton);
    parentMQTTDevice.addHAMqttDevice(&mqttVolumeUpButton);
    parentMQTTDevice.addHAMqttDevice(&mqttVolumeDownButton);
    parentMQTTDevice.addHAMqttDevice(&mqttRebootButton);

    standardFeatures.setMqttCallback(mqttCallback);
    standardFeatures.setMqttOnConnectCallback(onMQTTConnect);
}

void loadSavedPreferences()
{
    preferences.begin("arcadecabinet");

    bool ampState = preferences.getBool(KEY_AMPLIFIER_ENABLED, true);
    setAmplifierState(ampState);

    bool pMode = preferences.getBool(KEY_PARENTAL_MODE, false);
    setParentalMode(pMode);
    
    if (preferences.getBool(KEY_POWER_STATE) == true)
    {
        monitorPowerState(); // Lets check if the cabinet is powered on before continuing.

        if (!cabinetPowerState) // If it's not powered on there's no need to restore the marquee light settings
            return;

        Log.println("Restoring preferences after unexpected restart");

        LightMode mode = (LightMode) preferences.getUChar(KEY_MARQUEE_MODE, 0);

        if (mode == LIGHT_MODE_SOLID)
        {
            uint8_t R = preferences.getUChar(KEY_MARQUEE_R, 255);
            uint8_t G = preferences.getUChar(KEY_MARQUEE_G, 255);
            uint8_t B = preferences.getUChar(KEY_MARQUEE_B, 255);
            setMarqueeColor(R, G, B);
        }
        else
        {
            setLightMode(mode, false);
        }
        
    }
}

void setup()
{
    standardFeatures.enableLogging(deviceName, syslogServer, syslogPort);
    standardFeatures.enableDiagnosticPixel(14);
    standardFeatures.enableDiagnosticLed(13);
    standardFeatures.enableWiFi(wifiSSID, wifiPassword, deviceName);
    standardFeatures.enableOTA(deviceName, otaPassword);
    standardFeatures.enableSafeMode(appVersion);
    standardFeatures.enableMQTT(mqttServer, mqttUsername, mqttPassword, deviceName);

    setupLocalMQTT();

    pinMode(PC_POWER_LED_SENSE_PIN, INPUT_PULLUP);

    pinMode(PC_POWER_SWITCH_PIN, OUTPUT);
    pinMode(PC_RESET_SWITCH_PIN, OUTPUT);
    pinMode(AMP_POWER_ENABLE_PIN, OUTPUT);

    digitalWrite(PC_POWER_SWITCH_PIN, LOW);
    digitalWrite(PC_RESET_SWITCH_PIN, LOW);
    digitalWrite(AMP_POWER_ENABLE_PIN, LOW);

    loadSavedPreferences();
    
    Keyboard.begin();
    USB.begin();

    setupMarquee();

    player1Button.begin(PLAYER1_BUTTON_INPUT_PIN, INPUT, true /* ActiveLow */);
    player1Button.setLongClickTime(longClickTimePowerOn);
    player1Button.setLongClickDetectedHandler(player1ButtonHandler);
}

void monitorPowerState()
{
    // PC Power LED uses an OptoCoupler to ground the pin.  So ON is LOW and OFF is HIGH due to pull up.
    if (digitalRead(PC_POWER_LED_SENSE_PIN) == cabinetPowerState)
    {
        cabinetPowerState = digitalRead(PC_POWER_LED_SENSE_PIN) ? false : true;
    }
}

void managePowerStateChanges()
{
    // MANAGE POWER STATE
    if (managedPowerState != cabinetPowerState)
    {
        managedPowerState = cabinetPowerState;

        if (cabinetPowerState) // ON
        {
            if (lightMode == LIGHT_MODE_OFF)
            {
                setLightMode(LIGHT_MODE_RAINBOW);
            }
                
            standardFeatures.setDiagnosticPixelColor(StandardFeatures::NEOPIXEL_MAGENTA);
            player1Button.setLongClickTime(longClickTimeReset);
        }
        else // OFF
        {
            setLightMode(LIGHT_MODE_OFF);
            standardFeatures.setDiagnosticPixelColor(StandardFeatures::NEOPIXEL_BLACK);
            player1Button.setLongClickTime(longClickTimePowerOn);
        }

        switchAmplifier(amplifierEnabled);
        preferences.putBool(KEY_POWER_STATE, cabinetPowerState);
        Log.printf("Cabinet powering %s\n", cabinetPowerState ? "on" : "off");
    }

    if (reportedPowerState != cabinetPowerState && standardFeatures.isMQTTConnected())
    {
        reportedPowerState = cabinetPowerState;

        standardFeatures.mqttPublish(mqttPowerState.getStateTopic().c_str(), cabinetPowerState ? "ON" : "OFF", true);
    }
}

void player1ButtonHandler(Button2& btn)
{
    
    if (cabinetPowerState == HIGH)
    {
        toggleResetSwitch();
    }
    else if (cabinetPowerState == LOW && !parentalMode)
    {
        togglePowerSwitch();
    }
}

void manageMarqueePixels()
{
    if (millis() < nextLedStripUpdate)
        return;

    static uint16_t lightIndex = 0;

    if (ledBrightness == 0 && lightMode == LIGHT_MODE_OFF)
        return;

    if (lightMode == LIGHT_MODE_OFF && ledBrightness > 0)
        ledBrightness--;
    else if (lightMode != LIGHT_MODE_OFF && ledBrightness < 255)
        ledBrightness++;


    if (marqueePixels.getBrightness() != ledBrightness)
    {
        marqueePixels.setBrightness(ledBrightness);

        //if (lightMode == LIGHT_MODE_SOLID)
        //    marqueePixels.fill(marqueePixels.Color(ledStrip_R, ledStrip_G, ledStrip_B), 0, NUMPIXELS);
    }

    if (lightMode == LIGHT_MODE_RAINBOW)
    {
        lightIndex += 100;
        //marqueePixels.rainbow(lightIndex, 1);

        const uint8_t saturation = 255;
        const uint8_t brightness = 255;
        const uint8_t whiteBuffer = 5;

        for (uint16_t i = whiteBuffer; i < (NUMPIXELS - whiteBuffer); i++)
        {
            uint16_t hue = lightIndex + (i * 65536) / (NUMPIXELS-(whiteBuffer*2));
            uint32_t color = marqueePixels.ColorHSV(hue, saturation, brightness);
            color = marqueePixels.gamma32(color);
            marqueePixels.setPixelColor(i, color);
        }

        for (uint16_t i = 0; i < whiteBuffer; i++)
        {
            marqueePixels.setPixelColor(i, StandardFeatures::NEOPIXEL_WHITE);
            marqueePixels.setPixelColor(NUMPIXELS-1-i, StandardFeatures::NEOPIXEL_WHITE);
        }
    }
    else if (lightMode == LIGHT_MODE_SOLID)
    {
        if (ledMarqueeRequestedColor[0] != ledMarqueeColor[0] ||
            ledMarqueeRequestedColor[1] != ledMarqueeColor[1] ||
            ledMarqueeRequestedColor[2] != ledMarqueeColor[2]);
        {
            for (uint8_t i = 0; i < 3; i++)
            {
                if (ledMarqueeRequestedColor[i] == ledMarqueeColor[i])
                    continue;

                if (ledMarqueeColor[i] > ledMarqueeRequestedColor[i])
                {
                    ledMarqueeColor[i]--;
                }
                else
                {
                    ledMarqueeColor[i]++;
                }
            }
            marqueePixels.fill(marqueePixels.Color(ledMarqueeColor[0], ledMarqueeColor[1], ledMarqueeColor[2]), 0, NUMPIXELS);
        }
    }

    marqueePixels.show();
    nextLedStripUpdate = millis() + stripUpdateInterval;
}

void manageSerialReceive()
{
    const uint8_t MAX_MESSAGE_LENGTH = 8; // MFFFFFF
    static char message[MAX_MESSAGE_LENGTH];
    static unsigned int message_pos = 0;
    uint32_t message_timeout = 0;

    while (Serial.available())
    {
        if (message_timeout == 0)
        {
            message_timeout = millis() + 500;
        }

        char buffer = Serial.read();

        if (buffer != 13 && buffer != 10)
        {
            message[message_pos++] = buffer;
        }

        if (
            millis() > message_timeout ||
            message_pos >= (MAX_MESSAGE_LENGTH-1) ||
            (message_pos > 0 && (buffer == 13 || buffer == 10))
           )
        {
            message[message_pos] = '\0';

            if (message[0] == 'M') // Mode
            {
                if (message_pos == 2)
                {
                    char HM[2] = {message[1], '\0'};
                    uint8_t requestedMode = strtol(HM, NULL, 16);

                    if (requestedMode >= 0 and requestedMode <= 2)
                    {
                        setLightMode((LightMode) requestedMode);
                        Log.printf("Light mode set:%d\n", requestedMode);
                    }
                }
            }
            else if (message[0] == 'C') // Color
            {
                if (message_pos == 7)
                {
                    char HR[3] = {message[1], message[2], '\0'};
                    char HG[3] = {message[3], message[4], '\0'};
                    char HB[3] = {message[5], message[6], '\0'};
                    uint8_t R = strtol(HR, NULL, 16);
                    uint8_t G = strtol(HG, NULL, 16);
                    uint8_t B = strtol(HB, NULL, 16);
                    setMarqueeColor(R, G, B);
                    Log.printf("%s:%s:%s - %d:%d:%d\n", HR, HG, HB, R, G, B);
                }
            }

            message_pos = 0;
            message_timeout = 0;
        }
    }
}

void loop()
{
    standardFeatures.loop();

    if (standardFeatures.isOTARunning())
        return;

    player1Button.loop();

    monitorPowerState();

    managePowerStateChanges();

    manageMarqueePixels();

    manageSerialReceive();
};
