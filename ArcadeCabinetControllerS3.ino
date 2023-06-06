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

void switchAmplifierState(bool state)
{
    digitalWrite(AMP_POWER_ENABLE_PIN, state); // Turn on/off amplifier
}

void connectToNetwork()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID, wifiPassword);

    Log.setup();

    if (WiFi.waitForConnectResult() == WL_CONNECTED && wifiReconnectCount == 0)
        Log.println("Connected to WiFi");
}

void setupMQTT()
{
    mqttPowerButton.addConfigVar("device", deviceConfig);
    mqttPowerState.addConfigVar("device", deviceConfig);
    mqttParentalMode.addConfigVar("device", deviceConfig);
    mqttVolumeMuteButton.addConfigVar("device", deviceConfig);
    mqttVolumeUpButton.addConfigVar("device", deviceConfig);
    mqttVolumeDownButton.addConfigVar("device", deviceConfig);
    mqttAmplifierEnabledSwitch.addConfigVar("device", deviceConfig);

    mqttClient.setBufferSize(4096);
    mqttClient.setServer(mqtt_server, 1883);
    mqttClient.setCallback(mqttCallback);
}

void mqttConnect()
{
    Log.println("Connecting to MQTT");
    // Attempt to connect
    if (mqttClient.connect(deviceName, mqtt_username, mqtt_password))//, 0, 0, 0, 0, 0))
    {
        Log.println("Connected to MQTT");
        nextMqttConnectAttempt = 0;


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
    else
    {
        Log.println("Failed to connect to MQTT");
        nextMqttConnectAttempt = millis() + mqttReconnectInterval;
    }
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
            if (cabinetPowerState)
            {
                switchAmplifierState(true);
            }
        }
        else
        {
            Log.println("Amp Off");
            amplifierEnabled = false;
            if (cabinetPowerState)
            {
                switchAmplifierState(false);
            }
        }
        mqttClient.publish(mqttAmplifierEnabledSwitch.getStateTopic().c_str(), data, true);
    }
}

