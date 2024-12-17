#include <avr/io.h>
#include "usart.h"

void usart_init() {
    // Set baud rate
    uint16_t ubrr = F_CPU / 16 / 9600 - 1;
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    
    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    
    // Set frame format: 8 data bits, 1 stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void usart_sendString(const char *str) {
    while (*str) 
    {
        while (!(UCSR0A & (1 << UDRE0)));
        UDR0 =*str++;
        }
}