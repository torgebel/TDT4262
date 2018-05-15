#include <MozziGuts.h>            // importing Mozzi library
#include <Oscil.h>                // oscillator template
#include <EventDelay.h>           // delay for ultrasonic sensor
#include <ControlDelay.h>         // delay controlling
#include <RollingAverage.h>       // smoothing of input values
#include <tables/sin2048_int8.h>  // sine table for oscillator

// ultrasonic sensor pins
int trigPin = 2;
int echoPin = 4;

// input pin from linked arduino
int input_pin = 7;

// number of echo cells
unsigned int echo_cells_1 = 32;

// number of times to run updateControl each second
#define CONTROL_RATE 64

// setting echo delay 2 seconds
ControlDelay <128, int> kDelay;

// importing oscilation tables for sine curve
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin1(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin2(SIN2048_DATA);
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin3(SIN2048_DATA);

// number of values to average
RollingAverage <int, 32> kAverage;
int averaged;

byte volume;

EventDelay Delay;

void setup() {
  kDelay.set(echo_cells_1);   // linking delay to echo cells
  startMozzi();               // initializing mozzi
  
  // Ultrasonic Sensor setup
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);    
  Delay.set(10);
}

void updateControl() {
  // Ultrasonic Sensor
  int duration, distance, modFreq;
  digitalWrite(trigPin, LOW);
  Delay.start();
  digitalWrite(trigPin, HIGH);
  Delay.start();
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration * 0.034) / 2;

  // mapping Ultrasonic to low frequency
  modFreq = constrain(map(distance, 7, 17, 262, 494), 262, 494);
  // mapping Ultrasonic to high frequency
  // modFreq = constrain(map(distance, 5, 15, 523, 988), 523, 988);

  averaged = kAverage.next(modFreq); // average of mapped input

  // assigning frequency to echo
  aSin1.setFreq(averaged);
  aSin2.setFreq(kDelay.next(averaged));
  aSin3.setFreq(kDelay.read(echo_cells_1));

  // turn on if signal from input arduino
  static int previous;
  int current = digitalRead(input_pin);
  if (previous == LOW && current == HIGH) {
    volume = 255;
  } else if (previous == HIGH && current == LOW) {
    volume = 0;
  }
  previous = current;

}

int updateAudio() {
  // layering three sine curves
  long asig = (long)
              aSin1.next() * volume +
              aSin2.next() * volume +
              aSin3.next() * volume;
  asig >>= 3;
  return (int) (asig) >> 8; // returning to range after multiplying with 8 bit value
}

void loop() {
  audioHook(); // required in loop
}
