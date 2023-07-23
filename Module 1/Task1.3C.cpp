// LIBRARIES
#include <Adafruit_NeoPixel.h>

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

// OTHER CONFIG
#define LED_COUNT 12
#define LOGGING_ENABLED false

const byte COLOUR_PINS[] = {
    MAIN_RED_SELECTOR,
    MAIN_GREEN_SELECTOR,
    MAIN_BLUE_SELECTOR,
    SECONDARY_RED_SELECTOR,
    SECONDARY_GREEN_SELECTOR,
    SECONDARY_BLUE_SELECTOR};

const String COLOUR_DESCRIPTIONS [] = {
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
    {0 * LED_QUADRANT_SIZE, 1 * LED_QUADRANT_SIZE},
    {3 * LED_QUADRANT_SIZE, 2 * LED_QUADRANT_SIZE}};

// Init the object for the NeoPixel
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Store the colours
int MAIN_COLOUR[3];
int SECONDARY_COLOUR[3];

void initInterruptPin(byte pin, byte type)
{
    // Enable pin with specified type
    pinMode(pin, type);

    // Enable interrupts on the port of the specified pin
    PCICR |= bit(digitalPinToPCICRbit(pin));

    // Enable interrupts on the pin
    *digitalPinToPCMSK(pin) |= bit(digitalPinToPCMSKbit(pin));
}

ISR(PCINT0_vect)
{
    loadColours();
    // Reload LEDs after loading colours to display the newly selected colour.
    measureTiltAndUpdateLED();
}

ISR(PCINT2_vect)
{
    measureTiltAndUpdateLED();
}

void setup()
{
    Serial.begin(9600);

    for (int i = 0; i < NUMBER_OF_TILT_SENSORS; i++)
    {
        initInterruptPin(TILT_SENSOR_PINS[i], INPUT);
    }

    for (int i = 0; i < NUMBER_OF_COLOUR_SELECTORS; i++)
    {
        initInterruptPin(COLOUR_PINS[i], INPUT_PULLUP);
    }
  
  	pinMode(LED_PIN, OUTPUT);

    strip.begin();
  
    loadColours();
    measureTiltAndUpdateLED();
}

void loop() {}

void loadColours()
{
    for (int i = 0; i < 3; i++)
    {
        MAIN_COLOUR[i] = readColour(COLOUR_PINS[i], COLOUR_DESCRIPTIONS[i]);
        SECONDARY_COLOUR[i] = readColour(COLOUR_PINS[i + 3], COLOUR_DESCRIPTIONS[i + 3]);
    }
}

int readColour(byte pin, String colour)
{
    return read(pin, colour) ? 0 : 254;
}

void measureTiltAndUpdateLED()
{
    bool isTiltingForwards = read(TILT_BACK_FRONT_PIN, "tilting forwards");
    bool isTiltingRight = read(TILT_LEFT_RIGHT_PIN, "tilting right");
    int range_start = LED_RANGE_START_LOOKUP[isTiltingForwards][isTiltingRight];

    changeColorForRangeStartingAt(range_start);
}

void changeColorForRangeStartingAt(int start)
{
    strip.clear();
    for (int i = 0; i < LED_COUNT; i++)
    {
        setPixelColour(i, SECONDARY_COLOUR);
    }
    for (int i = start; i < start + LED_QUADRANT_SIZE; i++)
    {
        setPixelColour(i, MAIN_COLOUR);
    }
    strip.show();
}

// Below are proxy methods to log what is read in, and sent out.

bool read(byte pin, String description)
{
    bool value = digitalRead(pin);
    if (LOGGING_ENABLED)
    {
        Serial.print("READING ");
        Serial.print(description);
        Serial.print(": ");
        Serial.println(value);
    }
    return value;
}

void setPixelColour(int n, int rgb[3])
{
    if (LOGGING_ENABLED)
    {
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

    strip.setPixelColor(n, rgb[0], rgb[1], rgb[2]);
}

/*
REFERENCES:
   https://learn.adafruit.com/adafruit-neopixel-uberguide/arduino-library-use
   https://docs.arduino.cc/tutorials/generic/tilt-sensor
   https://playground.arduino.cc/Main/PinChangeInterrupt/
*/