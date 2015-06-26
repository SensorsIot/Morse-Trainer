/*
MORSE TRAINER MTR -2

Morse Output Digital on pin 12
1000 Hz Tone on Pin 3

Copyright (C) 2014 Andreas Spiess

sendLetter() subroutine based on coding of

Copyright (C) 2010, 2012 raron
GNU GPLv3 license (http://www.gnu.org/licenses)
Details: http://raronoff.wordpress.com/2010/12/16/morse-endecoder/
*/
#include <TimerOne.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <PS2Keyboard.h>


#define morseOutPin 12			// where morse signal is connected
#define tonePin	3				// PMW signal pin for 100 Hz tone
#define TIMER_MULT 1000000		// 100mS set timer duration in microseconds
#define dashTick 2				// lenght of dash (countet from 0)
#define endTick 1
#define spaceTick 4
#define volHigh 30
#define queueLength 10			// queuelength
#define groupLength 5
#define firstLetter 33
#define lastLetter 90			// asc(Last letter) used
#define statLength 10			// length till speed adaption
#define upProp 20				// reduction of prop for wrong letters
#define downProp 5				// reduction of prop for correct letters
#define speedInc 2				// faster speed if zero errors
#define speedDec 4				// reduced speed due to errors
#define BACKLIGHT_PIN 13
#define space 32

#define redPIN 9
#define greenPIN 10



volatile char morseSignalString[7];		// Morse signal for one character as temporary ASCII string of dots and dashes
volatile boolean sendingMorse = false;	// true during transmission of letter
volatile int tick;						// used for timing of morse generator
volatile boolean ex = false;				// leave the interrupt routine
volatile int stat;						// status of morse sending routine

byte charProp[lastLetter - firstLetter + 1];	// Each letter has his own propability. This value is adapted to capabilities of trainee during training session
int morseSignals;						// nr of morse signals to send in one morse character
int morseSignalPos;		//
int speed = 0;							// speed of morse transmission in bpm
char text[20];							// string for text output to LCD

boolean queueFull = false;
char queue[queueLength + 1];				// queue holding letters sent and not keyed in
int queueIndexS = 0;						// index of letter sent
int queueIndexR = 0;						// index of letter keyed in
char hKey = '\0';							// keybuard input
int lGroup = 0;							// number of letters sent without space
int statErrors = 0;						// number of errors before speed analysis is done
int statGroup = 0;						// number of letters sent before speed analysis is done
int addr = 0;								// EEPROM Address
boolean plainText = false;				// plain text probabilities stay constant

/* Different propabilities of characters
P1: Buchstaben
P2: Letters (German propability)
P3: Numbers
P4: punctuation marks
P5: Letters and 	punctuation marks
P6: Beginner 1 (A,E,M,N,T)
P7: Beginner 2 (A,E,G,I,K,M,N,O,R,T)
P8: Beginner 3: (A,D,E,G,I,K,M,N,O,P,R,T,U,W,Z)
P9: Beginner 4: (A,B,C,D,E,F,G,H,I,J,K,M,N,O,P,R,T,U,W,Z)

*/

