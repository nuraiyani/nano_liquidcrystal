#include <avr/io.h>
#include <util/delay.h>
#include "servo.h"

void initServo() {
    // Set PWM for servo control on pin 9
    DDRB |= (1 << PB1);
    TCCR1A = (1 << WGM11) | (1 << COM1A1);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    ICR1 = 19999; // 20ms period
}

void setServoAngle(uint8_t angle) {
    OCR1A = ((angle *11) + 500);
}