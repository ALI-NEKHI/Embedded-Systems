//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//=====[Declaration and initialization of public global objects]===============

DigitalIn enterButton(BUTTON1);
DigitalIn gasDetector(D2);
DigitalIn overTempDetector(D3);
DigitalIn aButton(D4);
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);

DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);
Timer lockoutTimer;

//=====[Declaration and initialization of public global variables]=============

bool alarmState = OFF;
bool emergencyMode = false;
bool systemLocked = false;
int numberOfIncorrectCodes = 0;

//=====[Declarations (prototypes) of public functions]=========================

void inputsInit();
void outputsInit();

void alarmActivationUpdate();
void alarmDeactivationUpdate();
void emergencyCheck();

void uartTask();
void availableCommands();

//=====[Main function]=========================================================

int main()
{
    inputsInit();
    outputsInit();
    while (true) {
        alarmActivationUpdate();
        emergencyCheck();
        alarmDeactivationUpdate();
        uartTask();
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    gasDetector.mode(PullDown);
    overTempDetector.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);
    enterButton.mode(PullDown);
}

void outputsInit()
{
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
}

void alarmActivationUpdate()
{
    if (gasDetector || overTempDetector) {
        alarmState = ON;
    }
    if (!emergencyMode) {
        alarmLed = alarmState;
    }
}

void emergencyCheck()
{
    if (gasDetector && overTempDetector) {
        emergencyMode = true;
        // Flash alarm LED rapidly to indicate emergency
        for (int i = 0; i < 3; i++) {
            alarmLed = ON;
            ThisThread::sleep_for(100ms);
            alarmLed = OFF;
            ThisThread::sleep_for(100ms);
        }
    }
}

void alarmDeactivationUpdate()
{
    if (systemLocked) {
        // Check if lockout period has ended
        if (lockoutTimer.read() >= 60) {
            numberOfIncorrectCodes = 0;
            systemLocked = false;
            systemBlockedLed = OFF;
            lockoutTimer.stop();
            lockoutTimer.reset();
        }
        return;
    }

    if (numberOfIncorrectCodes < 5) {
        // Check if code entry is initiated
        if (aButton && bButton && cButton && dButton && !enterButton) {
            incorrectCodeLed = OFF;  // Reset indicator if all buttons are HIGH before enter
        }

        if (enterButton && !incorrectCodeLed && alarmState) {
            // Correct code: A & B ON, C & D OFF
            if (aButton && bButton && !cButton && !dButton) {
                alarmState = OFF;
                emergencyMode = false;
                numberOfIncorrectCodes = 0;
                incorrectCodeLed = OFF;
                alarmLed = OFF;
            } else {
                incorrectCodeLed = ON;
                numberOfIncorrectCodes++;
            }
        }
    }

    // Block the system after 5 incorrect attempts
    if (numberOfIncorrectCodes >= 5) {
        systemBlockedLed = ON;
        systemLocked = true;
        lockoutTimer.start();
    }
}

void uartTask()
{
    char receivedChar = '\0';
    if (uartUsb.readable()) {
        uartUsb.read(&receivedChar, 1);
        if (receivedChar == '1') {
            if (alarmState) {
                uartUsb.write("The alarm is activated\r\n", 24);
            } else {
                uartUsb.write("The alarm is not activated\r\n", 28);
            }
        } else if (receivedChar == '2') {
            if (systemLocked) {
                uartUsb.write("System is blocked\r\n", 20);
            } else {
                uartUsb.write("System is not blocked\r\n", 24);
            }
        } else {
            availableCommands();
        }
    }
}

void availableCommands()
{
    uartUsb.write("Available commands:\r\n", 21);
    uartUsb.write("Press '1' to get the alarm state\r\n", 35);
    uartUsb.write("Press '2' to get system lock status\r\n\r\n", 40);
}
