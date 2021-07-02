// SkeeBall 3000 by LFPoulain 
// Chaine youtube : https://youtube.com/c/lesfrerespoulain
// Inspiré de https://github.com/ccgthree/skeeduino

#include <ESP32Servo.h>
#include "DFRobotDFPlayerMini.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>


//StripLEDCoude

#define PIN 18
#define NUM_LEDS 24
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

//StripLEDCible

#define PINCIBLE 4
#define NUM_LEDS_CIBLE 8
Adafruit_NeoPixel strip_cible = Adafruit_NeoPixel(NUM_LEDS_CIBLE, PINCIBLE, NEO_GRB + NEO_KHZ800);

//MatriceLED

#define PINMATRIX 15
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 16, PINMATRIX,
                            NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
                            NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255)
};


// Variable de défilement
 
int x    = matrix.width();
int pass = 0;

// On nomme le servo de retenu des balles.
Servo ballReturn;

// Ouverture et fermeture MAX du servo
int servopen = 0;
int servclose = 100;

//ESP32 port serie hardware pour communication DFPLayer

HardwareSerial dfplayer(1); 

 // Declaration lib DFPlayer sous le nom playermp3

DFRobotDFPlayerMini playermp3; // Declaration DFPlayer sous le nom playermp3

// Pin declarations
int sensors[6] = {32, 33, 25, 26, 27, 14}; // Array des interrupteurs
int resetButton = 21; // Bouton Start/reset
int servPin = 5; // Gpio Servo

// General declarations
int score = 0;
int balls = 0;

int highscore;         // the current state of the output pin
int resetState;
int loopled = 0;
int newgamesound = 0;
int highscoresound = 0;
int boot = 1;

// Buffer Declarations
char ballTemp[10];
char scoreTemp[10];
char highscoreTemp[10];

// Debounce delays
int sensDelay = 500;
int resetDelay = 500;

void setup() {

  // Decided to try using a for loop to designate each input pin as an INPUT. Works.
  for (int eachSensor = 0; eachSensor <= 5; eachSensor++) {
    pinMode(sensors[eachSensor], INPUT_PULLUP);
  }

  pinMode(resetButton, INPUT_PULLUP);

  //LIB ServoESP32

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  ballReturn.setPeriodHertz(50);
  ballReturn.attach(servPin, 500, 1500);

  // Retour moniteur serie

  delay(1000);
  Serial.begin(9600);

  // Initialisation DFPlayer

  dfplayer.begin(9600, SERIAL_8N1, 16, 17);  // speed, type, RX, TX;
  if (!playermp3.begin(dfplayer)) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    while (true) {
      delay(0); // Code to compatible with ESP8266 watch dog.
    }
  }
  Serial.println(F("DFPlayer Mini online."));

  // Lecture du son START

  playermp3.volume(30);  //Set volume value. From 0 to 30
  playermp3.playMp3Folder(1);  //play specific mp3 in SD:/01/001.mp3

  // Initialisation MatriceLED

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(colors[0]);
  resetDisplays(); // EmptyScreen

  // Initialisation Strip LED Coude

  strip.begin();
  strip.show();

  // Initialisation Strip LED Cible

  strip_cible.setBrightness(255);
  strip_cible.begin();
  strip_cible.fill(strip.Color(255, 100, 255), 0, 8);
  strip_cible.show(); // Initialize all pixels to 'off'

}

void loop() {

  //Affichage au premier demarrage
  if (boot == 1) skeeballDisplays();
  
  smartcolorWipe();

  while (balls > 0) {

    smartcolorWipe();
    scoreDisplays();
    newgamesound = 1;

    for (int s = 0; s <= 5; s++) 
    {  // Check sensor array.
      if (!digitalRead(sensors[s])) 
      {
        switch (s)
        {
          case 0 : play<100>() break;
          case 1 : play<50>() break;
          case 2 : play<40>() break;
          case 3 : play<30>() break;
          case 4 : play<20>() break;
          case 5 : play<10>() break;
        
          default: 
          break;
        }

      }//end of while loop

    }
    startOver();
  }

  if (newgamesound == 1) {
    playermp3.playMp3Folder(202);
    newgamesound = 0;
  }

  startOver();
  highscoreDisplays();

}
inline template<int _n > void play(void)
{
  pointsDisplays<_n>();
  ledPoints();
  playpoints(_n);  //play specific mp3
  addpoints<_n>();
}

