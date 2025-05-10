//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

//=====[Defines]===============================================================

#define OVER_TEMP_LEVEL 50
#define NUMBER_OF_AVG_SAMPLES 100
#define TIME_INCREMENT_MS 500

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);
AnalogIn potentiometer(A0);
AnalogIn lm35(A1);
DigitalIn mq2(PE_12);

//=====[Global Variables]======================================================

float lm35ReadingsArray[NUMBER_OF_AVG_SAMPLES];
float lm35ReadingsSum = 0.0;
float lm35ReadingsAverage = 0.0;
float lm35TempC = 0.0;
float potentiometerReading = 0.0;
bool gasDetected = false;
bool overTemp = false;

//=====[Function Prototypes]===================================================

void updateSensorReadings();
void displaySensorStatus();
float analogReadingScaledWithTheLM35Formula(float analogReading);

//=====[Main Function]=========================================================

int main()
{
    uartUsb.write("System Initialised.\r\n", 23);
    int lm35SampleIndex = 0;

    while (true) {
        lm35ReadingsArray[lm35SampleIndex] = lm35.read();
        lm35SampleIndex = (lm35SampleIndex + 1) % NUMBER_OF_AVG_SAMPLES;

        // Compute average
        lm35ReadingsSum = 0.0;
        for (int i = 0; i < NUMBER_OF_AVG_SAMPLES; i++) {
            lm35ReadingsSum += lm35ReadingsArray[i];
        }
        lm35ReadingsAverage = lm35ReadingsSum / NUMBER_OF_AVG_SAMPLES;
        lm35TempC = analogReadingScaledWithTheLM35Formula(lm35ReadingsAverage);

        // Read other sensors
        potentiometerReading = potentiometer.read();
        gasDetected = !mq2;
        overTemp = lm35TempC > OVER_TEMP_LEVEL;

        displaySensorStatus();
        ThisThread::sleep_for(TIME_INCREMENT_MS);
    }
}

//=====[Function Implementations]=============================================

void displaySensorStatus()
{
    char buffer[128];
    int len;

    len = sprintf(buffer, "Temperature: %.2f C\r\n", lm35TempC);
    uartUsb.write(buffer, len);

    len = sprintf(buffer, "Potentiometer: %.2f\r\n", potentiometerReading);
    uartUsb.write(buffer, len);

    if (gasDetected) {
        uartUsb.write("Gas Detected\r\n", 14);
    } else {
        uartUsb.write("No Gas Detected\r\n", 18);
    }

    if (overTemp) {
        uartUsb.write("Temperature Alarm\r\n", 19);
    } else if (gasDetected) {
        uartUsb.write("Gas Alarm\r\n", 11);
    } else {
        uartUsb.write("No Alarms\r\n", 11);
    }
    uartUsb.write("--------------------------\r\n", 28);
}

float analogReadingScaledWithTheLM35Formula(float analogReading)
{
    return (analogReading * 3.3 / 0.01);
}
