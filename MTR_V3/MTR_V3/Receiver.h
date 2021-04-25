char hKey = '\0';    // keybuard input
unsigned long reaction;


void changeProp(int _letter, int _change) {
	_letter -= FIRSTLETTER;
	if (_letter > 0 && (_letter < LASTLETTER)) {
		charProp[_letter] += _change;
		if (_change > 0) if (charProp[_letter] > MAXPROP) charProp[_letter] = MAXPROP;
		if (charProp[_letter] < MINPROP) charProp[_letter] = MINPROP;
	}
}


//------------------------------------------------------------------

void adaptTraining() {

	letterSentDef letterReceived;
	unsigned long _keyStrokeTime;

	int distance;
	char _hi;
	char _receivedLetter;
	switch (machineStat) {
	case waiting:
		// Serial.print(uxQueueMessagesWaiting(receiver_queue));
		if (uxQueueMessagesWaiting(receiverQueue) > 4) machineStat = contLost;
		else {
			if (keyboard.available()) {
				hKey = getKey();
				if (hKey == '*') machineStat = endTraining;
				else {
					_keyStrokeTime = millis();
					if (xQueueReceive(receiverQueue, (void*)&letterReceived, 0) == pdTRUE) {
						//Serial.println("Letter available");
						reaction = _keyStrokeTime - letterReceived.sendTime;
						_receivedLetter = letterReceived.letter;

						if (hKey == _receivedLetter) machineStat = correct;
						else machineStat = incorrect;
						Serial.println(String(_receivedLetter) + " - " + hKey + " Err:" + String(lettersError));
						if (_receivedLetter == ' ') Serial.println();

					}
					else {
						Serial.println("Letter unavailable");
						machineStat = incorrect;
						clearKeyboard();
					}
				}
			}
		}
		break;
	case correct:
		if (reactionTime[hKey - FIRSTLETTER] < 1) reactionTime[hKey - FIRSTLETTER] = reaction;  //first mesurement
		reactionTime[hKey - FIRSTLETTER] = ((4 * reactionTime[hKey - FIRSTLETTER]) + reaction) / 5;
		lettersCorrect++;
		setLED(LEDgreen);
		changeProp(hKey, PROP_DOWN);
		machineStat = feedback;
		break;
	case incorrect:
		if (hKey == ' ') machineStat = waitForSpace;
		else {
			lettersError++;
			if (xQueuePeek(receiverQueue, (void*)&letterReceived, 0) == pdTRUE) {
				char _xx = letterReceived.letter;
				if (_xx == hKey) {
					xQueueReceive(receiverQueue, (void*)&letterReceived, 0);
					Serial.println("               Skip: ");
				}
			}
			else Serial.println("weiss nicht");
			setLED(LEDred);
			changeProp(hKey, PROP_UP);
			machineStat = feedback;
		}
		break;
	case feedback:
		trainingDuration = (millis() - startTime) / 1000;  // in seconds
		dispText("Speed=" + String(speed), "Duration=" + String(trainingDuration), "");
		if ((lettersError + lettersCorrect) >= STATLENGTH) machineStat = adjSpeed;
		else machineStat = waiting;
		break;
	case waitForSpace:
		Serial.print("WaitForSpace: ");
		do {
			xQueueReceive(receiverQueue, (void*)&letterReceived, portMAX_DELAY);
			Serial.print(letterReceived.letter);
		} while (letterReceived.letter != ' ');
		machineStat = feedback;
		break;
	case contLost:
		debugSignal4();
		changeTrainerStatus(trainerStatus, lost);
		while (trainerStatus == lost) taskYIELD();
		machineStat = waiting;
		break;
	case adjSpeed:
		averageReactionTime = calcAverageReactionTime();
		distance = 60000 / speed;
		Serial.print("               Errors: " + String(lettersError));
		// Serial.print(" Dist: "+String(distance));
		Serial.print(" AvTime " + String(averageReactionTime));

		ratio = (float)distance / (float)averageReactionTime;
		Serial.print(" Ratio ");
		Serial.print(ratio);
		// transmitData();
		if (ratio > 2.0 && lettersError < 1) {
			Serial.print(" FAST");  // fast speed change
			changeSpeed(10);
		}
		else {
			if (lettersError > 2) {
				changeSpeed(SPEED_DOWN);
				Serial.print(" DOWN");
			}
			else if (lettersError == 0) {
				changeSpeed(SPEED_UP);
				Serial.print(" UP  ");
			}
		}
		Serial.println(" Speed: " + String(speed));

		lettersSent = 0;     // since last analysis
		lettersError = 0;    // since last analysis
		lettersCorrect = 0;  // since last analysis
		machineStat = waiting;
		break;
	case endTraining:
		changeTrainerStatus(trainerStatus, trainingEnd);
		xSemaphoreGive(KEYBOARDMutex);
		break;
	default:
		break;
	}
	if (machineStat != machineStatOld) {
		//    receiverStat();
		machineStatOld = machineStat;
	}
}

// Task: 
void receiverTask(void* parameters) {
	while (true) {
		xSemaphoreTake(KEYBOARDMutex, portMAX_DELAY);
		adaptTraining();
		xSemaphoreGive(KEYBOARDMutex);
		digitalWrite(DEBUGPIN2, 0);
		taskYIELD();
		digitalWrite(DEBUGPIN2, 1);

	}
}