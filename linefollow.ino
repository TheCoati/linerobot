#include <HCSR04.h>

// Ping sensor
const int TRIG_PIN = A0;
const int ECHO_PIN = 8;

// Motorshield
const int RIGHT_PWM_PIN = 3;
const int LEFT_PWM_PIN = 11;
const int LEFT_DIRECTION_PIN = 13;
const int RIGHT_DIRECTION_PIN = 12;

// Line sensor
const int SENSOR_PINS[5] = { A1, A2, A3, A4, A5 };

// LED displays
const int DISPLAY_PINS[2] = { 9, 10 };
const int DISPLAY_SEGMENT_PINS[7] = { 0, 1, 2, 4, 5, 6, 7 };

// Display letters
const int LETTER_S[7] = { HIGH, LOW, HIGH, HIGH, LOW, HIGH, HIGH };
const int LETTER_T[7] = { LOW, LOW, LOW, HIGH, HIGH, HIGH, HIGH };

// Configuration
const int FINISH_DETECTED_DELAY = 200;
const int DEFAULT_LEFT_SPEED = 75;
const int DEFAULT_RIGHT_SPEED = 60;
const int TURN_SPEED = 80;
const int ADJUST_SPEED = 40;
const int ROTATE_SPEED = 50;
const int ROTATE_ON_OBJECT_DISTANCE = 5.0;
const int DISPLAY_MULTIPLEX_DELAY = 1;

enum Sensors
{
    SENSOR_1 = (1 << 0),
    SENSOR_2 = (1 << 1),
    SENSOR_3 = (1 << 2),
    SENSOR_4 = (1 << 3),
    SENSOR_5 = (1 << 4),
};

UltraSonicDistanceSensor distanceSensor(TRIG_PIN, ECHO_PIN);

bool started = false;
unsigned long now = millis();
unsigned long finishDetectedTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastPingSensorUpdate = 0;
bool hasDetectedObject = false;

int currentDisplay = 0;
int* display[2] = { LETTER_S, LETTER_T };

void renderDisplay()
{
    unsigned long timePassed = now - lastDisplayUpdate;

    int commonPin = DISPLAY_PINS[currentDisplay];

    digitalWrite(commonPin, LOW);

    for (int i = 0; i < 7; i++) {
        int segmentPin = DISPLAY_SEGMENT_PINS[i];
        int segmentValue = display[currentDisplay][i];

        digitalWrite(segmentPin, segmentValue);
    }

    digitalWrite(commonPin, HIGH);

    // Switch display
    if (timePassed >= DISPLAY_MULTIPLEX_DELAY) {
        currentDisplay = (currentDisplay == 0 ? 1 : 0);
        lastDisplayUpdate = millis();
    }
}

bool detectObject()
{
    unsigned long timePassed = now - lastPingSensorUpdate;

    if (timePassed >= 60) {
        lastPingSensorUpdate = millis();
        double distance = distanceSensor.measureDistanceCm();

        hasDetectedObject = distance <= ROTATE_ON_OBJECT_DISTANCE;
    }

    return hasDetectedObject;
}

void moveForward()
{
    // Left = HIGH - Forward
    // Right = LOW - Forward
    digitalWrite(LEFT_DIRECTION_PIN, HIGH);
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, DEFAULT_LEFT_SPEED);
    analogWrite(RIGHT_PWM_PIN, DEFAULT_RIGHT_SPEED);
}

void turnLeft()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, 0);
    analogWrite(RIGHT_PWM_PIN, TURN_SPEED);
}

void turnRight()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, TURN_SPEED);
    analogWrite(RIGHT_PWM_PIN, 0);
}

void adjustLeft()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, 0);
    analogWrite(RIGHT_PWM_PIN, ADJUST_SPEED);

    analogWrite(LEFT_PWM_PIN, DEFAULT_LEFT_SPEED);
    analogWrite(RIGHT_PWM_PIN, DEFAULT_RIGHT_SPEED);
}