template<int _n > void addpoints() 
{
  score = score + _n;

  highscore = highscore < score ? score : highscore;

  balls = balls - 1;

  scoreDisplays();

  Serial.print("Your score is: ");
  Serial.println(score);
  Serial.print("Highscore is: ");
  Serial.println(highscore);
  Serial.print("Balls Remaining: ");
  Serial.println(balls);
  delay(sensDelay);
}

void playpoints(int _n) 
{
  playermp3.playMp3Folder(_n);
}

void reset() 
{
  returnControl(servopen);
  playermp3.playMp3Folder(201);
  score = 0;
  balls = 6;
  boot = 0;
  newgameDisplays();
  Serial.print("Balls Remaining: ");
  Serial.println(balls);
  delay(4000);
  returnControl(servclose);
}

void startOver() {
  resetState = digitalRead(resetButton);
  if (resetState == LOW) {
    reset();
    delay(resetDelay);
  }
}

void returnControl(int x) 
{
  ballReturn.write(x);
}

void newgameDisplays() 
{
  matrix.fillScreen(0);
  matrix.fillScreen(matrix.Color(100, 0, 100));
  matrix.setTextColor(matrix.Color(0, 0, 0));
  matrix.setCursor(8, 1);
  matrix.print("NEW");
  matrix.setTextColor(matrix.Color(0, 0, 0));
  matrix.setCursor(6, 10);
  matrix.print("GAME");
  matrix.setTextColor(matrix.Color(255, 255, 255));
  matrix.setCursor(7, 0);
  matrix.print("NEW");
  matrix.setTextColor(matrix.Color(255, 255, 255));
  matrix.setCursor(5, 9);
  matrix.print("GAME");
  matrix.show();
}

void scoreDisplays() 
{
  matrix.fillScreen(0);
  matrix.setTextColor(colors[0]);
  matrix.setCursor(2, 0);
  matrix.print("SCORE");
  matrix.setTextColor(colors[2]);
  if (score >= 100) matrix.setCursor(7, 9);
  if (score < 100 ) matrix.setCursor(10, 9);
  if (score == 0) matrix.setCursor(13, 9);
  matrix.print(score);
  matrix.show();
}

template<int _n>void pointsDisplays(void) 
{
  matrix.fillScreen(0);
  matrix.setTextColor(colors[0]);
  matrix.setCursor(2, 9);
  matrix.print("POINTS");
  matrix.setTextColor(colors[2]);
  if (_n >= 100) matrix.setCursor(7, 0);
  if (_n < 100) matrix.setCursor(10, 0);
  matrix.print(_n);
  matrix.show();
  matrix.fillScreen(0);
  delay(300);
  matrix.setTextColor(colors[1]);
  if (_n >= 100) matrix.setCursor(7, 0);
  if (_n < 100) matrix.setCursor(10, 0);
  matrix.print(_n);
  matrix.show();
  delay(400);
}

void resetDisplays() 
{
  matrix.fillScreen(0);
  matrix.show();
}

