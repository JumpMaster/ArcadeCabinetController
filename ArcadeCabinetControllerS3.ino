#define DIAGNOSTIC_PIXEL_PIN  14

#include "StandardFeatures.h"
#include "ArcadeCabinetControllerS3.h"

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

void restartDevice(AsyncWebServerRequest *request)
{
    request->send(200, "application/json", "{\"message\":\"Restart confirmed\"}");
    ESP.restart();
}

void setParentalMode(bool state)
{
    parentalMode = state;
    preferences.putBool(KEY_PARENTAL_MODE, parentalMode);
    mqttClient.publish(mqttParentalMode.getStateTopic().c_str(), parentalMode ? "ON" : "OFF", true);
    Log.printf("Parental mode turned %s\n", parentalMode ? "on" : "off");
}

void setAmplifierState(bool state)
{
    amplifierEnabled = state;

    preferences.putBool(KEY_AMPLIFIER_ENABLED, amplifierEnabled);

    Log.printf("Amp %s\n", amplifierEnabled ? "on" : "off");

    if (mqttClient.connected())
    {
        mqttClient.publish(mqttAmplifierEnabledSwitch.getStateTopic().c_str(), amplifierEnabled ? "ON" : "OFF", true);
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

    if (strcmp(topic, mqttPowerButton.getCommandTopic().c_str()) == 0)
    {
        if (strcmp(data, "PRESS") == 0)
        {
            Log.println("Virtual power button pressed");
            mqttClient.publish("log/mqttCallback", "Virtual power button pressed");
            togglePowerSwitch();
            if (cabinetPowerState == LOW)
            {   // Power on
                Log.println("Cabinet power on initiated");
                mqttClient.publish("log/mqttCallback", "Cabinet power on initiated");
            }
            else
            {   // Shutdown
                Log.println("Cabinet shutdown initiated");
                mqttClient.publish("log/mqttCallback", "Cabinet shutdown initiated");
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

void restLedControl(AsyncWebServerRequest *request)
{
    if (request->url().indexOf("setColor") >= 0)
    {
        if (request->hasParam("r") &&
            request->hasParam("g") &&
            request->hasParam("b"))
        {
            uint8_t R, G, B;
            R = request->getParam("r")->value().toInt();
            G = request->getParam("g")->value().toInt();
            B = request->getParam("b")->value().toInt();
            Log.printf("setColor %d:%d:%d\n",  R, G, B);
            
            setMarqueeColor(R, G, B);

            request->send(200, "application/json", "{\"message\":\"RGB set\"}");
            return;
        }
        else
        {
            request->send(200, "application/json", "{\"message\":\"Missing params\"}");
            return;
        }
    }
    else if (request->url().indexOf("setMode") >= 0)
    {
        if (request->hasParam("mode"))
        {
            if (request->getParam("mode")->value().indexOf("rainbow") >= 0)
            {
                setLightMode(LIGHT_MODE_RAINBOW);
                Log.println("Mode set to rainbow");
            }
            else if (request->getParam("mode")->value().indexOf("off") >= 0)
            {
                setLightMode(LIGHT_MODE_OFF);
                Log.println("Mode set to off");
            }
            request->send(200, "application/json", "{\"message\":\"Mode set\"}");
            return;
        }
        else
        {
            request->send(200, "application/json", "{\"message\":\"Missing params\"}");
            return;
        }
    }

    request->send(200, "application/json", "{\"message\":\"Unknown request\"}");
}

void setupRestAPI()
{
    // Function to be exposed
    restAPIserver.on("/led", restLedControl);
    restAPIserver.on("/restart", restartDevice);
    // start server
    restAPIserver.begin();
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
    StandardSetup();

    mqttClient.setCallback(mqttCallback);

    pinMode(PC_POWER_LED_SENSE_PIN, INPUT_PULLUP);

    pinMode(PC_POWER_SWITCH_PIN, OUTPUT);
    pinMode(PC_RESET_SWITCH_PIN, OUTPUT);
    pinMode(AMP_POWER_ENABLE_PIN, OUTPUT);

    digitalWrite(PC_POWER_SWITCH_PIN, LOW);
    digitalWrite(PC_RESET_SWITCH_PIN, LOW);
    digitalWrite(AMP_POWER_ENABLE_PIN, LOW);

    loadSavedPreferences();

    setupRestAPI();
    
    Keyboard.begin();
    USB.begin();

    setupMarquee();

    player1Button.begin();
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
                
            diagnosticPixelColor2 = NEOPIXEL_MAGENTA;
        }
        else // OFF
        {
            setLightMode(LIGHT_MODE_OFF);
            diagnosticPixelColor2 = NEOPIXEL_BLACK;
        }

        switchAmplifier(amplifierEnabled);
        preferences.putBool(KEY_POWER_STATE, cabinetPowerState);
        Log.printf("Cabinet powering %s\n", cabinetPowerState ? "on" : "off");
    }
}

void manageStartButton()
{
    // MANAGE PLAYER 1 START BUTTON
    player1Button.read();
    player1Button.update();

    if (cabinetPowerState == HIGH)
    {
        if (player1Button.pressedFor(15000) && !resetButtonPressed)
        {
            toggleResetSwitch();
            Log.println("Reset button pressed");
            resetButtonPressed = true;
        }
    }
    else if (cabinetPowerState == LOW && !parentalMode)
    {
        if (player1Button.pressedFor(300) && !powerButtonPressed)
        {
            togglePowerSwitch();
            Log.println("Power button pressed");
            powerButtonPressed = true;
        }
    }

    if (player1Button.isPressed() != player1ButtonState)
    {
        player1ButtonState = player1Button.isPressed();
        //Log.println(player1ButtonState ? "PRESSED" : "RELEASED");

        if (!player1ButtonState)
        {
            powerButtonPressed = false;
            resetButtonPressed = false;
        }
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
        marqueePixels.rainbow(lightIndex, 1);
    }
    else if (lightMode == LIGHT_MODE_SOLID)
    {
        if (ledMarqueeRequestedColor[0] != ledMarqueeColor[0] ||
            ledMarqueeRequestedColor[1] != ledMarqueeColor[1] ||
            ledMarqueeRequestedColor[2] != ledMarqueeColor[2]);
        {
            for (uint8_t i = 0; i <= 3; i++)
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

void manageLocalMQTT()
{
    if (mqttClient.connected() && mqttReconnected)
    {
        mqttReconnected = false;

        mqttClient.publish(mqttPowerButton.getConfigTopic().c_str(), mqttPowerButton.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttPowerState.getConfigTopic().c_str(), mqttPowerState.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttParentalMode.getConfigTopic().c_str(), mqttParentalMode.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttVolumeMuteButton.getConfigTopic().c_str(), mqttVolumeMuteButton.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttVolumeUpButton.getConfigTopic().c_str(), mqttVolumeUpButton.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttVolumeDownButton.getConfigTopic().c_str(), mqttVolumeDownButton.getConfigPayload().c_str(), true);
        mqttClient.publish(mqttAmplifierEnabledSwitch.getConfigTopic().c_str(), mqttAmplifierEnabledSwitch.getConfigPayload().c_str(), true);

        mqttClient.publish(mqttPowerState.getStateTopic().c_str(), cabinetPowerState ? "ON" : "OFF", true);
        mqttClient.publish(mqttParentalMode.getStateTopic().c_str(), parentalMode ? "ON" : "OFF", true);
        mqttClient.publish(mqttAmplifierEnabledSwitch.getStateTopic().c_str(), amplifierEnabled ? "ON" : "OFF", true);

        mqttClient.subscribe(mqttPowerButton.getCommandTopic().c_str());
        mqttClient.subscribe(mqttParentalMode.getCommandTopic().c_str());
        mqttClient.subscribe(mqttVolumeMuteButton.getCommandTopic().c_str());
        mqttClient.subscribe(mqttVolumeUpButton.getCommandTopic().c_str());
        mqttClient.subscribe(mqttVolumeDownButton.getCommandTopic().c_str());
        mqttClient.subscribe(mqttAmplifierEnabledSwitch.getCommandTopic().c_str());
    }

    if (reportedPowerState != cabinetPowerState)
    {
        reportedPowerState = cabinetPowerState;

        if (mqttClient.connected())
            mqttClient.publish(mqttPowerState.getStateTopic().c_str(), cabinetPowerState ? "ON" : "OFF", true);
    }
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
    StandardLoop();

    manageLocalMQTT();

    manageStartButton();

    monitorPowerState();

    managePowerStateChanges();

    manageMarqueePixels();

    manageSerialReceive();
};