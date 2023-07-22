#include <Adafruit_NeoPixel.h>

#define TILT_BACK_FRONT_PIN 2
#define TILT_LEFT_RIGHT_PIN 3

#define LED_PIN    13

#define LED_COUNT  12

#define LOGGING_ENABLED false

// N dimensional matrix. Each dimension corresponse to the off/on status of each tilt sensor.
const int LED_RANGE_START_LOOKUP [2][2] = {
  {0, 1},
  {3, 2}
};

// Division can be expensive, and this can be calculated once on init.
const int LED_QUADRANT_SIZE = LED_COUNT / 4;
const int MAIN_COLOUR [3] =  {0, 0, 255};
const int SECONDARY_COLOUR [3] = {255, 65, 0};

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup()
{  
  enableTiltSensorInterupt(TILT_BACK_FRONT_PIN);
  enableTiltSensorInterupt(TILT_LEFT_RIGHT_PIN);

  Serial.begin(9600);
  
  strip.begin();
  readTiltAndUpdateLED();
}

void enableTiltSensorInterupt(int pin) {
  pinMode(pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(pin), readTiltAndUpdateLED, CHANGE);
}

// Only interrupts are used, loop is not.
void loop() {}

void readTiltAndUpdateLED() {
  bool isTiltingForwards = read(TILT_BACK_FRONT_PIN, "Is tilting forwards");
  bool isTiltingLeft = read(TILT_LEFT_RIGHT_PIN, "Is tilting left");
  int range_start = LED_RANGE_START_LOOKUP[isTiltingForwards][isTiltingLeft] * LED_QUADRANT_SIZE;
  
  changeColorForRangeStartingAt(range_start, 255, 0, 255);
} 

bool read(int pin, String description) {
  bool value = digitalRead(pin);
  if (LOGGING_ENABLED) {
    Serial.print(description);
    Serial.print(": ");
    Serial.println(value);
  }
  return value;
}

void changeColorForRangeStartingAt(int start, int red, int green, int blue) {
  strip.clear();
  for( int i = 0; i < LED_COUNT; i++) {
    setPixelColour(i, SECONDARY_COLOUR[0], SECONDARY_COLOUR[1], SECONDARY_COLOUR[2]);
  }
  for( int i = start; i < start + 3; i++) {
    setPixelColour(i, MAIN_COLOUR[0], MAIN_COLOUR[1], MAIN_COLOUR[2]);
  }
  strip.show();
}

void setPixelColour(int n, int red, int green, int blue) {
  if (LOGGING_ENABLED) {
    Serial.print("Setting element [");
    Serial.print(n);
    Serial.print("] to RGB value [");
    Serial.print(red);     
    Serial.print(", ");
    Serial.print(green);     
    Serial.print(", ");
    Serial.print(blue);     
    Serial.println("].");
  }
 
  strip.setPixelColor(n, red, green, blue);
}

// References
// https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
// https://docs.arduino.cc/tutorials/generic/tilt-sensor