void setupOTA()
{
    ArduinoOTA.setHostname(deviceName);
  
    ArduinoOTA.onStart([]()
    {
        Log.println("OTA Start");
    });

    ArduinoOTA.onEnd([]()
    {
        Log.println("OTA End");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
    {
        //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error)
    {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
        {
            Log.println("Auth Failed");
        }
        else if (error == OTA_BEGIN_ERROR)
        {
            Log.println("Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR)
        {
            Log.println("Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR)
        {
            Log.println("Receive Failed");
        }
        else if (error == OTA_END_ERROR)
        {
            Log.println("End Failed");
        }
    });
    
    ArduinoOTA.begin();
}

void setupPixels()
{
    pixels.begin();
    pixels.clear();
    pixels.show();
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
            
            lightMode = LIGHT_MODE_SOLID;
            pixels.fill(pixels.Color(ledStrip_R, ledStrip_G, ledStrip_B), 0, NUMPIXELS);

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
    // start server
    restAPIserver.begin();
}

void setup()
{

    Serial.begin(115200);

    pinMode(ONBOARD_LED_PIN, OUTPUT);
    digitalWrite(ONBOARD_LED_PIN, HIGH);

    pinMode(NEOPIXEL_POWER, OUTPUT);
    pinMode(NEOPIXEL_POWER, LOW);

    pinMode(PC_POWER_LED_SENSE_PIN, INPUT_PULLUP);

    pinMode(PC_POWER_SWITCH_PIN, OUTPUT);
    pinMode(PC_RESET_SWITCH_PIN, OUTPUT);
    pinMode(AMP_POWER_ENABLE_PIN, OUTPUT);

    digitalWrite(PC_POWER_SWITCH_PIN, LOW);
    digitalWrite(PC_RESET_SWITCH_PIN, LOW);
    digitalWrite(AMP_POWER_ENABLE_PIN, LOW);
    digitalWrite(ONBOARD_LED_PIN, HIGH);

    player1Button.begin();

    setupPixels();

    setupMQTT();

    connectToNetwork();

    setupOTA();

    setupRestAPI();

    mqttConnect();

    Keyboard.begin();
    USB.begin();

/*
    //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
    xTaskCreatePinnedToCore(
                    loop0,          // Task function.
                    "Task0",        // name of task.
                    10000,          // Stack size of task
                    NULL,           // parameter of the task
                    0,              // priority of the task
                    &loop0Handle,   // Task handle to keep track of created task
                    0);             // pin task to core 0
*/
}

void manageWiFi()
{
    // if WiFi is down, try reconnecting
    if ((WiFi.status() != WL_CONNECTED) && (millis() - wifiReconnectPreviousMillis >= wifiReconnectInterval))
    {
        if (wifiReconnectCount >= 10)
        {
            ESP.restart();
        }
        
        wifiReconnectCount++;

        connectToNetwork();

        if (WiFi.status() == WL_CONNECTED)
        {
            wifiReconnectCount = 0;
            wifiReconnectPreviousMillis = 0;
            Log.println("Reconnected to WiFi");
        }
        else
        {
          wifiReconnectPreviousMillis = millis();
        }
    }
}

void manageOnboardLED()
{
    /*
    if (startupComplete)
        return;

    // TURN OFF ONBOARD LED ONCE UPTIME IS GREATER THEN 5 SECONDS
    if (millis() > 30000)
    {
        digitalWrite(ONBOARD_LED_PIN, LOW);
        startupComplete = true;
    }
    else */ 
    if (millis() > nextOnboardLedUpdate)
    {
        nextOnboardLedUpdate = millis() + 250;
        onboardLedState = !onboardLedState;
        digitalWrite(ONBOARD_LED_PIN, onboardLedState ? HIGH : LOW);
    }
}

void manageMQTT()
{
    if (mqttClient.connected())
    {
        mqttClient.loop();

        if (millis() > nextMetricsUpdate)
        {
            sendTelegrafMetrics();
            nextMetricsUpdate = millis() + 30000;
        }
    } else if (millis() > nextMqttConnectAttempt)
    {
        mqttConnect();
    }
}

void sendTelegrafMetrics()
{
    if (millis() > nextMetricsUpdate)
    {
        nextMetricsUpdate = millis() + 30000;
        uint32_t uptime = esp_timer_get_time() / 1000000;

        char buffer[150];
        snprintf(buffer, sizeof(buffer),
            "status,device=%s uptime=%d,resetReason=%d,firmware=\"%s\",memUsed=%ld,memTotal=%ld",
            deviceName,
            uptime,
            esp_reset_reason(),
            esp_get_idf_version(),
            (ESP.getHeapSize()-ESP.getFreeHeap()),
            ESP.getHeapSize());
        mqttClient.publish("telegraf/particle", buffer);
    }
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
            if (amplifierEnabled)
            {
                switchAmplifierState(true);
            }
        }
        else // OFF
        {
            lightMode = LIGHT_MODE_OFF;
            switchAmplifierState(false);
        }

        if (mqttClient.connected())
            mqttClient.publish(mqttPowerState.getStateTopic().c_str(), cabinetPowerState ? "ON" : "OFF", true);
    
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
            Log.println("Arcade cabinet reset");
            resetButtonPressed = true;
        }
    }
    else if (cabinetPowerState == LOW && !parentalMode)
    {
        if (player1Button.pressedFor(300) && !powerButtonPressed) // && !player1Button.pressedFor(15000))
        {
            Log.println("Power button pressed");
            powerButtonPressed = true;
            togglePowerSwitch();
        }
    }

    if (player1Button.isPressed() != player1ButtonState)
    {
        player1ButtonState = player1Button.isPressed();
        Log.println(player1ButtonState ? "PRESSED" : "RELEASED");

        if (!player1ButtonState)
        {
            powerButtonPressed = false;
            resetButtonPressed = false;
        }
    }
}

 void manageLedStrip()
 {
    static uint16_t lightIndex = 0;

    if (ledBrightness == 0 && lightMode == LIGHT_MODE_OFF)
        return;

    if (lightMode == LIGHT_MODE_OFF && ledBrightness > 0)
        ledBrightness--;
    else if (lightMode != LIGHT_MODE_OFF && ledBrightness < 255)
        ledBrightness++;


    if (pixels.getBrightness() != ledBrightness)
    {
        pixels.setBrightness(ledBrightness);

        if (lightMode == LIGHT_MODE_SOLID)
            pixels.fill(pixels.Color(ledStrip_R, ledStrip_G, ledStrip_B), 0, NUMPIXELS);
    }


    //if (millis() < nextLedStripUpdate)
    //    return;

    if (lightMode == LIGHT_MODE_RAINBOW)
    {
        lightIndex += 50;
        pixels.rainbow(lightIndex, 1);
    }

    pixels.show();
    //nextLedStripUpdate = millis() + stripUpdateInterval;
 }

void loop() {

    ArduinoOTA.handle();

    manageWiFi();

    manageMQTT();

    manageOnboardLED();

    manageStartButton();

    monitorPowerState();

    managePowerStateChanges();

    manageLedStrip();
};
