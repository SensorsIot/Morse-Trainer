void loadPriorities(int _menu) {
     
    String wordToSend = "";
    
    switch (_menu)
    {
    case 0: // Propabilities from last exercise
        source = randomized;
        EEPROMread();
        groupLength = 5;
        break;

    case 1: // All letters even distributed
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P1_LETTERS + ii);
        //    Serial.print(char(FIRSTLETTER+ii)); 
        //    Serial.print(" "); 
        //    Serial.println(charProp[ii]);
        }
        groupLength = 5;
        break;

    case 2:  // All letters with clear text propabilities
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P2_CLEARTEXT + ii);
        }
        groupLength = 5;
        break;

    case 3: //Numbers
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P3_NUMBERS + ii);
        }
        groupLength = 5;
        break;

    case 4: // Letters and numbers evenly distributed
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P4_MARKS + ii);
        }
        groupLength = 5;
        break;

    case 5:
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P5_LETTERS_NUMBERS + ii);
        }

        groupLength = 5;
        break;

    case 6: // call signs
        source = callSigns;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P5_LETTERS_NUMBERS + ii);
        }
        wordToSend = getrandomCall(6);
        groupLength = wordToSend.length();
        break;

    case 7: // English words
        source = english;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P5_LETTERS_NUMBERS + ii);
        }
        wordToSend = getRandomWord(0);
        groupLength = wordToSend.length();
        break;

    case 8: // Abbreviations
        source = abbreviations;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P1_LETTERS + ii);
        }
        wordToSend = getRandomAbbrev(0);
        break;

    case 9: // Beginners1
    {
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P6 + ii);
        }
        groupLength = 5;
        break;
    }

    case 10: // Beginners2
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P7 + ii);
        }
        groupLength = 5;
        break;

    case 11: // Beginners3
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P8 + ii);
        }
        groupLength = 5;
        break;

    case 12: // Beginners4
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
            charProp[ii] = pgm_read_byte_near(P9 + ii);
        }
        groupLength = 5;
        break;

    default:
        source = randomized;
        for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) charProp[ii] = P1_LETTERS[ii];
        groupLength = 5;
        break;
    }
}

int inputSpeed() {
    int _speed = 0;
    char _hi;
    int _i = 0;
    byte _num[3];


    _hi = readKeyBlocking();
    while (_hi != 10) {   // enter
        _num[_i++] = _hi - '0';
        _hi = readKeyBlocking();
    }
    int _mult = 1;
    for (_i = 2; _i >= 0; _i--) {
        if (_num[_i] < 10) {
            _speed = _speed + (_num[_i] * _mult);
            _mult = _mult * 10;
        }
    }
    return _speed;
}

void changeTrainerStatus(trainerStatDef &trainerStatus, trainerStatDef _newStat) {
    xSemaphoreTake(changeSharedData, portMAX_DELAY);
    if (trainerStatus != _newStat) {
        trainerStatus = _newStat;
        // Serial.print("New Status: "+String(trainerStatus));
    }
    xSemaphoreGive(changeSharedData);
}

void initializeTrainer() {
    speed = 60;
    char _hi;
    if (xSemaphoreTake(KEYBOARDMutex, portMAX_DELAY) == pdTRUE) {
        setSpeed(float(speed));
        //Initialize Statistics buffer
        for (int ii = 0; ii <= LASTLETTER - FIRSTLETTER; ii++) charPropTransfer[ii] = 1;


        do {
            dispText("Please enter Speed", "", "");
            Serial.println("Entering Speed");
            _hi = 'S';
            xQueueSend(morseQueue, (void*)&_hi, portMAX_DELAY);
            speed = inputSpeed();
            if (speed < 20 || speed > 200)
            {
                dispText("Wrong Speed", "", "");
                vTaskDelay(1000);
                speed = 0;
            }
        } while (speed < 20 || speed > 200);

        dispText("Speed= " + String(speed), "", "");
        Serial.println("Speed= " + String(speed));
        setSpeed(float(speed));
        beep(1, 100);
        vTaskDelay(500);
        _hi = 'M';
        xQueueSend(morseQueue, (void*)&_hi, portMAX_DELAY);
        do {
            menu = readKeyBlocking() - '0';

        } while (menu < 0 || menu > 9);
        beep(1, 100);
        loadPriorities(menu);
        dispText("Speed= " + String(speed), "Menu= " + String(menu), "");
        vTaskDelay(1000);

        startTime = millis();
        clearKeyboard();
        lettersInGroup = 0;
        machineStat = waiting;
        randomSeed(micros() % 255);
        random(100);  // start random at a different point
        lettersSent = 0;     // since last analysis
        lettersError = 0;    // since last analysis
        lettersCorrect = 0;  // since last analysis
        lettersInGroup = 0;
        xQueueReset(morseQueue);
        xQueueReset(receiverQueue);
        changeTrainerStatus(trainerStatus, training);
        Serial.println("Initialized and Ready");
        xSemaphoreGive(KEYBOARDMutex);
    }
}