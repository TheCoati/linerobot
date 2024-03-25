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
const int DISPLAY_OFF[7] = { LOW, LOW, LOW, LOW, LOW, LOW, LOW };
const int LETTER_S[7] = { HIGH, LOW, HIGH, HIGH, LOW,  HIGH, HIGH };
const int LETTER_T[7] = { LOW,  LOW, LOW,  HIGH, HIGH, HIGH, HIGH };

// Display numbers
const int NUMBERS[10][7] = {
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 0
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 1
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 2
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 3
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 4
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 5
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 6
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 7
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 8
    { HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH }, // 9
};

// Configuration
const int FINISH_DETECTED_DELAY = 200;       // How long the sensor should detect full black to trigger finish.
const int DEFAULT_LEFT_SPEED = 75;           // Default left engine speed in forward motion.
const int DEFAULT_RIGHT_SPEED = 60;          // Default right engine speed in forward motion.
const int TURN_SPEED = 80;                   // Speed while turning.
const int ADJUST_SPEED = 40;                 // Speed while adjusting.
const int ROTATE_SPEED = 50;                 // Speed while turning around on both engine.
const int ROTATE_ON_OBJECT_DISTANCE = 5.0;   // Object distance to trigger turn around.
const int DISPLAY_MULTIPLEX_DELAY = 1;       // Delay between displays for multiplexing.

enum Sensors
{
    SENSOR_1 = (1 << 0), // 1
    SENSOR_2 = (1 << 1), // 2
    SENSOR_3 = (1 << 2), // 4
    SENSOR_4 = (1 << 3), // 8
    SENSOR_5 = (1 << 4), // 16
};

UltraSonicDistanceSensor distanceSensor(TRIG_PIN, ECHO_PIN);

bool started = false;

unsigned long now = millis();
unsigned long finishDetectedTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastPingSensorUpdate = 0;

bool hasDetectedObject = false;

int currentDisplay = 0;
int* display[2] = { DISPLAY_OFF, DISPLAY_OFF };

/**
 * Show value on the displays.
 * Method will switch displays on interval for multiplexing.
 */
void renderDisplay()
{
    unsigned long timePassed = now - lastDisplayUpdate;
    int commonPin = DISPLAY_PINS[currentDisplay];
    int size = sizeof(display) / sizeof(display[0]);

    digitalWrite(commonPin, LOW);

    for (int i = 0; i < 7; i++) {
        int segmentPin = DISPLAY_SEGMENT_PINS[i];
        int segmentValue = display[currentDisplay][i];

        digitalWrite(segmentPin, segmentValue);
    }

    digitalWrite(commonPin, HIGH);

    // Switch display (Multiplex)
    if (timePassed >= DISPLAY_MULTIPLEX_DELAY && size > 1) {
        currentDisplay = (currentDisplay == 0 ? 1 : 0);
        lastDisplayUpdate = millis();
    } else {
        currentDisplay = 0;
    }
}

/**
 * Change the value of the displays.
 * @param display1 Array with output values for display 1
 * @param display2 Array with output values for display 2
 */
void setDisplay(int display1[7], int display2[7])
{
    display[0] = display1 == NULL ? DISPLAY_OFF : display1;
    display[1] = display2 == NULL ? DISPLAY_OFF : display2;    
}

/**
 * Start the countdown to start the robot.
 */
void startCountdown()
{

}

/**
 * Run ping sensor to detect an object.
 * @return bool when an object is in configured range
 */
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

/**
 * Move the robot straight forward.
 */
void moveForward()
{
    // Left = HIGH - Forward
    // Right = LOW - Forward
    digitalWrite(LEFT_DIRECTION_PIN, HIGH);
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, DEFAULT_LEFT_SPEED);
    analogWrite(RIGHT_PWM_PIN, DEFAULT_RIGHT_SPEED);
}

/**
 * Adjust the robot slightly to the left.
 */
void adjustLeft()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, 0);
    analogWrite(RIGHT_PWM_PIN, ADJUST_SPEED);

    analogWrite(LEFT_PWM_PIN, DEFAULT_LEFT_SPEED);
    analogWrite(RIGHT_PWM_PIN, DEFAULT_RIGHT_SPEED);
}

/**
 * Adjust the robot slightly to the right.
 */
void adjustRight()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, ADJUST_SPEED);
    analogWrite(RIGHT_PWM_PIN, 0);

    analogWrite(LEFT_PWM_PIN, DEFAULT_LEFT_SPEED);
    analogWrite(RIGHT_PWM_PIN, DEFAULT_RIGHT_SPEED);
}

/**
 * Make the robot turn hard left.
 */
void turnLeft()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, 0);
    analogWrite(RIGHT_PWM_PIN, TURN_SPEED);
}

