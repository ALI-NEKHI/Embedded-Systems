//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"
#include "keypad.h"
#include "lcd.h"

//=====[Defines]===============================================================

#define CODE_LENGTH 4
#define OVER_TEMP_THRESHOLD 50.0
#define GAS_DETECTED_STATE 0
#define LCD_UPDATE_INTERVAL_MS 5000

//=====[Global Objects]========================================================

Keypad keypad;
Lcd lcd(I2C_SDA, I2C_SCL, PCF8574_ADDR);

AnalogIn lm35(A1);
DigitalIn mq2(PE_12);
DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Global Variables]======================================================

const char correctCode[CODE_LENGTH] = {'1', '8', '0', '5'};
char inputCode[CODE_LENGTH];
int codeIndex = 0;
bool alarmActive = true;

Timer lcdUpdateTimer;

//=====[Function Prototypes]===================================================

void initializeSystem();
void updateSystem();
void checkKeypad();
void updateLCD();
bool validateCode();
void displayWarningIfNeeded();

//=====[Main Function]=========================================================

int main() {
    initializeSystem();
    lcdUpdateTimer.start();
    lcd.print("Enter Code to\nDeactivate Alarm\n\n\n");

    while (true) {
        checkKeypad();
        if (lcdUpdateTimer.read_ms() >= LCD_UPDATE_INTERVAL_MS) {
            updateLCD();
            lcdUpdateTimer.reset();
        }
        ThisThread::sleep_for(100ms);
    }
}

//=====[Function Implementations]=============================================

void initializeSystem() {
    keypad.init();
    lcd.init();
    lcd.backlight();
}

void checkKeypad() {
    char key = keypad.getKey();

    if (key != '\0') {
        if (key == '2') {
            lcd.cls();
            lcd.printf("Gas Detector: %s\n", mq2.read() == GAS_DETECTED_STATE ? "ALERT" : "Safe");
        } else if (key == '3') {
            float temp = lm35.read() * 3.3 / 0.01;
            lcd.cls();
            lcd.printf("Temp: %.1f C\n%s\n", temp, temp > OVER_TEMP_THRESHOLD ? "OVER LIMIT" : "Normal");
        } else if (key >= '0' && key <= '9' && codeIndex < CODE_LENGTH) {
            inputCode[codeIndex++] = key;
        } else if (key == '#' && codeIndex == CODE_LENGTH) {
            if (validateCode()) {
                alarmActive = false;
                lcd.cls();
                lcd.print("Alarm Deactivated\n\n\n\n");
                alarmLed = 0;
                incorrectCodeLed = 0;
            } else {
                lcd.cls();
                lcd.print("Incorrect Code\nTry Again\n\n\n");
                incorrectCodeLed = 1;
            }
            codeIndex = 0;
        }
    }
}

bool validateCode() {
    for (int i = 0; i < CODE_LENGTH; i++) {
        if (inputCode[i] != correctCode[i]) {
            return false;
        }
    }
    return true;
}

void updateLCD() {
    float temp = lm35.read() * 3.3 / 0.01;
    lcd.cls();
    lcd.printf("Temp: %.1f C\n", temp);
    lcd.printf("Gas: %s\n", mq2.read() == GAS_DETECTED_STATE ? "ALERT" : "Safe");
    lcd.printf("Alarm: %s\n", alarmActive ? "ON" : "OFF");
    displayWarningIfNeeded();
}

void displayWarningIfNeeded() {
    float temp = lm35.read() * 3.3 / 0.01;
    if (mq2.read() == GAS_DETECTED_STATE || temp > OVER_TEMP_THRESHOLD) {
        lcd.printf("!! WARNING !!\n");
    }
}
