//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"
#include "keypad.h"
#include "rtc_api.h"

//=====[Defines]===============================================================

#define CODE_LENGTH 4
#define MAX_LOG_ENTRIES 5

//=====[Global Objects]========================================================

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);
Keypad keypad;

DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);

//=====[Global Variables]======================================================

const char validCode[CODE_LENGTH] = {'1', '8', '0', '5'};
char enteredCode[CODE_LENGTH];
int codeIndex = 0;
bool alarmActive = true;

struct AlarmLog {
    time_t timestamp;
};

AlarmLog logEntries[MAX_LOG_ENTRIES];
int logIndex = 0;

//=====[Function Prototypes]===================================================

void logAlarmEvent();
void showAlarmLog();
void promptCodeEntry();
void handleKeyInput(char key);
bool validateCode();
void setupRTC();

//=====[Main]==================================================================

int main() {
    keypad.init();
    setupRTC();

    uartUsb.write("Enter Code to Deactivate Alarm\r\n", 34);

    while (true) {
        char key = keypad.getKey();
        if (key != '\0') {
            handleKeyInput(key);
        }
        ThisThread::sleep_for(100ms);
    }
}

//=====[Function Implementations]=============================================

void setupRTC() {
    rtc_init();
    // Set RTC to known value if needed. (Manually via serial command in real setup)
}

void logAlarmEvent() {
    if (logIndex >= MAX_LOG_ENTRIES) {
        for (int i = 1; i < MAX_LOG_ENTRIES; i++) {
            logEntries[i - 1] = logEntries[i];
        }
        logIndex = MAX_LOG_ENTRIES - 1;
    }
    logEntries[logIndex++].timestamp = time(NULL);
}

void showAlarmLog() {
    char buffer[64];
    for (int i = 0; i < logIndex; i++) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S\r\n", localtime(&logEntries[i].timestamp));
        uartUsb.write(buffer, strlen(buffer));
    }
}

void promptCodeEntry() {
    uartUsb.write("Enter Code to Deactivate Alarm\r\n", 34);
    codeIndex = 0;
}

void handleKeyInput(char key) {
    if (alarmActive && codeIndex < CODE_LENGTH && key != '#' && key != '*') {
        enteredCode[codeIndex++] = key;
    }
    if (key == '#') {
        if (alarmActive && codeIndex == CODE_LENGTH && validateCode()) {
            alarmActive = false;
            alarmLed = 0;
            incorrectCodeLed = 0;
            uartUsb.write("Alarm Deactivated\r\n", 20);
        } else {
            incorrectCodeLed = 1;
            uartUsb.write("Incorrect Code\r\n", 17);
        }
        promptCodeEntry();
    } else if (key == '*') {
        showAlarmLog();
    }
    if (alarmActive && codeIndex == CODE_LENGTH && !validateCode()) {
        incorrectCodeLed = 1;
        uartUsb.write("Incorrect Code\r\n", 17);
        promptCodeEntry();
    }
    if (alarmActive && codeIndex == 1) {
        alarmLed = 1;
        logAlarmEvent();
        uartUsb.write("Alarm Triggered - Logging Time\r\n", 33);
    }
}

bool validateCode() {
    for (int i = 0; i < CODE_LENGTH; i++) {
        if (enteredCode[i] != validCode[i]) {
            return false;
        }
    }
    return true;
}
