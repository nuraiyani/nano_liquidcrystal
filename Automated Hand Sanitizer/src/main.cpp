#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "usart.h"
#include "servo.h"

#define TRIG_PIN  PD2
#define ECHO_PIN  PD3
#define HEART_RATE_PIN 0

void initUltrasonicSensor();
uint16_t measureDistance();
void initADC();
uint16_t readADC(uint8_t channel);

int main(void) {
    // Initialize components
    usart_init();
    initUltrasonicSensor();
    initADC();
    initServo();

    uint16_t distance;
    uint16_t heartRate;

    char buffer[50];
    
    while (1) {
        // Measure distance using ultrasonic sensor
        distance = measureDistance();

        // Read heart rate from sensor
        //heartRate = readADC(HEART_RATE_PIN);

        // Convert ADC value to a usable format (e.g., beats per minute)
        uint16_t bpm = heartRate * 2;  // Simplified conversion for demonstration

        // Control servo based on
        }
}