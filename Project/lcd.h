#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

// LCD pins
#define RS PC6
#define E  PC7

#define LCD_PORT_DATA PORTD
#define LCD_DDR_DATA  DDRD

#define LCD_PORT_CTRL PORTC
#define LCD_DDR_CTRL  DDRC

void lcd_cmd(unsigned char cmd);
void lcd_data(unsigned char data);
void lcd_init(void);
void lcd_print(char *str);
void lcd_send_nibble(unsigned char nibble);
void lcd_goto(unsigned char row, unsigned char col);
void adc_init(void);
uint16_t adc_read(uint8_t channel);

char buffer[16]; // For voltage string

// int main(void) {
// 	LCD_DDR_DATA |= 0xF0; // PD4–PD7 output
// 	LCD_DDR_CTRL |= (1 << RS) | (1 << E); // PC6, PC7 output

// 	lcd_init();
// 	adc_init();
	
// 	// lcd_print("Patience!!");

// 	while (1) {
// 		uint16_t adc_val = adc_read(0); // Read from ADC0 (PA0)
// 		uint16_t mv = (adc_val * 5000UL) / 1023;
// 		snprintf(buffer, sizeof(buffer), "Voltage: %u.%02u V", mv / 1000, (mv % 1000) / 10);

// 		lcd_cmd(0x80);        // Go to line 1, pos 0
// 		// lcd_print("Voltage Read:");
// 		// lcd_cmd(0xC0);        // Go to line 2
// 		lcd_print(buffer);

// 		_delay_ms(100);
// 	}
// }

void lcd_send_nibble(unsigned char nibble) {
	LCD_PORT_DATA = (LCD_PORT_DATA & 0x0F) | (nibble & 0xF0);
	LCD_PORT_CTRL |= (1 << E);
	_delay_us(1);
	LCD_PORT_CTRL &= ~(1 << E);
	_delay_us(100);
}

void lcd_cmd(unsigned char cmd) {
	LCD_PORT_CTRL &= ~(1 << RS);
	lcd_send_nibble(cmd);
	lcd_send_nibble(cmd << 4);
}

void lcd_data(unsigned char data) {
	LCD_PORT_CTRL |= (1 << RS);
	lcd_send_nibble(data);
	lcd_send_nibble(data << 4);
}

void lcd_init(void) {
	_delay_ms(20);
	lcd_cmd(0x02);
	lcd_cmd(0x28);
	lcd_cmd(0x0C);
	lcd_cmd(0x06);
	lcd_cmd(0x01);
	_delay_ms(2);
}

void lcd_print(char *str) {
	while (*str) {
		lcd_data(*str++);
	}
}
