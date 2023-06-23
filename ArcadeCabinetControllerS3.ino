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

void setMarqueeColor(uint8_t R, uint8_t G, uint8_t B)
{
    lightMode = LIGHT_MODE_SOLID;
    marqueePixels.fill(Adafruit_NeoPixel::Color(R, G, B), 0, NUMPIXELS);
}

void restartDevice(AsyncWebServerRequest *request)
{
    ESP.restart();
}

void switchAmplifierState(bool state)
{
    digitalWrite(AMP_POWER_ENABLE_PIN, state); // Turn on/off amplifier
}

void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    char data[length + 1];
    memcpy(data, payload, length);
    data[length] = '\0';

    Log.printf("%s : %s\n", topic, data);

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
        if (strcmp(data, "ON") == 0)
        {
            parentalMode = true;
        }
        else
        {
            parentalMode = false;
        }
        mqttClient.publish(mqttParentalMode.getStateTopic().c_str(), data, true);
        Log.printf("Parental mode turned %s\n", parentalMode ? "on" : "off");
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
        if (strcmp(data, "ON") == 0)
        {
            Log.println("Amp On");
            amplifierEnabled = true;
        }
        else
        {
            Log.println("Amp Off");
            amplifierEnabled = false;
        }
        if (cabinetPowerState)
        {
            switchAmplifierState(amplifierEnabled);
        }
        mqttClient.publish(mqttAmplifierEnabledSwitch.getStateTopic().c_str(), data, true);
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
            uint8_t r, g, b;
            ledStrip_R = request->getParam("r")->value().toInt();
            ledStrip_G = request->getParam("g")->value().toInt();
            ledStrip_B = request->getParam("b")->value().toInt();
            Log.printf("setColor %d:%d:%d\n",  ledStrip_R, ledStrip_G, ledStrip_B);
            
            setMarqueeColor(ledStrip_R, ledStrip_G, ledStrip_B);

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
                lightMode = LIGHT_MODE_RAINBOW;
                Log.println("Mode set to rainbow");
            }
            else if (request->getParam("mode")->value().indexOf("off") >= 0)
            {
                lightMode = LIGHT_MODE_OFF;
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

    setupRestAPI();
    
    Keyboard.begin();
    USB.begin();

    //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
                    loop0,          // Task function.
                    "Task0",        // name of task.
                    10000,          // Stack size of task
                    NULL,           // parameter of the task
                    0,              // priority of the task
                    &loop0Handle,   // Task handle to keep track of created task
                    0);             // pin task to core 0
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
            lightMode = LIGHT_MODE_RAINBOW;
            diagnosticPixelColor2 = NEOPIXEL_MAGENTA;
            if (amplifierEnabled)
            {
                switchAmplifierState(true);
            }
        }
        else // OFF
        {
            lightMode = LIGHT_MODE_OFF;
            diagnosticPixelColor2 = NEOPIXEL_BLACK;
            switchAmplifierState(false);
        }
    
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

        if (lightMode == LIGHT_MODE_SOLID)
            marqueePixels.fill(marqueePixels.Color(ledStrip_R, ledStrip_G, ledStrip_B), 0, NUMPIXELS);
    }


    //if (millis() < nextLedStripUpdate)
    //    return;

    if (lightMode == LIGHT_MODE_RAINBOW)
    {
        lightIndex += 50;
        marqueePixels.rainbow(lightIndex, 1);
    }

    marqueePixels.show();
    //nextLedStripUpdate = millis() + stripUpdateInterval;
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
    const uint8_t MAX_MESSAGE_LENGTH = 255;
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

        if (millis() > message_timeout || (message_pos > 0 && (buffer == 13 || buffer == 10)))
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
                        lightMode = (LightMode) requestedMode;
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
                    Log.printf("%s:%s:%s - %d:%d:%d\n", HR, HG, HB, R, G, B);
                    setMarqueeColor(R, G, B);
                }
            }

            message_pos = 0;
            message_timeout = 0;
        }
        else if (buffer != 13 && buffer != 10)
        {
            message[message_pos++] = buffer;
        }
    }
}

void loop()
{
    StandardLoop();

    manageLocalMQTT();
};

void loop0(void * pvParameters)
{
    // Setup
    setupMarquee();

    player1Button.begin();

    // Loop
    for(;;)
    {
        manageStartButton();

        monitorPowerState();

        managePowerStateChanges();

        manageMarqueePixels();

        manageSerialReceive();
    }
}