/**
 * Make the robot turn hard right.
 */
void turnRight()
{
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, TURN_SPEED);
    analogWrite(RIGHT_PWM_PIN, 0);
}

/**
 * Turn the robot around.
 */
void turnAround()
{
    digitalWrite(LEFT_DIRECTION_PIN, LOW);
    digitalWrite(RIGHT_DIRECTION_PIN, LOW);

    analogWrite(LEFT_PWM_PIN, ROTATE_SPEED);
    analogWrite(RIGHT_PWM_PIN, ROTATE_SPEED);
}

/**
 * Stop the robot.
 */
void stop() {
    analogWrite(RIGHT_PWM_PIN, 0);
    analogWrite(LEFT_PWM_PIN, 0);
}

/**
 * Detect if the robot has reached the finish.
 * @return bool when the robot keeps checking the finish for configured amount of time
 */
bool checkFinish()
{
    if (finishDetectedTime == 0) {
        finishDetectedTime = millis();
        return;
    }

    unsigned long timePassed = now - finishDetectedTime;

    return timePassed >= FINISH_DETECTED_DELAY;
}

void setup()
{
    Serial.end(); // Disable Serial

    TCCR2B = TCCR2B & B11111000 | B00000111;

    // Motors
    pinMode(LEFT_PWM_PIN, OUTPUT);
    pinMode(RIGHT_PWM_PIN, OUTPUT);
    pinMode(LEFT_DIRECTION_PIN, OUTPUT);
    pinMode(RIGHT_DIRECTION_PIN, OUTPUT);

    // Line sensor
    for (int i = 0; i < 5; i++) {
        pinMode(SENSOR_PINS[i], INPUT);
    }

    // Display commons
    for (int i = 0; i < 2; i++) {
        pinMode(DISPLAY_PINS[i], OUTPUT);
        digitalWrite(DISPLAY_PINS[i], HIGH);
    }

    // Display segments
    for (int i = 0; i < 7; i++) {
        pinMode(DISPLAY_SEGMENT_PINS[i], OUTPUT);
    }

    // Ping sensor
    pinMode(ECHO_PIN, INPUT);
    pinMode(TRIG_PIN, OUTPUT);
    digitalWrite(TRIG_PIN, LOW);

    // Default display value St.
    setDisplay(LETTER_S, LETTER_T);
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
            if (checkFinish()) {
                stop();
            }
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

//    int sensor1 = digitalRead(SENSOR_PINS[0]);
//    int sensor2 = digitalRead(SENSOR_PINS[1]);
//    int sensor3 = digitalRead(SENSOR_PINS[2]);
//    int sensor4 = digitalRead(SENSOR_PINS[3]);
//    int sensor5 = digitalRead(SENSOR_PINS[4]);
//
//    if (sensor1 == LOW && sensor2 == LOW && sensor3 == LOW && sensor4 == LOW && sensor5 == LOW) {
//        if (checkFinish()) {
//            stop();
//        }
//    } else if (sensor1 == HIGH && sensor2 == HIGH && sensor3 == LOW && sensor4 == HIGH && sensor5 == HIGH) {
//        moveForward();
//    } else if (sensor1 == LOW && sensor2 == LOW && sensor3 == LOW && sensor4 == HIGH && sensor5 == HIGH) {
//        turnLeft();
//    } else if (sensor1 == HIGH && sensor2 == HIGH && sensor3 == LOW && sensor4 == LOW && sensor5 == LOW) {
//        turnRight();
//    } else if (sensor1 == HIGH && sensor2 == LOW && sensor3 == LOW && sensor4 == HIGH && sensor5 == HIGH) {
//        adjustLeft();
//    } else if (sensor1 == HIGH && sensor2 == HIGH && sensor3 == LOW && sensor4 == LOW && sensor5 == HIGH) {
//        adjustRight();
//    } else if (sensor1 == HIGH && sensor2 == HIGH && sensor3 == HIGH && sensor4 == HIGH && sensor5 == HIGH) {
//        turnAround();
//    } else if (sensor1 == HIGH && sensor2 == LOW && sensor3 == HIGH && sensor4 == HIGH && sensor5 == HIGH){
//        adjustLeft();
//    } else if (sensor1 == HIGH && sensor2 == HIGH && sensor3 == HIGH && sensor4 == LOW && sensor5 == HIGH) {
//        adjustRight();
//    } else {
//        turnRight();
//    }

    // If state was not detecting the finish reset the finish detection timer
    if (state != (SENSOR_1 | SENSOR_2 | SENSOR_3 | SENSOR_4 | SENSOR_5)) {
        finishDetectedTime = 0;
    }
}