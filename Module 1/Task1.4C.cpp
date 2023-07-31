
// PUBLISHER ARDUINO UNIT =================================================================================
//  ======================================================================================================
//  ======================================================================================================


#include <Wire.h>


// C++ code
//

const byte events [] = {
 	0b00000001, 	
 	0b00000010,
   	0b00000100
};

void setup()
{
  Wire.begin(1);
  Serial.begin(9600);
}


int x = 0;
void loop()
{
  Serial.println("Sending!");
  Wire.beginTransmission(2);
  Wire.write(events[x % 3]);
  x += 1;
  Wire.endTransmission();
  delay(rand() % 8000 + 2000);
}












// CONSUMER ARDUINO UNIT =================================================================================
//  ======================================================================================================
//  ======================================================================================================



// CONSTANTS ================================================================================

// LIBRARIES
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// PORT D
#define NUMBER_OF_COLOUR_SELECTORS 6
#define MAIN_RED_SELECTOR 8
#define MAIN_GREEN_SELECTOR 9
#define MAIN_BLUE_SELECTOR 10
#define SECONDARY_RED_SELECTOR 11
#define SECONDARY_GREEN_SELECTOR 12
#define SECONDARY_BLUE_SELECTOR 13

// PORT B
#define LED_PIN 4
#define NUMBER_OF_TILT_SENSORS 2
#define TILT_BACK_FRONT_PIN 2
#define TILT_LEFT_RIGHT_PIN 3
#define NOTIFICATION_BUTTON 5

// TIMERS
#define ARDUINO_FREQUENCY 16000000

// TILT MODE CONFIG
#define FADE_STEP 15
#define FADE_STEP_DURATION_IN_SECONDS 0.1

// OTHER CONFIG
#define LED_COUNT 12
#define LOGGING_ENABLED false



// TYPE DEFINITIONS ================================================================================
enum Mode {
  TILT,
  NOTIFICATION
};


// GLOBAL VARIABLES ================================================================================
const byte COLOUR_PINS[] = {
  MAIN_RED_SELECTOR,
  MAIN_GREEN_SELECTOR,
  MAIN_BLUE_SELECTOR,
  SECONDARY_RED_SELECTOR,
  SECONDARY_GREEN_SELECTOR,
  SECONDARY_BLUE_SELECTOR
};

const String COLOUR_DESCRIPTIONS[] = {
  "MAIN_RED",
  "MAIN_GREEN",
  "MAIN_BLUE",
  "SECONDARY_RED",
  "SECONDARY_GREEN",
  "SECONDARY_BLUE"
};

const byte TILT_SENSOR_PINS[] = { TILT_BACK_FRONT_PIN, TILT_LEFT_RIGHT_PIN };

// Division can be expensive, and this can be calculated once on init.
const int LED_QUADRANT_SIZE = LED_COUNT / 4;

// N dimensional matrix. Each dimension corresponse to the off/on status of each tilt sensor.
// This look up will provide the starting index for any given combination of sensor statuses.
const int LED_RANGE_START_LOOKUP[2][2] = {
  { 0 * LED_QUADRANT_SIZE, 1 * LED_QUADRANT_SIZE },
  { 3 * LED_QUADRANT_SIZE, 2 * LED_QUADRANT_SIZE }
};

