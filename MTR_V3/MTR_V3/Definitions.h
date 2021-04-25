#define MINI

#ifdef BIGBOARD
// PIN Assignment D1 Mini
#define DEBUGPIN0 04
#define GREEN_LED 05
#define MORSEOUTPUT 24 // digital Morse signal
#define TONEPIN 25  // audio Morse signal
#define KEYBOARD_CLK 18   // PS/2
#define KEYBOARD_DATA 19  // PS/2
#define GPIO21 21    // SDA
#define GPIO22 22    // SCL
#define RED_LED 23
#define DEBUGPIN1 16      
#define DEBUGPIN2 17   
#define DEBUGPIN3 27
#define DEBUGPIN4 32
#define DEBUGPIN5 32
#define GPIO33 33
#endif

#ifdef MINI
// PIN Assignment big board
#define DEBUGPIN0 04
#define GREEN_LED 05
#define MORSEOUTPUT 16 // digital Morse signal
#define TONEPIN 17  // audio Morse signal
#define KEYBOARD_CLK 18   // PS/2
#define KEYBOARD_DATA 19  // PS/2
#define GPIO21 21    // SDA
#define GPIO22 22    // SCL
#define RED_LED 23
#define DEBUGPIN1 25      
#define DEBUGPIN2 26   
#define DEBUGPIN3 27
#define DEBUGPIN4 32
#define DEBUGPIN5 33
#endif


#define QUEUELENGTH 30      // QUEUELENGTH
#define FIRSTLETTER 33
#define LASTLETTER 90     // asc(Last letter) used
#define LETTER_SPACE 32   // space
#define STATLENGTH 10     // length till speed adaption
#define PROP_UP 10       // increase of prop for wrong letters
#define PROP_DOWN -2        // reduction of prop for correct letters
#define SPEED_UP 2        // faster speed if zero errors
#define SPEED_DOWN -4        // reduced speed due to errors

#define MINPROP 1
#define MAXPROP 99

#define DASHTICK 2        // lenght of dash (countet from 0)
#define ENDTICK 1
#define SPACETICK 4

#define TONEOFF 0
#define TONEON 127

#define EEPROM_SIZE LASTLETTER-FIRSTLETTER

unsigned long lastEntryMorse;

enum ledColor {
  LEDoff,
  LEDgreen,
  LEDred
};

enum machine {
  waiting,
  correct,
  incorrect,
  feedback,
  waitForSpace,
  contLost,
  adjSpeed,
  endTraining
};

enum displayDef {
    NONE,
    OLED,
    LCD
} displayType;


enum sourceDef {
  randomized,
  callSigns,
  english,
  abbreviations
} source;

enum trainerStatDef {
    transmitterStopped,
    initialize,
    training,
    lost,
    trainingEnd
} trainerStatus;

struct letterSentDef {
    char letter;
    unsigned long sendTime;
} letterSent;

byte machineStat, machineStatOld;
int groupLength;

byte charProp[LASTLETTER - FIRSTLETTER + 1];  // Each letter has his own propability. This value is adapted to capabilities of trainee during training session
int charPropTransfer[LASTLETTER - FIRSTLETTER + 1]; // combination of prop and character for sorting
int reactionTime[LASTLETTER - FIRSTLETTER + 1]; // last reaction time
int morseSignals;           // nr of morse signals to send in one morse character
int speed = 0;              // speed of morse transmission in bpm
int averageReactionTime;

float ratio;

byte menu = 0;
unsigned long startTime;
String JSONmessage;
int trainingDuration;

int lettersInGroup = 0;  // number of letters sent without space
int lettersSent = 0;     // since last analysis
int lettersError = 0;    // since last analysis
int lettersCorrect = 0;  // since last analysis




//char* myStrings[] = {"This is string 1", "This is string 2", "This is string 3",
//                     "This is string 4", "This is string 5", "This is string 6"
//                    };

/* Different propabilities of characters
  P1: Letters equal propability
  P2: Letters (German propability)
  P3: Numbers
  P4: punctuation marks
  P5: Letters and punctuation marks
  P6: Characters and numbers
  P7: Beginner 2 (A,E,G,I,K,M,N,O,R,T)
  P8: Beginner 3: (A,D,E,G,I,K,M,N,O,P,R,T,U,W,Z)
  P9: Beginner 4: (A,B,C,D,E,F,G,H,I,J,K,M,N,O,P,R,T,U,W,Z)

*/

// Morse code binary tree table (or, dichotomic search table)
// ITU with punctuation (but without non-english characters)
const int morseTreetop = 63;
const char morseTable[] PROGMEM = "*5*H*4*S***V*3*I***F***U?*_**2*E*&*L\"**R*+.****A***P@**W***J'1* *6-B*=*D*/"
                                  "*X***N***C;(!K***Y***T*7*Z**,G***Q***M:8*!***O*9***0*";
const int morseTableLength = (morseTreetop * 2) + 1;
const int morseTreeLevels = log(morseTreetop + 1) / log(2);

const byte P1_LETTERS[]         PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}; // Letters even
const byte P2_CLEARTEXT[]       PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 30, 11, 17, 27, 90, 8, 16, 27, 43, 2, 7, 19, 14, 56, 12, 4, 0, 37, 34, 31, 21, 5, 10, 1, 1, 7};  // Letters English
const byte P3_NUMBERS[]         PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Numbers
const byte P4_MARKS[]           PROGMEM = {0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 0, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  //  punctuations
const byte P5_LETTERS_NUMBERS[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}; // Letters & Numbers
const byte P6[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50}; // Call signs
const byte P7[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 0, 50, 0, 50, 0, 50, 0, 50, 0, 50, 50, 50, 0, 0, 50, 0, 50, 0, 0, 0, 0, 0, 0};  //Beginner 1
const byte P8[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0, 0, 50, 50, 0, 50, 0, 50, 0, 50, 0, 50, 50, 50, 50, 0, 50, 0, 50, 50, 0, 50, 0, 0, 50};  // Beginners 2
const byte P9[] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 50, 0, 50, 50, 50, 50, 0, 50, 0, 50, 50, 0, 50, 0, 0, 50};  // beginers4

// Queues
static QueueHandle_t morseQueue;
static int morse_queue_len = 10;

static QueueHandle_t receiverQueue;
static int receiver_queue_len = 10;

static SemaphoreHandle_t OLEDMutex;
static SemaphoreHandle_t KEYBOARDMutex;
static SemaphoreHandle_t changeSharedData;
