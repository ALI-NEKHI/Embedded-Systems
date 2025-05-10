
#include "mbed.h"
#include "arm_book_lib.h"

DigitalIn b1(BUTTON1);
DigitalIn d2(D2); // Gas sensor
DigitalIn d3(D3); // Temp sensor
DigitalIn d4(D4); // Code A
DigitalIn d5(D5); // Code B
DigitalIn d6(D6); // Code C
DigitalIn d7(D7); // Code D

DigitalOut led1(LED1); // Alarm
DigitalOut led2(LED2); // Emergency
DigitalOut led3(LED3); // Lockout

Timer lockoutTimer;

bool alarmActive = false;
bool emergencyMode = false;
bool locked = false;
int wrongAttempts = 0;

void resetSystem() {
    alarmActive = false;
    emergencyMode = false;
    locked = false;
    wrongAttempts = 0;
    lockoutTimer.stop();
    lockoutTimer.reset();
    led1 = led2 = led3 = OFF;
}

bool correctCodeEntered() {
    return d4 && d5 && d6 && d7;  // Example 4-digit combo: all HIGH
}

bool incorrectCodeEntered() {
    return d4 || d5 || d6 || d7;  // Any incorrect combo
}

int main() {
    // Set all inputs to PullDown
    b1.mode(PullDown); d2.mode(PullDown); d3.mode(PullDown);
    d4.mode(PullDown); d5.mode(PullDown); d6.mode(PullDown); d7.mode(PullDown);

    while (true) {
        // Handle lockout mode
        if (locked) {
            led3 = !led3; // blink slowly
            ThisThread::sleep_for(500ms);
            if (lockoutTimer.read() >= 60) {
                locked = false;
                wrongAttempts = 0;
                led3 = OFF;
                lockoutTimer.stop();
                lockoutTimer.reset();
            }
            continue;
        }

        // Trigger alarm if gas or temp detected
        if (d2 || d3) {
            alarmActive = true;
        }

        // Trigger emergency mode if both detected
        if (d2 && d3) {
            emergencyMode = true;
        }

        // Emergency Mode: flashing LED1
        if (emergencyMode) {
            for (int i = 0; i < 3; i++) {
                led1 = ON; led2 = ON;
                ThisThread::sleep_for(100ms);
                led1 = OFF; led2 = OFF;
                ThisThread::sleep_for(100ms);
            }

            // Wait for correct code
            if (correctCodeEntered()) {
                resetSystem();
            } else if (incorrectCodeEntered()) {
                wrongAttempts++;
                if (wrongAttempts >= 5) {
                    locked = true;
                    lockoutTimer.start();
                }
            }
            continue;
        }

        // Regular alarm mode: steady LED
        led1 = alarmActive;

        // Reset if correct code entered
        if (alarmActive && correctCodeEntered()) {
            resetSystem();
        }

        // Wrong code increases attempts
        if (alarmActive && incorrectCodeEntered()) {
            wrongAttempts++;
            if (wrongAttempts >= 5) {
                locked = true;
                lockoutTimer.start();
            }
        }

        ThisThread::sleep_for(100ms);
    }
}
