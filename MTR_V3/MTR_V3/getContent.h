const String CWchars = "abcdefghijklmnopqrstuvwxyz0123456789.,:-/=?@+SANKVäöüH";
const byte pool[][2]  = {
  // letters
  {B01000000, 2},  // a    0
  {B10000000, 4},  // b
  {B10100000, 4},  // c
  {B10000000, 3},  // d
  {B00000000, 1},  // e
  {B00100000, 4},  // f
  {B11000000, 3},  // g
  {B00000000, 4},  // h
  {B00000000, 2},  // i
  {B01110000, 4},  // j
  {B10100000, 3},  // k
  {B01000000, 4},  // l
  {B11000000, 2},  // m
  {B10000000, 2},  // n
  {B11100000, 3},  // o
  {B01100000, 4},  // p
  {B11010000, 4},  // q
  {B01000000, 3},  // r
  {B00000000, 3},  // s
  {B10000000, 1},  // t
  {B00100000, 3},  // u
  {B00010000, 4},  // v
  {B01100000, 3},  // w
  {B10010000, 4},  // x
  {B10110000, 4},  // y
  {B11000000, 4},  // z  25
  // numbers
  {B11111000, 5},  // 0  26
  {B01111000, 5},  // 1
  {B00111000, 5},  // 2
  {B00011000, 5},  // 3
  {B00001000, 5},  // 4
  {B00000000, 5},  // 5
  {B10000000, 5},  // 6
  {B11000000, 5},  // 7
  {B11100000, 5},  // 8
  {B11110000, 5},  // 9  35
  // interpunct   . , : - / = ? @ +    010101 110011 111000 100001 10010 10001 001100 011010 01010
  {B01010100, 6},  // .  36
  {B11001100, 6},  // ,  37
  {B11100000, 6},  // :  38
  {B10000100, 6},  // -  39
  {B10010000, 5},  // /  40
  {B10001000, 5},  // =  41
  {B00110000, 6},  // ?  42
  {B01101000, 6},  // @  43
  {B01010000, 5},  // +  44    (at the same time <ar> !)
  // Pro signs  <>  <as> <ka> <kn> <sk>
  {B01000000, 5},  // <as> 45 S
  {B10101000, 5},  // <ka> 46 A
  {B10110000, 5},  // <kn> 47 N
  {B00010100, 6},   // <sk> 48    K
  {B00010000, 5},  // <ve> 49 E
  // German characters
  {B01010000, 4},  // ä    50
  {B11100000, 4},  // ö    51
  {B00110000, 4},  // ü    52
  {B11110000, 4}   // ch   53  H
};


String getrandomCall(int maxLength) {            // random call-sign like pattern, maxLength = 3 - 6, 0 returns any length
  const byte prefixType[] = {1, 0, 1, 2, 3, 1};    // 0 = a, 1 = aa, 2 = a9, 3 = 9a
  byte prefix;
  String call = "";
  unsigned int l = 0;
  //int s, e;

  if (maxLength == 1 || maxLength == 2)
    maxLength = 3;
  if (maxLength > 6)
    maxLength = 6;

  if (maxLength == 3)
    prefix = 0;
  else
    prefix = prefixType[random(0, 6)];          // what type of prefix?
  switch (prefix) {
    case 1: call += CWchars.charAt(random(0, 26));
      ++l;
    case 0: call += CWchars.charAt(random(0, 26));
      ++l;
      break;
    case 2: call += CWchars.charAt(random(0, 26));
      call += CWchars.charAt(random(26, 36));
      l = 2;
      break;
    case 3: call += CWchars.charAt(random(26, 36));
      call += CWchars.charAt(random(0, 26));
      l = 2;
      break;
  } // we have a prefix by now; l is its length
  // now generate a number
  call += CWchars.charAt(random(26, 36));
  ++l;
  // generate a suffix, 1 2 or 3 chars long - we re-use prefix for the suffix length
  if (maxLength == 3)
    prefix = 1;
  else if (maxLength == 0) {
    prefix = random(1, 4);
    prefix = (prefix == 2 ? prefix :  random(1, 4)); // increase the likelihood for suffixes of length 2
  }
  else {
    prefix = _min(maxLength - l, 3);                 // we try to always give the desired length, but never more than 3 suffix chars
  }
  while (prefix--) {
    call += CWchars.charAt(random(0, 26));
    ++l;
  } // now we have the suffix as well
  // are we /p or /m? - we do this only in rare cases - 1 out of 9, and only when maxLength = 0, or maxLength-l >= 2
  if (maxLength == 0 ) //|| maxLength - l >= 2)
    if (! random(0, 8)) {
      call += "/";
      call += ( random(0, 2) ? "m" : "p" );
    }
  // we have a complete call sign!
  call.toUpperCase();
  return call;
}


/////// generate CW representations from its input string
/////// CWchars = "abcdefghijklmnopqrstuvwxyz0123456789.,:-/=?@+SANKVäöüH";

String generateCWword(String symbols) {
  int pointer;
  byte bitMask, NoE;
  //byte nextElement[8];      // the list of elements; 0 = dit, 1 = dah
  String result = "";

  int l = symbols.length();

  for (int i = 0; i < l; ++i) {
    char c = symbols.charAt(i);                                 // next char in string
    pointer = CWchars.indexOf(c);                               // at which position is the character in CWchars?
    NoE = pool[pointer][1];                                     // how many elements in this morse code symbol?
    bitMask = pool[pointer][0];                                 // bitMask indicates which of the elements are dots and which are dashes
    for (int j = 0; j < NoE; ++j) {
      result += (bitMask & B10000000 ? "2" : "1" );         // get MSB and store it in string - 2 is dah, 1 is dit, 0 = inter-element space
      bitMask = bitMask << 1;                               // shift bitmask 1 bit to the left
      //DEBUG("Bitmask: ");
      //DEBUG(String(bitmask, BIN));
    } /// now we are at the end of one character, therefore we add enough space for inter-character
    result += "0";
  }     /// now we are at the end of the word, therefore we remove the final 0!
  result.remove(result.length() - 1);
  return result;
}

String getRandomWord( int maxLength) {        //// give me a random English word, max maxLength chars long (1-5) - 0 returns any length
  if (maxLength > 6)
    maxLength = 0;
	int rn = random(EnglishWords::WORDS_POINTER[maxLength], EnglishWords::WORDS_NUMBER_OF_ELEMENTS);
  String ww = EnglishWords::words[random(EnglishWords::WORDS_POINTER[maxLength], EnglishWords::WORDS_NUMBER_OF_ELEMENTS)];
  ww.toUpperCase();
  return ww;
}

String getRandomAbbrev( int maxLength) {        //// give me a random CW abbreviation , max maxLength chars long (1-5) - 0 returns any length
  if (maxLength > 6)
    maxLength = 0;
    String ww =Abbrev::abbreviations[random(Abbrev::ABBREV_POINTER[maxLength], Abbrev::ABBREV_NUMBER_OF_ELEMENTS)];
    ww.toUpperCase();
  return ww;
}
