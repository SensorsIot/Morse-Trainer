/*
  MORSE TRAINER MTR-3

  Morse Output Digital on pin 12
  1000 Hz Tone on Pin 3

  Copyright (C) 2021 Andreas Spiess

  sendLetter() subroutine based on coding of

  Copyright (C) 2010, 2012 raron
  GNU GPLv3 license (http://www.gnu.org/licenses)
  Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/


  One dot is 6000/speed in letter per minute in ms ( http://www.kent-engineers.com/codespeed.htm )

*/

// #define DEBUG
#define FARNSWORTH

#define VERSION  "Version 6.0"

#include "Definitions.h"
#include <EEPROM.h>
#include <PS2Kbd.h>
#include <Wire.h>
#include <SSD1306.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ArduinoSort.h>
#include <LiquidCrystal_PCF8574.h>

#include "english_words.h"
#include "abbrev.h"
#include "IO.h"
#include "Morse.h"
#include "OTA.h"
#include "MQTT.h"
#include "getContent.h"
#include "Initialize.h"
#include "Coordinator.h"
#include "Receiver.h"

//-----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    Serial.println(__FILE__);
    Serial.println(" begins");

    pinMode(DEBUGPIN0, OUTPUT);
    pinMode(DEBUGPIN1, OUTPUT);
    pinMode(DEBUGPIN2, OUTPUT);
    pinMode(DEBUGPIN3, OUTPUT);
    pinMode(DEBUGPIN4, OUTPUT);
    pinMode(DEBUGPIN5, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(MORSEOUTPUT, OUTPUT);
   // pinMode(TONEPIN, OUTPUT);

    OLEDMutex = xSemaphoreCreateMutex();
    KEYBOARDMutex = xSemaphoreCreateMutex();
    changeSharedData = xSemaphoreCreateMutex();

    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("failed to initialise EEPROM");
        ESP.restart();
    }
    else Serial.println("Timer & EEPROM initialized");

    trainerStatus = transmitterStopped;

    Wire.begin(); 
    if (checkI2Caddress(0x27) == 0) {
        displayType = LCD;
        Serial.println("LCD");
    }
    else {
        if (checkI2Caddress(0x3C) == 0) {
            displayType = OLED;
            Serial.println("OLED");
        }
        else displayType = NONE;
    }

    if (displayType == OLED) {
        display.init();
    }
    if (displayType == LCD) {
        lcd.begin(20, 4); // initialize the lcd
    }

    dispText(String(VERSION), "WiFi connecting...", "");

    setupWiFi();
    dispText(String(VERSION), "WiFi connected", "");
    Serial.println("WiFi initialized");
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // right or wrong LEDs
    blinkLED(RED_LED);
    blinkLED(GREEN_LED);

    // setup tone
    ledcSetup(0, 1000, 8);
    ledcAttachPin(TONEPIN, 0);
    keyboard.begin();
    Serial.println("Keyboard, OLED, and pins initialized");

    morseQueue = xQueueCreate(morse_queue_len, sizeof(char));
    receiverQueue = xQueueCreate(receiver_queue_len, sizeof(letterSentDef));

    reconnect();
    changeTrainerStatus(trainerStatus, initialize);

    
    xTaskCreatePinnedToCore(receiverTask,
        "RECEIVE",
        2048,
        NULL,
        1,
        NULL,
        1);

    xTaskCreatePinnedToCore(coordinatorTask,
        "TRANSMIT",
        4096,
        NULL,
        1,
        NULL,
        1);

    vTaskDelay(100);

    xTaskCreatePinnedToCore(morseTask,
        "MORSE",
        2048,
        NULL,
        1,
        NULL,
        1);
 // Delete "setup and loop" task
 //  vTaskDelete(NULL);

    Serial.println("Setup Done");

}



//---------------------------------------------------------------
void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();  // MQTT client
    ArduinoOTA.handle();
    // Serial.print("Loop: Executing on core ");
    // Serial.println(xPortGetCoreID());
}

//*******************************************************************************************************
