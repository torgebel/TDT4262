#include <Adafruit_NeoPixel.h>

//state machine
#define NUM_LDR 3
#define NUM_AVG_SAMPLES 10


#define ONTHR 30  //very dependent on the lighting conditions in the room. 

#define TIMEOUT_MS 1000

//ultrasound
// define pin numbers
#define TRIG_PIN 10
#define ECHO_PIN 9
#define VIBRATE_PIN 3

#define COM 7
 
// defines variables
long duration;
int distance;

int newEvent = 0;
int oldEvent = 0;
int event = 0;
int state = 0;

long int startTimer = 0;

bool timerEnabled = false;


//made the events addable. A pertaines to ldr[0], B to ldr[2] etc. AB means that both A and B are covered wich can happen if the hand hoveres close to one of the ldr's.
enum events {
  A = 1,
  B = 2,
  C = 4,
  AB = 3,
  AC = 5,
  BC = 6,
  ABC = 7,
  TIMEOUT = 0
};

enum states {
  noMusic = 0,
  music = 1,
  waiting = 2 //the waiting state is implemented because we don't want the glass to start playing the moment one sensor senses your hand, we want to encourage the right motion, so onece it registers one sensor, it waits for another sensor to register touch before playing music.
};

int sensValue;

int ldr[NUM_LDR];


// Declare and initialise global GPIO pin constant for Neopixel ring
const byte neoPin = 11;

// Declare and initialise global constant for number of pixels
const byte neoPixels = 24; //on the small glasses there are only 4 leds. 

// Declare and initialise variable for Neopixel brightness
byte neoBright = 100;

// Create new Neopixel ring object
Adafruit_NeoPixel ring = Adafruit_NeoPixel(neoPixels, neoPin, NEO_GRB);

//matrix med alle fargene
int strip[7][3] = {
  {255, 0, 0}, //red
  {255, 127, 0}, //orange
  {255, 255, 0}, //yellow
  {0, 255, 0}, //green
  {0, 0, 255}, //blue
  {75, 0, 130}, //indigo
  {148, 0, 211}, //violet
};




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  ring.begin();
  ring.setBrightness(neoBright);
  ring.show(); // Initialize all pixels to 'off'
  pinMode(TRIG_PIN, OUTPUT); // Sets the trigPin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the echoPin as an Input
  pinMode(VIBRATE_PIN, OUTPUT); // Sets the vibrate pin as an Output
  pinMode(COM, OUTPUT); //sets the communicate pin to output. 
}


void loop() {
  // Clears the trigPin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  // Calculating the distance
  distance = (duration * 0.034) / 2;

  //maps the distance to an index of a row in the strips matrix
  int colour = map(distance, 7, 17, 0, 6);



  //takes an average of 10 samples from the light sensors.
  for (int i = 0; i < NUM_LDR; i++) {
    for (int j = 0; j < NUM_AVG_SAMPLES; j++) {
      ldr[i] += analogRead(i);
      ldr[i] = ldr[i] / 2;
    }
  }
  /*Serial.print("LDR:");
    Serial.print(ldr[0], DEC);
    Serial.print("\t");
    Serial.print(ldr[1], DEC);
    Serial.print("\t");
    Serial.print(ldr[2], DEC);
    Serial.print("\n");*/ //used this to calibrte the ONTHR constant.


  newEvent = 0;
  if (ldr[0] < ONTHR) {
    newEvent += A;
  }
  if (ldr[1] < ONTHR) {
    newEvent += B;
  }
  if (ldr[2] < ONTHR) {
    newEvent += C;
  }
  if ((newEvent != event) && (newEvent != 0)) {
    event = newEvent;
    startTimer = millis();
    //Serial.print("startTimer: ");
    //Serial.println(startTimer)
    timerEnabled = true;
    Serial.print("Going to new event: ");
    Serial.println(event);
  }
  else if (((millis() - startTimer) > TIMEOUT_MS) && timerEnabled) {
    //Serial.print("millis");
    //Serial.println(millis());
    event = TIMEOUT;
    timerEnabled = false;
  }
  else {
    event = oldEvent;
  }
  switch (state) {
    case noMusic:

      Serial.println("No Music: Turning off music");
      digitalWrite(VIBRATE_PIN, LOW);
      digitalWrite(COM, LOW);
      for (int i = 0; i < neoPixels; i++) { // turning off light;
        ring.setPixelColor(i, ring.Color(0, 0, 0));
      }
      ring.show();
      if ((event >= 1) && (event <= 6)) { //event: A, B, C, AB, AC, BC
        state = waiting;
        Serial.print("No Music: going to Waiting, event: ");
        Serial.println(event);
      }
      break;
    case music:
      if (event != oldEvent) {
        Serial.println("Music");
        digitalWrite(VIBRATE_PIN, HIGH);
        digitalWrite(COM, HIGH);
        for (int i = 0; i < neoPixels; i++) {
          ring.setPixelColor(i, ring.Color(strip[colour][0], strip[colour][1], strip[colour][2]));
        }
        ring.show();
      }
      if (event == TIMEOUT) {
        state = noMusic;
        Serial.println("Music: going to noMusic, event: TIMEOUT");
      }
      if (event == ABC) {
        state = noMusic;
        Serial.println("Music: going to noMusic, event: ABC");
      }
      break;
    case waiting:
      if (event != oldEvent) {
        Serial.println("Waiting");
      }
      if ((event >= 1) && (event <= 6) && (oldEvent != event)) { //event: A, B, C, AB, AC, BC
        state = music;
        Serial.print("Waiting: going to Music, event:");
        Serial.println(event);
      }
      else if ((event == TIMEOUT)) { // we implemented a timeout that would determine how long you could go between sensors before the motion was no longer valid, and the glass would stopp playing. 
        state = noMusic;
        Serial.println("Waiting: going to noMusic, event: TIMEOUT");
        digitalWrite(VIBRATE_PIN, LOW);
        digitalWrite(COM, LOW);
        for (int i = 0; i < neoPixels; i++) {
          ring.setPixelColor(i, ring.Color(0, 0, 0));
        }
        ring.show();
        
      }
      else if (event == ABC) { //if all sensors were covered at the same time, the glass should stop playing because the hand would mute the vibrations that make the sound.
        state = noMusic;
        Serial.println("Waiting: going to noMusic, event: ABC");
        Serial.println("Waiting: going to noMusic, event: TIMEOUT");
        digitalWrite(VIBRATE_PIN, LOW);
        digitalWrite(COM, LOW);
        for (int i = 0; i < neoPixels; i++) {
          ring.setPixelColor(i, ring.Color(0, 0, 0));
        }
        ring.show();
      }
      break;
  }
  if (event != 0) {
    oldEvent = event;
  }
}