void adjustRight()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, ADJUST_SPEED);
    analogWrite(RIGHT_PWM_PIN, 0);

    analogWrite(LEFT_PWM_PIN, DEFAULT_LEFT_SPEED);
    analogWrite(RIGHT_PWM_PIN, DEFAULT_RIGHT_SPEED);
}

void turnAround()
{
    digitalWrite(LEFT_DIRECTION_PIN, LOW);
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, ROTATE_SPEED);
    analogWrite(RIGHT_PWM_PIN, ROTATE_SPEED);
}

void stop() {
    analogWrite(RIGHT_PWM_PIN, 0);
    analogWrite(LEFT_PWM_PIN, 0);
}

void checkFinish()
{
    if (finishDetectedTime == 0) {
        finishDetectedTime = millis();
        return;
    }

    unsigned long timePassed = now - finishDetectedTime;

    if (timePassed >= FINISH_DETECTED_DELAY) {
        stop();
    }
}

void setup()
{
    Serial.end();
    // Serial.begin(9600);

    TCCR2B = TCCR2B & B11111000 | B00000111;

    pinMode(LEFT_PWM_PIN, OUTPUT);
    pinMode(RIGHT_PWM_PIN, OUTPUT);
    pinMode(LEFT_DIRECTION_PIN, OUTPUT);
    pinMode(RIGHT_DIRECTION_PIN, OUTPUT);

    for (int i = 0; i < 5; i++) {
        pinMode(SENSOR_PINS[i], INPUT);
    }

    for (int i = 0; i < 2; i++) {
        pinMode(DISPLAY_PINS[i], OUTPUT);
        digitalWrite(DISPLAY_PINS[i], HIGH);
    }

    for (int i = 0; i < 7; i++) {
        pinMode(DISPLAY_SEGMENT_PINS[i], OUTPUT);
    }

    pinMode(ECHO_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    digitalWrite(TRIG_PIN, LOW);
}

void loop()
{
    now = millis();

    // Displays
    renderDisplay();

    // Ping sensor
    if (detectObject()) {
        turnAround();
        return;
    }

    // Line sensor
    int sensor1 = digitalRead(SENSOR_PINS[0]) == LOW ? SENSOR_1 : 0;
    int sensor2 = digitalRead(SENSOR_PINS[1]) == LOW ? SENSOR_2 : 0;
    int sensor3 = digitalRead(SENSOR_PINS[2]) == LOW ? SENSOR_3 : 0;
    int sensor4 = digitalRead(SENSOR_PINS[3]) == LOW ? SENSOR_4 : 0;
    int sensor5 = digitalRead(SENSOR_PINS[4]) == LOW ? SENSOR_5 : 0;

    const int state = (sensor1 | sensor2 | sensor3 | sensor4 | sensor5);

    switch (state) {
        case (SENSOR_1 | SENSOR_2 | SENSOR_3 | SENSOR_4 | SENSOR_5):
            checkFinish();
            break;
        case (SENSOR_3):
            moveForward();
            break;
        case (SENSOR_1 | SENSOR_2 | SENSOR_3):
            turnLeft();
            break;
        case (SENSOR_3 | SENSOR_4 | SENSOR_5):
            turnRight();
            break;
        case (SENSOR_2):
        case (SENSOR_2 | SENSOR_3):
            adjustLeft();
            break;
        case (SENSOR_4):
        case (SENSOR_3 | SENSOR_4):
            adjustRight();
            break;
        case (0): // All sensors HIGH
            turnAround();
            break;
        default: // Hand of wall - Always right
            turnRight();
    }

    // If state was not detecting the finish reset the finish detection timer
    if (state != (SENSOR_1 | SENSOR_2 | SENSOR_3 | SENSOR_4 | SENSOR_5)) {
        finishDetectedTime = 0;
    }
}