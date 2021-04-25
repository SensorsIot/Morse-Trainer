
LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display


PS2Kbd keyboard(KEYBOARD_DATA, KEYBOARD_CLK);
SSD1306  display(0x3c, 21, 22);

void blinkLED(byte LED) {
  digitalWrite(LED, LOW);
  vTaskDelay(500);
  digitalWrite(LED, HIGH);
  vTaskDelay(500);
}


bool checkI2Caddress(int _address) {
    Wire.beginTransmission(_address);
    return Wire.endTransmission();
}

void dispText(String _line1, String _line2, String _line3) {
    if (xSemaphoreTake(OLEDMutex, portMAX_DELAY)) {
        if (displayType == OLED) {
            display.clear();
            display.drawString(0, 0, _line1);
            display.drawString(0, 20, _line2);
            display.drawString(0, 40, _line3);
            display.display();
        }
        if (displayType == LCD) {
            lcd.setBacklight(255);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(_line1);
            lcd.setCursor(0, 1);
            lcd.print(_line2);
            lcd.setCursor(0, 1);
            lcd.print(_line3);
        }
        xSemaphoreGive(OLEDMutex);
    }
    else Serial.println("LCD busy");
}

void beep(int _number, int _dura) {
    for (int _i = 0; _i < _number; _i++) {
        ledcWrite(0, TONEON);
        digitalWrite(MORSEOUTPUT, HIGH);
        vTaskDelay(_dura);
        ledcWrite(0, TONEOFF);
        digitalWrite(MORSEOUTPUT, LOW);
        vTaskDelay(_dura);
    }
}

byte mapKey(byte _inKey) {
    char _outKey;
    if (_inKey > 96) _inKey -= LETTER_SPACE;
    switch (_inKey) {
    case 33:
        _outKey = '+';
        break;
    case 35:
        _outKey = '*';
        break;
    case 38:
        _outKey = '/';
        break;
    case 95:
        _outKey = '?';
        break;
    case 47:
        _outKey = '-';
        break;
    case 40:
        _outKey = ')';
        break;
    case 41:
        _outKey = '=';
        break;
    case 42:
        _outKey = '(';
        break;
    case 45:
        _outKey = 39;
        break;
    case 'Z':
        _outKey = 'Y';
        break;
    case 'Y':
        _outKey = 'Z';
        break;
    default:
        _outKey = _inKey;
        break;
    }
    return _outKey;
}

char getKey() {
    char _hi=0;
    _hi = mapKey(keyboard.read());
    return _hi;
}

char readKeyBlocking() {
    char _hi=0;
    while (!keyboard.available()) taskYIELD();
    _hi = getKey();
    return _hi;
}

void clearKeyboard() {
    // taskENTER_CRITICAL(&blockInterrupts);
    while (keyboard.available()) {
        keyboard.read();
    }
    // taskEXIT_CRITICAL(&blockInterrupts);
}

void EEPROMread() {
  int _hi = 0;
  for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
    charProp[ii] = EEPROM.readByte(ii);    // read propabilities from EEPROM
    _hi = +EEPROM.readByte(ii);
  }
  if (_hi == 0) {
    for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++)charProp[ii] = pgm_read_byte_near(P1_LETTERS + ii);
  }
}

void EEPROMwrite() {
    for (int ii = 0; ii <= (LASTLETTER - FIRSTLETTER); ii++) {
      EEPROM.writeByte(ii, charProp[ii]);     // write current propabilities to EEPROM for future use
    }
    EEPROM.commit();
    int _x = 0;
}

void setLED(ledColor _color) {
  switch (_color) {
    case LEDoff: // Both Off
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      break;
    case LEDgreen: // Green
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, HIGH);
      break;
    case LEDred: // Red
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);
      break;
    default:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, HIGH);
      break;
  }
}

void debugSignal4() {
    digitalWrite(DEBUGPIN4, 1);
    digitalWrite(DEBUGPIN4, 0);
}

void debugSignal5() {
    digitalWrite(DEBUGPIN5, 1);
    digitalWrite(DEBUGPIN5, 0);
}