#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define BIT_TIME 256000

#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))

#define GBA_DDR DDRB
#define GBA_OUT PORTB
#define GBA_IN PINB
#define CLOCK_BIT 1
#define MOSI_BIT 2
#define MISO_BIT 3

inline void cpuPreScale(uint8_t n) {
	CLKPR = 0x80;
	CLKPR = n;
}

inline void initTimer() {
	OCR0A = F_CPU / BIT_TIME;
    TCCR0A = (1 << WGM00) | (1 << WGM01) | (1 << COM0A0);
    TCCR0B = (1 << WGM02) | (1 << CS00);
    TIMSK0 = 0;
}

inline void resetTimer() {
    TCNT0 = 0;
    TIFR0 = 1 << OCF0A;
}

inline void waitTimer() {
	while((TIFR0 & (1 << OCF0A)) == 0);
	TIFR0 = 1 << OCF0A;
}

void setup() {

	CPU_PRESCALE(0);

	// Set MOSI to input
	GBA_DDR &= ~(1 << MISO_BIT);

	// Set MOSI and CLOCK to output
	GBA_DDR |= (1 << MOSI_BIT) | (1 << CLOCK_BIT);

	// Start with clock set high
	GBA_OUT |= (1 << CLOCK_BIT);

	Serial.begin(38400);

	_delay_ms(1000);

	initTimer();
}

void loop() {

	if(Serial.available() >= 4) {
		uint32_t data = 0;

		// Read 4 bytes into 'data'
		data |= Serial.read() << 24;
		data |= Serial.read() << 16;
		data |= Serial.read() << 8;
		data |= Serial.read();

		// Perform the exchange between normal serial
		// and GBA's 32-bit protocol.
        // Before the exchange, 'data' contains data from PC --> GBA
	    exchange(&data);
        // After the exchange, 'data' contains data from GBA --> PC

		// Write any data that was sent from the GBA during exchange
		Serial.write(data >> 24) & 0xff;
		Serial.write(data >> 16) & 0xff;
		Serial.write(data >> 8) & 0xff;
		Serial.write(data) & 0xff;

		// Flush the serial port because
		// only should be dealing with 4 bytes at a time
		Serial.flush();
	}
}

/*
 * Having data be an input pointer reduces return overhead
 */
void exchange(uint32_t *data_) {

    // Pull in the value of the input pointer
	uint32_t data = *data_;

	// Clear global interrupt flag
	// This function must not be interrupted
	cli();

	// Reset the timer to prevent overflow
	resetTimer();

	for(uint8_t i = 0; i < 32; i++) {

		// Set the CLOCK and MOSI low to start bit exchange
		GBA_OUT &= ~((1 << CLOCK_BIT) | (1 << MOSI_BIT));

		// Set MOSI to the MSB of data
		// Note that below, the MSB of data is popped off, like a shift register
		GBA_OUT |= ((data >> 31) & 1) << MOSI_BIT;

		// Wait for AVR timer to tick
		waitTimer();

		// Set GBA clock high
		GBA_OUT |= 1 << CLOCK_BIT;

		// Push a bit off the data that came from USB
		// the least significant bit is now for reading from GBA
		data <<= 1;

		// Pull a bit in from the GBA and put it in the LSB of data
		data |= (GBA_IN >> MISO_BIT) & 1;

		// Wait for AVR timer to tock
		waitTimer();
	}

	// Set global interrupt flag
	sei();

    // Set the input pointer's value
    *data_ = data;
}
