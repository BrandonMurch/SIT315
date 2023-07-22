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
  pinMode(TILT_BACK_FRONT_PIN, INPUT);
  pinMode(TILT_LEFT_RIGHT_PIN, INPUT);
  
  Serial.begin(9600);
  
  strip.begin();
  // Turn on LEDs with starting state.
  measureTiltAndUpdateLED();
}

void loop() {
  measureTiltAndUpdateLED();
  delay(100);
}

void measureTiltAndUpdateLED() {
  int range_start = LED_RANGE_START_LOOKUP[readBackFront()][readLeftRight()] * LED_QUADRANT_SIZE;
  
  changeColorForRangeStartingAt(range_start, 255, 0, 255);
} 

bool readBackFront() {
  bool isTiltingForwards = digitalRead(TILT_BACK_FRONT_PIN);
  logBool("isTiltingForwards", isTiltingForwards); 
  return isTiltingForwards;
}

bool readLeftRight() {
  bool isTiltingLeft = digitalRead(TILT_LEFT_RIGHT_PIN);
  logBool("isTiltingLeft", isTiltingLeft); 
  return isTiltingLeft;
}

void logBool(String booleanName, bool value) {
  if (LOGGING_ENABLED) {
    Serial.print(booleanName);
    Serial.print(": ");
    Serial.println(value);
  }
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

// References: 
// https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
// https://docs.arduino.cc/tutorials/generic/tilt-sensor
