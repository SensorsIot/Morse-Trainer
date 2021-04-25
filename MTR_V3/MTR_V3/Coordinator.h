void changeSpeed(int adjustment) {
	speed = speed + adjustment;
	if (speed < 20) speed = 20;
	if (speed > 200) speed = 200;
	setSpeed(float(speed));
}


String createWord(sourceDef _source) {
	String wordToSend = "";
	int i = 0;

	switch (_source) {
	case randomized:
		while (i < groupLength) {
			wordToSend = wordToSend + generateLetter(); // letter;
			i++;
		}
		break;
	case callSigns:
		wordToSend = getrandomCall(6);
		break;
	case english:
		wordToSend = getRandomWord(0);
		break;
	case abbreviations:
		wordToSend = getRandomAbbrev(0);
		break;
	default:
		break;
	};
	return wordToSend;
}


void fillMorseQueue() {
	String nextWord = createWord(source) + ' ';
	int i = 0;
	while (i < nextWord.length() && trainerStatus != lost) {
		while (xQueueSend(morseQueue, (void*)&nextWord[i], portMAX_DELAY));
		i++;
		lettersSent++;
	}
}

void traineeLost() {   // context lost, restart

	unsigned long entryLost;
	char _hi = '\0';

	changeSpeed(SPEED_DOWN);
	dispText("Context Lost", "Speed " + String(speed), "");
	Serial.println("Lost");

	xQueueReset(morseQueue);
	xQueueReset(receiverQueue);
	lettersInGroup = 0;
	machineStat = waiting;
	lettersSent = 0;     // since last analysis
	lettersError = 0;    // since last analysis
	lettersCorrect = 0;  // since last analysis
	lettersInGroup = 0;

	// wait a few second to give trainee time
	entryLost = millis();
	while (((millis() - entryLost) < 3000) && (_hi != '*')) {
		_hi = getKey();
	}
	if (_hi == '*') {
		changeTrainerStatus(trainerStatus, trainingEnd);
	}
	else changeTrainerStatus(trainerStatus, training);

	clearKeyboard();
}

void printSummary() {
	for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
		if (charProp[ii] > 0) {
			Serial.print(char(ii + FIRSTLETTER));
			Serial.print(" Prop ");
			Serial.print(charProp[ii]);
			// Serial.print(" Trans ");
			// Serial.print(charPropTransfer[ii]);
			Serial.print(" Reaction: ");
			Serial.println(reactionTime[ii]);
		}
	}
}

void endOfTraining() {
	xQueueReset(morseQueue);
	xQueueReset(receiverQueue);
	char hi = ' ';
	xQueueSend(morseQueue, (void*)&hi, portMAX_DELAY);
	hi = '+';
	xQueueSend(morseQueue, (void*)&hi, portMAX_DELAY);

	setLED(LEDoff);
	for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
		if (charProp[ii] > 0) charPropTransfer[ii] = ((charProp[ii] + 9) * 100) + (ii + FIRSTLETTER);  // Esatablish Ranking
	}
	// printSummary();
	vTaskDelay(500);
	EEPROMwrite();
	Serial.println("Stored in EEPROM");
	sortArrayReverse(charPropTransfer, LASTLETTER - FIRSTLETTER + 1);

	String propString = "";
	for (int ii = 0; ii < 4; ii++) {
		byte letter = charPropTransfer[ii] % 100;
		propString = propString + char(letter) + ' ' + charProp[letter - FIRSTLETTER] + ' ';
	}
	dispText(propString, "Reaction: " + String(averageReactionTime), "");
	Serial.println("Top Characters: " + propString);
	Serial.print("Average Reaction Time ");
	Serial.println(averageReactionTime);
	transmitFinalData();
	Serial.println("Training ended");
	vTaskDelay(5000);
}

void receiverStat() {
	Serial.print("machineStat ");
	Serial.print(machineStat);
	Serial.print(" receiver_queue len ");
	Serial.print(uxQueueMessagesWaiting(receiverQueue));
	Serial.print(" letter_queue len ");
	Serial.println(uxQueueMessagesWaiting(morseQueue));
}


void coordinatorTask(void* parameters) {
	while (1) {
		// Serial.print("trainerStatus "+String(trainerStatus));
		digitalWrite(DEBUGPIN1, 1);
		switch (trainerStatus) {
		case transmitterStopped:
			taskYIELD();
		case initialize:
			initializeTrainer();
			break;
		case training:
			fillMorseQueue();
			break;
		case lost:
			traineeLost();
			break;
		case trainingEnd:
			endOfTraining();
			initializeTrainer();
			break;
		default:
			break;
		}
		digitalWrite(DEBUGPIN1, 0);
		//Serial.print("Coordinator: Executing on core ");
		// Serial.println(xPortGetCoreID());
		taskYIELD();

	}

}