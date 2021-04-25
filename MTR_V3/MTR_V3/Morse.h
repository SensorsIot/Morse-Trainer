int oneTick = 100; // delay for one element
char morseSignalString[7];    // Morse signal for one character as temporary ASCII string of dots and dashes
float ac = 0.0;               // additiona character pause for Farnsworth
float aw = 0.0;               // additiona character pause for Farnsworth
int morseSignalPos;   


void transformToMorse(char encodeMorseChar) {  // translate character in morse code
	int i;
	// change to capital letter if not
	if (encodeMorseChar > 96) encodeMorseChar -= LETTER_SPACE;  // make letter upper case
	// Scan for the character to send in the Morse table
	for (i = 0; i < morseTableLength; i++) if (pgm_read_byte_near(morseTable + i) == encodeMorseChar) break;
	int morseTablePos = i + 1; // 1-based position

	// Reverse dichotomic / binary tree path tracing

	// Find out what level in the binary tree the character is
	int test;
	for (i = 0; i < morseTreeLevels; i++)
	{
		test = (morseTablePos + (0x0001 << i)) % (0x0002 << i);
		if (test == 0) break;
	}
	int startLevel = i;
	morseSignals = morseTreeLevels - i; // = the number of dots and/or dashes
	morseSignalPos = 0;
	// Travel the reverse path to the top of the morse table
	if (morseSignals > 0) {
		// build the morse signal (backwards from last signal to first)
		for (i = startLevel; i < morseTreeLevels; i++)
		{
			int add = (0x0001 << i);
			test = (morseTablePos + add) / (0x0002 << i);
			if (test & 0x0001 == 1)
			{
				morseTablePos += add;
				// Add a dot to the temporary morse signal string
				morseSignalString[morseSignals - 1 - morseSignalPos++] = '.';
			}
			else
			{
				morseTablePos -= add;
				// Add a dash to the temporary morse signal string
				morseSignalString[morseSignals - 1 - morseSignalPos++] = '-';
			}
		}
	}
	else { // unless it was on the top to begin with (A space character)
		morseSignalString[0] = ' ';
		morseSignalPos = 1;
		morseSignals = 1; // cheating a little; a wordspace for a "morse signal"
	}
	morseSignalString[morseSignalPos] = '\0';
}



void setSpeed(float _speed) { // adjust speed of morse
	float _s = _speed / 5.0;  // real speed in wpm
	float _c; // Farnsworth character speed in wpm
	float _x = 1.2 / _s;;  // unit in real speed
	float _u;   // unit in Farnsworth speed
	float _ta;   // additioo
	float _tc;
	float _tw;

#ifdef FARNSWORTH

	if (_s > 18) _c = _s;
	else _c = 18.0;
	_u = 1.2 / _c;
	oneTick = (int)(_u * 1000.0);

	_ta = ((60.0 * _c) - (37.2 * _s)) / (_s * _c);
	_tc = (3 * _ta) / 19.0;
	_tw = (7 * _ta) / 19.0;
	ac = _tc - (3.0 * _x);
	aw = _tw - 7.0 * _x;

	//  sprintf(data, "Speed= %f, _s= %f, _c= %f, _x= %f, _u= %f , _ta= %f, _tc= %f, _tw= %f, ac= %f, aw= %f", _speed, _s, _c, _x, _u, _ta, _tc, _tw, ac, aw);
	//  Serial.println(data);

#else
	oneTick = (int)(_x * 1000.0);
	ac = 0;
	aw = 0;
	//  sprintf(data, "Speed= %f, _s= %f, _c= %f, _x= %f, _u= %f , _ta= %f, _tc= %f, _tw= %f, ac= %f, aw= %f", _speed, _s, _c, _x, _u, _ta, _tc, _tw, ac, aw);
	//  Serial.println(data);
#endif
}

char generateLetter() //random generate letter according its propability (higher propability means letter is generated more often)
{
	int _pointer;
	int _totProbabilities;
	int _sum;
	int _index;
	// do {

	   // Calculate sum of all propabilities
	_totProbabilities = 0;
	for (int _i = 0; _i <= LASTLETTER - FIRSTLETTER; _i++) _totProbabilities = _totProbabilities + charProp[_i]; // _tot = sum of all probabilities
	_pointer = random(0, _totProbabilities); // stick into one place below max probabilities. Because each letter has its own propability, letters with bigger propabilities get a higher chance for a hit


	// Find the letter with the hit by summing all propabilities
	_index = 0;
	_sum = 0;
	while (_sum <= _pointer) {
		_sum = _sum + charProp[_index];
		_index++;
	}
	//  } while (beforeLetter == _index );  // not twice the same letter
	//  beforeLetter = _index;
	return char(FIRSTLETTER + _index - 1);
}