// Init the object for the NeoPixel
Adafruit_NeoPixel ledRing(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// General State
Mode mode = TILT;

// Tilt State
int tiltMainColour[3];
int tiltSecondaryColour[3];
int tiltBrightness = 254;
bool isFadeCurrentlyDescending = true;

// Notification state
int rotateTailLEDIndex = 0;
int notificationColour[3] = { 0, 0, 0 };
int notificationHalfColour[3] = { 0, 0, 0 };
int notificationTenthColour[3] = { 0, 0, 0 };







// Timer Interrupts ================================================================================

void setupTimerInterrupts(double interruptFrequency) {
  // Clear timer control registers
  TCCR1A = 0;
  TCCR1B = 0;

  // Clear timer counter
  TCNT1 = 0;

  // Clear timer on compare match
  TCCR1B |= (1 << WGM12);

  // Use 256 prescaler
  TCCR1B |= (1 << CS12);

  // Set compare match on timer 1.
  // duration * frequency / prescaler  - 1 (for zero-index)
  OCR1A = interruptFrequency * ARDUINO_FREQUENCY / 256 - 1;

  // Enable timer overflow interrupt
  TIMSK1 |= (1 << OCIE1A);
}

ISR(TIMER1_COMPA_vect) {
  if (mode == NOTIFICATION) {
    rotateLED();
  } else if (mode == TILT) {
    fadeLED();
  }
}








// Pin Interrupts ================================================================================

void enableTiltInterrupts() {
  for (int i = 0; i < NUMBER_OF_TILT_SENSORS; i++) {
    initInterruptPin(TILT_SENSOR_PINS[i], INPUT);
  }

  for (int i = 0; i < NUMBER_OF_COLOUR_SELECTORS; i++) {
    initInterruptPin(COLOUR_PINS[i], INPUT_PULLUP);
  }
}

void disableTiltInterrupts() {
  for (int i = 0; i < NUMBER_OF_TILT_SENSORS; i++) {
    disableInterrupt(TILT_SENSOR_PINS[i]);
  }

  for (int i = 0; i < NUMBER_OF_COLOUR_SELECTORS; i++) {
    disableInterrupt(COLOUR_PINS[i]);
  }
}

void initInterruptPin(byte pin, byte type) {
  // Enable pin with specified type
  pinMode(pin, type);

  // Enable interrupts on the port of the specified pin
  PCICR |= bit(digitalPinToPCICRbit(pin));

  // Enable interrupts on the pin
  *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));
}

void disableInterrupt(byte pin) {
  *digitalPinToPCMSK(pin) ^= bit(digitalPinToPCMSKbit(pin));
}

// Interrupts from pins D8-D13
ISR(PCINT0_vect) {
  if (mode == TILT) {
    loadColours();
    measureTiltAndUpdateLED();
  }
}

// Interrupts from pins D1-D7
ISR(PCINT2_vect) {
  // Dismiss notification if button is pressed
  if (mode == NOTIFICATION && digitalRead(NOTIFICATION_BUTTON == HIGH)) {
    mode = TILT;

    loadColours();
    measureTiltAndUpdateLED();

    // Restart tilt interrupts
    Wire.begin(2);
    disableInterrupt(NOTIFICATION_BUTTON);
    enableTiltInterrupts();
  } else {
    measureTiltAndUpdateLED();
  }
}








// I2C Interrupts ================================================================================
void setupI2C() {
  Wire.begin(2);
  Wire.onReceive(displayNotification);
}






// SETUP ================================================================================

void setup() {
  // Stop interrupts during setup
  cli();

  Serial.begin(9600);

  setupTimerInterrupts(FADE_STEP_DURATION_IN_SECONDS);
  enableTiltInterrupts();
  setupI2C();

  // Init LED strip
  pinMode(LED_PIN, OUTPUT);
  ledRing.begin();
  loadColours();
  measureTiltAndUpdateLED();

  // Resume interupts
  sei();
}







// LOOP ================================================================================

void loop() {
  // All functionality is done through interrupts.
}






// TILT MODE LOGIC ================================================================================

void measureTiltAndUpdateLED() {
  bool isTiltingForwards = read(TILT_BACK_FRONT_PIN, "tilting forwards");
  bool isTiltingRight = read(TILT_LEFT_RIGHT_PIN, "tilting right");
  int range_start = LED_RANGE_START_LOOKUP[isTiltingForwards][isTiltingRight];

  updateColorForRangeStartingAt(range_start);
}

void loadColours() {
  for (int i = 0; i < 3; i++) {
    tiltMainColour[i] = readColour(COLOUR_PINS[i], COLOUR_DESCRIPTIONS[i]);
    tiltSecondaryColour[i] = readColour(COLOUR_PINS[i + 3], COLOUR_DESCRIPTIONS[i + 3]);
  }
}

