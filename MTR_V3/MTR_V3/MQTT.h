const char* mqtt_server = "192.168.0.203";

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(GREEN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(GREEN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
    Serial.print("Attempting MQTT connection...");
    unsigned long connectEntry = millis();
    while ((millis() - connectEntry < 5000) || !client.connect("MorseTrainer")) {
        vTaskDelay(500);
    }
    Serial.println();
    if (client.connect("ESP32Client")) {
        Serial.println("MQTT Connected");
        // Once connected, publish an announcement...
        client.publish("MTR/status", "booted");
        // ... and resubscribe
        client.subscribe("inTopic");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      ESP.restart();
    }
  }

int calcAverageReactionTime() {
  int _numb = 0;
  int _avTime = 0;
  int _avReaction=0;
  for (int i = 0; i <= LASTLETTER - FIRSTLETTER; i++) {
    if (reactionTime[i] > 0) {
      _avTime += reactionTime[i];
      _numb++;
    }
  }
  if(_numb!=0) _avReaction = _avTime / _numb;
  return _avReaction;
}

void transmitFinalData() {
  unsigned long entry = millis();
  char msg[400];
  StaticJsonDocument<200> doc;

  if (!client.connected()) {
    reconnect();
  }
#ifdef DEBUG
  Serial.println(F("Transmitting packet ... "));
#endif

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  // NOTE: transmit() is a blocking method!
  //       See example SX128x_Transmit_Interrupt for details
  //       on non-blocking transmission method.

  doc["Sp"] = speed;
  doc["Re"] = averageReactionTime;
  doc["Du"] = trainingDuration;

  for (int ii = 0; ii < 5; ii++) {
    if (charPropTransfer[ii] > 6900) {
      doc[String(ii)] = String(char(charPropTransfer[ii] % 100));   // Transfer Ranking of letters above 50 propability
    }
  }
  JSONmessage = "";
  serializeJson(doc, JSONmessage);
  JSONmessage.toCharArray(msg, JSONmessage.length() + 1);
#ifdef DEBUG
  Serial.println(JSONmessage);
#endif
  Serial.println(msg);
  client.publish("MTR/values", msg);
}


void transmitData() {
  unsigned long entry = millis();
  char msg[200];
  StaticJsonDocument<200> doc;

  if (!client.connected()) {
    reconnect();
  }
#ifdef DEBUG
  Serial.println(F("Transmitting packet ... "));
#endif

  // you can transmit C-string or Arduino string up to
  // 256 characters long
  // NOTE: transmit() is a blocking method!
  //       See example SX128x_Transmit_Interrupt for details
  //       on non-blocking transmission method.

  doc["Sp"] = speed;
  doc["Ratio"] = ratio;
  doc["React"] = averageReactionTime;;
  doc["Dura"] = trainingDuration;
  JSONmessage = "";
  serializeJson(doc, JSONmessage);
  JSONmessage.toCharArray(msg, JSONmessage.length() + 1);
#ifdef DEBUG
  Serial.println(JSONmessage);
#endif
  client.publish("MTR/training", msg);
  Serial.print(millis() - entry);
}