void queueLetter(char _letterForMorse) {
	letterSent.letter = _letterForMorse;
	letterSent.sendTime = millis();
	//Serial.print("Transmit Letter: ");
	//Serial.println(letterSent.letter);
	while (xQueueSend(receiverQueue, (void*)&letterSent, 500) != pdTRUE) Serial.print(".");

}


//----------------------------------- Main Task -------------------------------

void morseTask(void* parameters) {
	char letterForMorse;
	boolean transmittingMorse = false;  // true during transmission of letter
	boolean ex = false;        // leave the interrupt routine
	int morseStat;            // status of morse sending routine
	int tick;            // used for timing of morse generator
	while (1) {
		xQueueReceive(morseQueue, (void*)&letterForMorse, portMAX_DELAY);
		Serial.println(letterForMorse);
		transformToMorse(letterForMorse);
		ex = false;
		morseStat = 1;
		morseSignalPos = 0;
		transmittingMorse = true;
		lastEntryMorse = xTaskGetTickCount();
		while (transmittingMorse) {
			while ((ex == false) && (transmittingMorse == true)) {
				switch (morseStat) {
				case 1:
				{
					if (digitalRead(MORSEOUTPUT) == LOW)  morseStat = 2;
					else morseStat = 3;
					break;
				}
				case 2:
				{
					morseStat = 7;
					if (morseSignalPos == 0) morseStat = 4;
					if (morseSignalString[morseSignalPos] == '\0') morseStat = 5;
					if (morseSignalString[0] == ' ') morseStat = 6;
					break;
				}
				case 3:
				{
					if (morseSignalString[morseSignalPos] == '.') morseStat = 9;
					else morseStat = 8;
					break;
				}
				case 4:
				{
					digitalWrite(MORSEOUTPUT, HIGH);
					ledcWrite(0, TONEON);
					morseStat = 3;
					ex = true;
					tick = 0;
					break;
				}
				case 5:
				{
					if (tick < ENDTICK) morseStat = 10;
					else morseStat = 11;
					break;
				}
				case 6:
				{
					if (tick < SPACETICK) morseStat = 12;
					else morseStat = 13;
					break;
				}
				case 7:
				{
					digitalWrite(MORSEOUTPUT, HIGH);
					ledcWrite(0, TONEON);
					ex = true;
					morseStat = 3;
					break;
				}
				case 8:
				{
					if (tick < DASHTICK) morseStat = 15;
					else morseStat = 14;
					break;
				}
				case 9:
				{
					digitalWrite(MORSEOUTPUT, LOW);
					ledcWrite(0, TONEOFF);
					morseSignalPos++;
					morseStat = 2;
					ex = true;
					break;
				}
				case 10:
				{
					tick++;
					morseStat = 5;
					ex = true;
					break;
				}
				case 11:
				{
					tick = 0;
					ex = true;
					queueLetter(letterForMorse);
					transmittingMorse = false;
					if (ac > 0)vTaskDelay((int)(ac * 1000.0)); // wait for Farnsworth
					break;
				}
				case 12:
				{
					tick++;
					morseStat = 6;
					ex = true;
					break;
				}
				case 13:
				{
					tick = 0;

					queueLetter(letterForMorse);
					transmittingMorse = false;
					if (aw > 0) vTaskDelay((int)(aw * 1000.00)); // Farnsworth enlarge word space
					break;
				}
				case 14:
				{
					digitalWrite(MORSEOUTPUT, LOW);
					ledcWrite(0, TONEOFF);
					tick = 0;
					morseSignalPos++;
					morseStat = 1;
					ex = true;
					break;
				}
				case 15:
				{
					tick++;
					morseStat = 8;
					ex = true;
					break;
				}

				default:
				{
					morseStat = 1;
				}
				}
			}
			ex = false;
			//digitalWrite(DEBUGPIN3, 0);
			// delay(oneTick);
			vTaskDelay(oneTick);
			 // while ((xTaskGetTickCount() - lastEntryMorse) < oneTick) taskYIELD();
			lastEntryMorse = xTaskGetTickCount();
			//digitalWrite(DEBUGPIN3, 1);
		}
	}
}