int readColour(byte pin, String colour) {
  return read(pin, colour) ? 0 : 254;
}

void updateColorForRangeStartingAt(int start) {
  ledRing.clear();
  for (int i = 0; i < LED_COUNT; i++) {
    if (i >= start && i < start + LED_QUADRANT_SIZE) {
      setPixelColour(i, tiltMainColour);
    } else {
      setPixelColour(i, tiltSecondaryColour);
    }
  }
  ledRing.show();
}

void fadeLED() {
  if (isFadeCurrentlyDescending) {
    tiltBrightness += FADE_STEP;
  } else {
    tiltBrightness -= FADE_STEP;
  }

  if (tiltBrightness < 20 || tiltBrightness > 234) {
    isFadeCurrentlyDescending = !isFadeCurrentlyDescending;
  }

  if (tiltBrightness < 20) {
    tiltBrightness = 20;
  } else if (tiltBrightness > 234) {
    tiltBrightness = 234;
  }

  setBrightness(tiltBrightness);
  ledRing.show();
}





// NOTIFICATION MODE LOGIC ================================================================================
void displayNotification(int messageLength) {
  log("Message Received");
  cli();

  // Disable new notifications from occurring
  Wire.end();

  // Enable interrupts for the notification mode and disable other interrupts
  disableTiltInterrupts();
  initInterruptPin(NOTIFICATION_BUTTON, INPUT);

  setNotificationColour(Wire.read());
  mode = NOTIFICATION;

  sei();
}

void setNotificationColour(byte eventIdentifier) {
  int colour[3] = { 0, 0, 0 };

  switch (eventIdentifier) {
    case 0b1:
      colour[0] = 254;
      break;
    case 0b10:
      colour[1] = 254;
      break;
    case 0b100:
      colour[2] = 254;
      break;
  }

  for (int i = 0; i < 3; i++) {
    notificationColour[i] = colour[i];
    notificationHalfColour[i] = colour[i] * 0.5;
    notificationTenthColour[i] = colour[i] * 0.1;
  }
}

void rotateLED() {
  ledRing.clear();
  setPixelColour((rotateTailLEDIndex + 2) % LED_COUNT, notificationColour);
  setPixelColour((rotateTailLEDIndex + 1) % LED_COUNT, notificationHalfColour);
  setPixelColour(rotateTailLEDIndex, notificationTenthColour);
  ledRing.show();
  rotateTailLEDIndex = (rotateTailLEDIndex + 1) % LED_COUNT;
}








// PROXY I/O METHODS ================================================================================
// Below are proxy methods to log what is read in, and sent out.

bool read(byte pin, String description) {
  bool value = digitalRead(pin);
  if (LOGGING_ENABLED) {
    Serial.print("READING ");
    Serial.print(description);
    Serial.print(": ");
    Serial.println(value);
  }
  return value;
}

void setPixelColour(int n, int rgb[3]) {
  if (LOGGING_ENABLED) {
    Serial.print("Setting element # [");
    Serial.print(n);
    Serial.print("] to RGB value [");
    Serial.print(rgb[0]);
    Serial.print(", ");
    Serial.print(rgb[1]);
    Serial.print(", ");
    Serial.print(rgb[2]);
    Serial.println("].");
  }

  ledRing.setPixelColor(n, rgb[0], rgb[1], rgb[2]);
}

void setBrightness(int brightness) {

  if (LOGGING_ENABLED) {
    Serial.print("Setting brightness to: ");
    Serial.println(brightness);
  }

  ledRing.setBrightness(brightness);
}

void log(String message) {
  if (LOGGING_ENABLED) {
    Serial.println(message);
  }
}

/*
REFERENCES:
   https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
   https://docs.arduino.cc/tutorials/generic/tilt-sensor
   https://playground.arduino.cc/Main/PinChangeInterrupt/
   https://www.sparkfun.com/news/2613
   https://docs.arduino.cc/learn/communication/wire
*/