const byte P1[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
const byte P2[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 11, 17, 27, 90, 8, 16, 27, 43, 2, 7, 19, 14, 56, 12, 4, 0, 37, 34, 31, 21, 5, 10, 1, 1, 7};
const byte P3[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const byte P4[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
const byte P5[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 0, 50, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50};
const byte P6[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0, 0, 50, 50, 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0};
const byte P7[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 50, 50, 0, 0, 50, 0, 50, 0, 0, 0, 0, 0, 0};
const byte P8[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 50, 0, 50, 0, 50, 0, 50, 0, 50, 50, 50, 50, 0, 50, 0, 50, 50, 0, 50, 0, 0, 50};
const byte P9[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 50, 50, 50, 50, 0, 50, 0, 50, 50, 0, 50, 0, 0, 50};


// Morse code binary tree table (or, dichotomic search table)

// ITU with punctuation (but without non-english characters)
const int morseTreetop = 63;
const char morseTable[] PROGMEM = "*5*H*4*S***V*3*I***F***U?*_**2*E*&*L\"**R*+.****A***P@**W***J'1* *6-B*=*D*/"
                                  "*X***N***C;(!K***Y***T*7*Z**,G***Q***M:8*!***O*9***0*";

const int morseTableLength = (morseTreetop * 2) + 1;
const int morseTreeLevels = log(morseTreetop + 1) / log(2);

// PS2 Keyboard
const int DataPin = 7;
const int IRQpin =  2;

PS2Keyboard keyboard;


// set the LCD address to 0x20 for a 20 chars 2 line display
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

void setup()
{
  Serial.begin(9600);
  Timer1.attachInterrupt(transmitMorse);				// attach the ISR routine  here
  Serial.println("Begin");

  pinMode(greenPIN, OUTPUT);
  pinMode(redPIN, OUTPUT);
  pinMode(morseOutPin, OUTPUT);

  // right or wrong LEDs
  digitalWrite(redPIN, HIGH);
  digitalWrite(greenPIN, HIGH);

  lcd.begin(20, 4);	// initialize the lcd for 20 chars 4 lines and turn on backlight
  lcd.backlight();	// finish with backlight on

  keyboard.begin(DataPin, IRQpin, PS2Keymap_German);

  initialize();
  Serial.println("Initialized");
  Serial.println(speed);
  randomSeed(micros() % 255);
  Serial.println(random(100));  // start random at a different point
}


void loop()
{
  //*** S E N D I N G ***

  if (sendingMorse == false)
  {
    if (lGroup < groupLength)
    {
      queue[queueIndexS] = generateLetter(); // new keyqueue[queueIndexS] = letter;
      lGroup++;
    }
    else
    {
      queue[queueIndexS] = ' ';
      lGroup = 0;
    }
    sendLetter(queue[queueIndexS]); // morse letter
    queueIndexS = indexAdv(queueIndexS); // advance index
    statGroup++;
  }

  if (queueDist() >= 5) contextLost(); // if sender lost trainee



  // *** R E C E I V I N G ***

  if (keyboard.available())
  {
    hKey = keyboard.read();
    if (hKey != '*')	// End of training
    {
      if (hKey > 96) hKey -= space;  / make letter upper case

      // hKey = space: Synchronize with sender
      if (hKey == ' ')
      {
        while (queue[queueIndexR] != hKey && queueDist() > 0)
        {
          queueIndexR = indexAdv(queueIndexR);
          wrong(queue[queueIndexR], ' ');
        }
      }

      if (queue[queueIndexR] == hKey) correct(hKey);	// key entered was correct
      else wrong(hKey, queue[queueIndexR] );			    // key entered was wrong

      //too many keys entered
      if (queueDist() == 0) queueIndexR = queueIndexS;

      queueIndexR = indexAdv(queueIndexR);
      lcdDispText(3, 12, "Prop");
      if (hKey != space) lcdDispInt(3, 17, charProp[hKey - firstLetter]);	// display propability of letter
    }
    else
    {
      // *** End of training

      digitalWrite(greenPIN, HIGH);
      digitalWrite(redPIN, HIGH);

      Serial.println("Write");
      for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
      {
        Serial.print(ii);
        Serial.print(char(ii + firstLetter));
        Serial.print(" prop ");
        Serial.println(charProp[ii]);

        EEPROM.write(ii, charProp[ii]);			// write current propabilities to EEPROM for future use
      }
      sendLetter('+');
      while (sendingMorse == true) {}
      delay(1000);
      initialize();			// goto start
    }
  }

  // adjust speed after a number of letters (statLength)
  if (statGroup == statLength)
  {
    analyzeSpeed();
    statGroup = 0;
  }
}

//*******************************************************************************************************

int queueDist()		// distance between sender and trainee
{
  int hi = queueIndexS - queueIndexR;
  if (hi < 0) hi = hi + queueLength + 1;
  return hi;
}

void contextLost()		// context lost, restart
{
  lcdDispText(4, 0, "Context is Lost");
  speed = speed - speedDec;
  lcdDispText(0, 15, "down");
  if (speed < 20) speed = 20;
  setSpeed(speed);
  statErrors = 0;
  lGroup = 0;
  queueIndexR = 0;
  queueIndexS = 0;
  statGroup = 0;
  for (int ia = 0; ia < 10; ia++)			// wait a few second to give trainee time
  {
    sendLetter(' ');  // empty key serial queue
    while (sendingMorse == true) {}
  }
  keyboard.read();
}

void correct(char letter1)	// keyed in character was correct
{
  digitalWrite(greenPIN, HIGH);
  digitalWrite(redPIN, LOW);

  int h1 = letter1 - firstLetter;
  if (h1 > 0 && charProp[h1] != 0 && plainText == false)
  {
    if (charProp[h1] <= 1 + downProp) charProp[h1] = 1;		// prop never below 1
    else charProp[h1] = charProp[h1] - downProp;
  }
  lcdDispText(3, 0, "Errors");
  lcdDispInt(3, 7, statErrors);
}

void wrong(char letter1, char letter2)	// wrong character keyed in
{
  int h1 = letter1 - firstLetter;
  int h2 = letter2 - firstLetter;

  digitalWrite(greenPIN, LOW);
  digitalWrite(redPIN, HIGH);

  // if one character lost during reception
  int h3 = queueIndexR; //current position
  int h4 = indexAdv(queueIndexR);	// next letter in queue
  if (queue[h4] == hKey)
  {
    h1 = queue[h3] - firstLetter; // character at current position wrong
    h2 = 0;
    queueIndexR = indexAdv(queueIndexR); // correct position
  }

  if (h1 > 0 && charProp[h1] != 0 && plainText == false)
  {
    charProp[h1] = charProp[h1] + upProp;
    if (charProp[h1] >= 100) charProp[h1] = 100;
  }
  if (h2 > 0 && charProp[h1] != 0 && plainText == false)
  {
    charProp[h2] = charProp[h2] + upProp;
    if (charProp[h2] >= 100) charProp[h2] = 100;
  }

  statErrors++;
  lcdDispText(3, 0, "Errors");
  lcdDispInt(3, 7, statErrors);

}

void lcdDispText(int line, int col, char text[])	// display text string on LCD
{
  lcd.setCursor(col, line);
  for (int i = col; i <= col + 15 && i < 19; i++) lcd.print(' '); // Clear display line
  lcd.setCursor(col, line);
  int ii = 0;
  while (text[ii] != '\0')
  {
    lcd.print(text[ii]);
    ii++;
  }
}

void lcdDispInt(int line, int col, int number)	// display integer number on LCD
{
  lcd.setCursor(col, line);
  for (int ii = col; ii <= col + 4 && ii < 19; ii++) lcd.print(' '); // Clear display line
  lcd.setCursor(col, line);
  lcd.print(number);
}


void analyzeSpeed()		// change speed if necessary
{
  if (statErrors > 1)
  {
    speed = speed - speedDec;
    if (speed < 20) speed = 20;
    lcdDispText(0, 15, "down");
    setSpeed(speed);
  }
  if (statErrors == 0)
  {
    if (speed < 200) speed = speed + speedInc;
    lcdDispText(0, 15, "up   ");
    setSpeed(speed);
  }
  statErrors = 0;
}

void sendLetter(char encodeMorseChar)	// translate character in morse code
{
  //	if (sendingMorse=false)
  int i;
  // change to capital letter if not
  if (encodeMorseChar > 96) encodeMorseChar -= space;  // make letter upper case
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
  if (morseSignals > 0)
  {
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
  else
  { // unless it was on the top to begin with (A space character)
    morseSignalString[0] = ' ';
    morseSignalPos = 1;
    morseSignals = 1; // cheating a little; a wordspace for a "morse signal"
  }

  morseSignalString[morseSignalPos] = '\0';

  noInterrupts();
  ex = false;
  stat = 1;
  morseSignalPos = 0;
  sendingMorse = true;
  interrupts();
}

void transmitMorse()		// Interrupt routine to transmit morse code
{
  while ((ex == false) && (sendingMorse == true))
  {
    switch (stat)
    {
      case 1:
        {
          if (digitalRead(morseOutPin) == LOW)  stat = 2;
          else stat = 3;
          break;
        }
      case 2:
        {
          stat = 7;
          if (morseSignalPos == 0) stat = 4;
          if (morseSignalString[morseSignalPos] == '\0') stat = 5;
          if (morseSignalString[0] == ' ') stat = 6;
          break;
        }
      case 3:
        {
          if (morseSignalString[morseSignalPos] == '.') stat = 9;
          else stat = 8;
          break;
        }
      case 4:
        {
          digitalWrite (morseOutPin, HIGH);
          tone(tonePin, 1000);
          stat = 3;
          ex = true;
          tick = 0;
          break;
        }
      case 5:
        {
          if (tick < endTick) stat = 10;
          else stat = 11;
          break;
        }
      case 6:
        {
          if (tick < spaceTick) stat = 12;
          else stat = 13;
          break;
        }
      case 7:
        {
          digitalWrite (morseOutPin, HIGH);
          tone(tonePin, 1000);
          ex = true;
          stat = 3;
          break;
        }
      case 8:
        {
          if (tick < dashTick) stat = 15;
          else stat = 14;
          break;
        }
      case 9:
        {
          digitalWrite (morseOutPin, LOW);
          noTone(tonePin);
          morseSignalPos++;
          stat = 2;
          ex = true;
          break;
        }
      case 10:
        {
          tick++;
          stat = 5;
          ex = true;
          break;
        }
      case 11:
        {
          tick = 0;
          ex = true;
          sendingMorse = false;
          break;
        }
      case 12:
        {
          tick++;
          stat = 6;
          ex = true;
          break;
        }
      case 13:
        {
          tick = 0;
          sendingMorse = false;
          ex = true;
          break;
        }
      case 14:
        {
          digitalWrite (morseOutPin, LOW);
          noTone(tonePin);
          tick = 0;
          morseSignalPos++;
          stat = 1;
          ex = true;
          break;
        }
      case 15:
        {
          tick++;
          stat = 8;
          ex = true;
          break;
        }

      default:
        {
          stat = 1;
        }
    }
  }
  ex = false;
}

void setSpeed(float value)	// adjust speed of morse
{
  double M_tim = TIMER_MULT * 6 / value;
  Timer1.initialize(M_tim);                // Initialise timer 1 with correct time
  lcdDispInt(0, 7, speed);
}

char generateLetter()	//Random generate letter according its propability (higher propability means letter is generated more often)
{
  int ii;
  int hi;
  int tot = 0;

  for  (ii = 0; ii <= lastLetter - firstLetter; ii++) tot = tot + charProp[ii]; // tot = sum of all probabilities

  int sum = 0;
  hi = random(0, tot);
  ii = 0;
  while (sum < hi)
  {
    sum = sum + charProp[ii];
    ii++;
  }
  return char(firstLetter + ii - 1);
}

int indexAdv(int index)	//advance queue index by one
{
  index++;
  if (index == queueLength) index = 0;
  return index;
}

byte readKey() {
  byte c;
  while (!keyboard.available()) {}
  c = keyboard.read();
  return c;
}

int inputSpeed() {
  int _speed = 0;
  byte _num[3];

  for (int _i = 0; _i < 3; _i++) {
    _num[_i] = 254;
  }
  int _i = 0;
  while (!keyboard.available()) {}
  byte _hi = keyboard.read();
  while ( _hi != 13 && _i < 3) {
    _num[_i] = _hi - '0';
    _i++;
    _hi = readKey();
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

void initialize()		// initialize program
{
  for (int ib = 0; ib < queueLength; ib++) 	queue[ib] = ' ';
  plainText = false;


  // LCD
  queueIndexS = 0;
  queueIndexR = 0;
  while (sendingMorse == true) {}
  speed = 0;

  lcd.clear();
  tone(tonePin, 1000);
  digitalWrite(morseOutPin, HIGH);
  delay(100);
  noTone(tonePin);
  digitalWrite(morseOutPin, LOW);

  // Enter Speed
  while (keyboard.read() >= 0) {}
  // flush input buffer
  while (speed < 20 || speed > 200)
  {
    lcdDispText(0, 0, "Speed");

    speed = inputSpeed();
    if  (speed < 20 || speed > 200)
    {
      lcdDispText(0, 0, "Error");
      delay(1000);
    }
  }
  setSpeed(speed);

  // Enter Menu Option

  byte menu = 0;
  do {
    lcdDispText(1, 0, "Menu");
    menu = readKey() - '0';
    if (menu < 0 || menu > 9) {
      lcdDispText(1, 0, "Error");
    }
  } while (menu < 0 || menu > 9) ;


  switch (menu)
  {
    case 0: // Propabilities from last exercise
      {
        lcdDispText(1, 7, "0");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++) 	charProp[ii] = EEPROM.read(ii);		// read propabilities from EEPROM
        break;
      }

    case 1: // All letters even distributed
      {
        lcdDispText(1, 7, "1");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P1 + ii);

        }
        break;
      }

    case 2:  // All letters with clear text propabilities
      {
        lcdDispText(1, 7, "2");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P2 + ii);
        }
        plainText = true;		// plain text probabilities
        break;
      }

    case 3: //Numbers
      {
        lcdDispText(1, 7, "3");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P3 + ii);
        }
        break;
      }


    case 4: // Letters and numbers evenly distributed
      {
        lcdDispText(1, 7, "4");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P4 + ii);
        }
        break;

      }
    case 5:
      {
        lcdDispText(1, 7, "5");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P5 + ii);
        }
        break;
      }


    case 6: // Beginners1
      {
        lcdDispText(1, 7, "6");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P6 + ii);
        }
        break;

      }
    case 7: // Beginners2
      {
        lcdDispText(1, 7, "7");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P7 + ii);
        }
        break;
      }

    case 8: // Beginners3
      {
        lcdDispText(1, 7, "8");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P8 + ii);
        }
        break;
      }

    case 9: // Beginners4
      {
        lcdDispText(1, 7, "9");
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++)
        {
          charProp[ii] = pgm_read_byte_near(P9 + ii);
        }
        break;
      }

    default:
      {
        for (int ii = 0; ii <= (lastLetter - firstLetter); ii++) charProp[ii] = P1[ii];
      }

  }


  lcdDispText(1, 0, "         ");
  while (keyboard.read() >= 0) {}		// flush input buffer
  Serial.println("Read");
  for (int ii = firstLetter; ii <= lastLetter; ii++)
  {
    int hi = ii - firstLetter;
    if (charProp[hi] > 0) {
      Serial.print(char(ii));
      Serial.print(" prop ");
      Serial.println(charProp[hi]);
    }
  }
}
