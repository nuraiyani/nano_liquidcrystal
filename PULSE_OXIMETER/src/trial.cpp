#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define TRIG_PIN PD2
#define ECHO_PIN PD3
#define BUZZER_PIN PB0
#define SERVO_PIN1 PB1
#define SERVO_PIN2 PB2

#define SOUND 250

volatile uint16_t melodyPitch[] = {659, 659, 0, 659, 0, 523, 659, 0, 784};
volatile uint8_t melodyDuration[] = {10, 10, 10, 10, 10, 10, 10, 10, 10};

void setup() {
    // Set up pins
    DDRD |= (1 << TRIG_PIN); // TRIG_PIN as output
    DDRD &= ~(1 << ECHO_PIN); // ECHO_PIN as input
    DDRB |= (1 << BUZZER_PIN); // BUZZER_PIN as output
    DDRB |= (1 << SERVO_PIN1); // SERVO_PIN1 as output
    DDRB |= (1 << SERVO_PIN2); // SERVO_PIN2 as output
    
    // Initialize servos to starting positions
    // Assuming servo control is handled through PWM on PB1 and PB2
    // Initialize Timer1 for Fast PWM, 8-bit mode
    TCCR1A = (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1);
    TCCR1B = (1 << WGM12) | (1 << CS11); // Prescaler set to 8
    
    OCR1A = 128; // 90 degrees
    OCR1B = 0; // 0 degrees
}

uint16_t pulseIn(uint8_t pin, uint8_t state) {
    uint16_t width = 0;
    uint8_t bit = (1 << pin);
    uint8_t stateMask = state ? bit : 0;

    while ((PIND & bit) != stateMask) {}
    while ((PIND & bit) == stateMask) {
        width++;
        _delay_us(1);
        if (width >= 65535) break;
    }
    
    return width;
}

void playMelody(volatile uint16_t *pitch, volatile uint8_t *duration, uint8_t length) {
    for (uint8_t i = 0; i < length; i++) {
        if (pitch[i] == 0) {
            _delay_ms(duration[i] * 10);
        } else {
            tone(BUZZER_PIN, pitch[i], duration[i] * 10);
        }
    }
}

void tone(uint8_t pin, uint16_t frequency, uint16_t duration) {
    uint16_t delayValue = (1000000 / frequency) / 2;
    uint32_t numCycles = (uint32_t)frequency * (uint32_t)duration / 1000;
    for (uint32_t i = 0; i < numCycles; i++) {
        PORTB |= (1 << pin);
        _delay_us(delayValue);
        PORTB &= ~(1 << pin);
        _delay_us(delayValue);
    }
}

void loop() {
    uint16_t duration, distance;
    
    PORTD &= ~(1 << TRIG_PIN); // Set TRIG_PIN low
    _delay_us(2);
    PORTD |= (1 << TRIG_PIN); // Set TRIG_PIN high
    _delay_us(10);
    PORTD &= ~(1 << TRIG_PIN); // Set TRIG_PIN low
    
    duration = pulseIn(ECHO_PIN, 1);
    distance = (duration / 2) / 29.1;
    
    if (distance < 8) {
        // Hand detected
        playMelody(melodyPitch, melodyDuration, 9);
        _delay_ms(500);
        
        OCR1A = 0; // 0 degrees
        OCR1B = 128; // 90 degrees
        _delay_ms(3000);
        OCR1A = 128; // 90 degrees
        OCR1B = 0; // 0 degrees
    } else if (distance > 12 || distance <= 0) {
        // Hand not detected
    } else {
        // Hand too far
        tone(BUZZER_PIN, 784, 200);
        _delay_ms(500);
    }
    
    _delay_ms(500);
}

int main(void) {
    setup();    while (1) {
        loop();
    }
}
