#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL // Define CPU frequency for delay functions

#define TRIGGER_PIN PD2
#define ECHO_PIN PD3

#define TRIGGER_PORT PORTD
#define TRIGGER_DDR DDRD
#define TRIGGER_PIN_REG PIND

#define ECHO_PORT PORTD
#define ECHO_DDR DDRD
#define ECHO_PIN_REG PIND

#define YELLOW_LED_PORT PORTB
#define YELLOW_LED_DDR DDRB
#define YELLOW_LED_PIN PB2

#define GREEN_LED_PORT PORTB
#define GREEN_LED_DDR DDRB
#define GREEN_LED_PIN PB3

#define SERVO_PORT PORTB
#define SERVO_DDR DDRB
#define SERVO_PIN PB1

volatile uint16_t pulse_duration = 0;

void io_init(void) {
    // Set trigger pin as output
    TRIGGER_DDR |= (1 << TRIGGER_PIN);
    
    // Set echo pin as input
    ECHO_DDR &= ~(1 << ECHO_PIN);
    
    // Set LED pins as output
    YELLOW_LED_DDR |= (1 << YELLOW_LED_PIN);
    GREEN_LED_DDR |= (1 << GREEN_LED_PIN);
    
    // Set servo pin as output
    SERVO_DDR |= (1 << SERVO_PIN);
}

void set_yellow_led(uint8_t state) {
    if (state) {
        YELLOW_LED_PORT |= (1 << YELLOW_LED_PIN);
    } else {
        YELLOW_LED_PORT &= ~(1 << YELLOW_LED_PIN);
    }
}

void set_green_led(uint8_t state) {
    if (state) {
        GREEN_LED_PORT |= (1 << GREEN_LED_PIN);
    } else {
        GREEN_LED_PORT &= ~(1 << GREEN_LED_PIN);
    }
}

void trigger_ultrasonic() {
    // Send a 10us pulse to trigger pin
    TRIGGER_PORT |= (1 << TRIGGER_PIN);
    _delay_us(10);
    TRIGGER_PORT &= ~(1 << TRIGGER_PIN);
}

uint16_t measure_distance() {
    uint16_t distance = 0;
    
    trigger_ultrasonic();
    
    // Wait for echo pin to go high
    while (!(ECHO_PIN_REG & (1 << ECHO_PIN)));
    
    // Start timer
    TCNT1 = 0;
    TCCR1B |= (1 << CS11);
    
    // Wait for echo pin to go low
    while (ECHO_PIN_REG & (1 << ECHO_PIN));
    
    // Stop timer
    TCCR1B &= ~(1 << CS11);
    pulse_duration = TCNT1;
    
    // Calculate distance in cm
    distance = (pulse_duration / 2) / 58;
    
    return distance;
}

void servo_set_angle(uint8_t angle) {
    // Calculate the pulse width for the given angle
    uint16_t pulse_width = (angle * 2) + 50; // Adjust for prescaler of 64, result in ticks
    
    // Set the servo pin to output the calculated pulse width
    OCR0A = pulse_width;
}

void timer0_init() {
    // Set up Timer/Counter0 for Fast PWM mode with OCR0A as TOP
    TCCR0A = (1 << WGM01) | (1 << WGM00) | (1 << COM0A1);
    TCCR0B = (1 << WGM02) | (1 << CS01) | (1 << CS00); // Prescaler 64
    OCR0A = 0; // Initialize Output Compare Register A
}

int main(void) {
    io_init();
    timer0_init();
    sei(); // Enable global interrupts
    
    while (1) {
        uint16_t distance = measure_distance();
        
        if (distance <= 10) {
            set_green_led(1); // Turn on green LED
            set_yellow_led(0); // Turn off yellow LED
            servo_set_angle(90); // Turn servo to 90 degrees
        } else if (distance <= 20) {
            set_green_led(0); // Turn off green LED
            set_yellow_led(1); // Turn on yellow LED
            servo_set_angle(180); // Turn servo to 180 degrees
        } else {
            set_green_led(0); // Turn off green LED
            set_yellow_led(0); // Turn off yellow LED
            servo_set_angle(0); // Turn servo to 0 degrees (or any default position)
        }

        _delay_ms(100); // Small delay for stability
    }

    return 0;
}