void highscoreDisplays() 
{
  if (boot == 0) {
    static unsigned long wait_highscoreDisplays = 50;
    static unsigned long lastUpdate_highscoreDisplays = 0;
    unsigned long now_highscoreDisplays = millis();
    matrix.fillScreen(matrix.Color(0, 100, 100));
    matrix.setTextColor(colors[3]);
    matrix.setCursor(x + 1, 1);
    matrix.print((String)"SCORE:" + score);
    matrix.setTextColor(colors[1]);
    matrix.setCursor(x, 0);
    matrix.print((String)"SCORE:" + score);
    matrix.setTextColor(colors[3]);
    matrix.setCursor(x + 1, 10);
    matrix.print((String)"HIGHSCORE:" + highscore);
    matrix.setTextColor(colors[0]);
    matrix.setCursor(x, 9);
    matrix.print((String)"HIGHSCORE:" + highscore);
    if (now_highscoreDisplays > (lastUpdate_highscoreDisplays + wait_highscoreDisplays)) {
      x--;
      lastUpdate_highscoreDisplays = now_highscoreDisplays;
      if (x <= -86) x = 46;
    }
    matrix.show();
  }
}

void skeeballDisplays() 
{
  static unsigned long wait_highscoreDisplays = 50;
  static unsigned long lastUpdate_highscoreDisplays = 0;
  unsigned long now_highscoreDisplays = millis();
  matrix.fillScreen(matrix.Color(100, 0, 100));
  matrix.setTextColor(matrix.Color(0, 0, 0));
  matrix.setCursor(x + 1, 5);
  matrix.print(F("GALACTIC TRAVELER"));
  matrix.setTextColor(matrix.Color(255, 255, 255));
  matrix.setCursor(x, 4);
  matrix.print(F("GALACTIC TRAVELER"));
  if (now_highscoreDisplays > (lastUpdate_highscoreDisplays + wait_highscoreDisplays)) {
    x--;
    lastUpdate_highscoreDisplays = now_highscoreDisplays;
    if (x <= -103) x = 36;
  }
  matrix.show();
}

void ledPoints() 
{
  ciblerouge();
  theaterChase(0xff, 0, 0, 20);
  cibleturquoise();
}

void ciblerouge() 
{

    strip_cible.setBrightness(255);
    strip_cible.begin();
    strip_cible.fill(strip.Color(255, 0, 0), 0, 8);
    strip_cible.show(); // Initialize all pixels to 'off'

}

void cibleturquoise() 
{

    strip_cible.setBrightness(255);
    strip_cible.begin();
    strip_cible.fill(strip.Color(255, 100, 255), 0, 8);
    strip_cible.show(); // Initialize all pixels to 'off'

}


void theaterChase(byte red, byte green, byte blue, int SpeedDelay) {
  for (int j = 0; j < 5; j++) { //do 10 cycles of chasing
    for (int q = 0; q < 3; q++) {
      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        setPixel(i + q, red, green, blue);  //turn every third pixel on
      }
      showStrip();

      delay(SpeedDelay);

      for (int i = 0; i < NUM_LEDS; i = i + 3) {
        setPixel(i + q, 0, 0, 0);    //turn every third pixel off
      }
    }
  }
}

// Petit bout de code très tardif pour rempalcer le delay() par millis()
// La boucle de se pause plus pour l'effet des leds ! Merci Nico13s

void smartcolorWipe() {
  static unsigned long wait = 20;
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  static uint16_t i = NUM_LEDS;

  byte Ba = 0x00;
  byte B2 = 0xff;
  byte R1 = 0x9F;
  byte R2 = 0xC7;
  byte G1 = 0x00;
  byte G2 = 0x00;
  static byte B = Ba;
  static byte R = R1;
  static byte G = G1;


  if (now > (lastUpdate + wait)) {
    i--;
    setPixel(i, B, R, G);
    showStrip();
    if (i == 0) {
      i = NUM_LEDS;
      if (B == Ba) {
        B = B2;
        R = R2;
        G = G2;
      }
      else {
        B = Ba;
        R = R1;
        G = G1;
      }
    }
    lastUpdate = now;
  }
}

// Fonctions FastLed/Neopixel universelles de https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/

void showStrip() {
  strip.show();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < NUM_LEDS; i++ ) {
    setPixel(i, red, green, blue);
  }
  showStrip();
}
