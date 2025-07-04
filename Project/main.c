/*
 * Project.c
 *
 * Created: 04-Jul-25 7:58:21 AM
 * Author : User
 */ 
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "lcd.h"

#define MAX_PASSWORD_LENGTH 4

const char * password = "1234";
char entered_password[MAX_PASSWORD_LENGTH + 1] = "\0"; // Initialize with null terminator
uint8_t password_length = 0;

void keypad_init(void)
{
    DDRA = 0x0F;
    DDRB = 0xFF;
    PORTA = 0xFF;
}

char keypad_characters[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

char get_keypad_key(void)
{
    // The previous DDRA = 0xFF; in init is fine for now, but 0x0F is cleaner
    // And PORTA = 0xFF; in init should be fine to set all outputs high initially.

    for (uint8_t row = 0; row < 4; row++)
    {
        // Step 1: Set all row pins (PA0-PA3) HIGH first
        PORTA = 0xFF; // Ensure PA0-PA3 are HIGH. This clears any lingering low states from previous scans.

        // Step 2: Set the current row low
        PORTA &= ~(1 << row); // Drives PA0, PA1, PA2, PA3 low sequentially
        _delay_ms(10);

        // Check each column
        for (uint8_t col = 0; col < 4; col++)
        {
            if (!(PINA & (1 << (col + 4)))) // Column pins are PB0, PB1, PB2, PB3
            {
                // Wait for key release (debounce)
                while (!(PINA & (1 << (col + 4))));
                
                // Set the current row high again immediately after detection
                PORTA |= (1 << row);
                
                return keypad_characters[row][col];
            }
        }
        
        // No key found in this row, set it high again before moving to next row
        // This line is technically redundant if you do PORTA |= 0x0F at the start of the loop
        PORTA |= (1 << row); 
    }
    
    return '\0'; // No key pressed
}

void keypad_scan(void)
{
    char key = get_keypad_key();
    if( key == '\0' )
    {
        return;
    }
    PORTB = 0x04;
    _delay_ms(50); // Debounce delay
    PORTB = 0x00; // Clear PORTD after debounce
    if(key == '#') // Check if the '#' key is pressed
    {
        if (strcmp(entered_password, password) == 0) // Compare entered password with the correct one
        {
            // Password is correct, perform action
            PORTB = 0x01; // For example, turn on an LED connected to PORTD
            lcd_cmd(0x01); // Clear LCD
            _delay_ms(2); // Wait for the clear command to complete
            lcd_cmd(0x80); // Clear LCD
            lcd_print("Access Granted");
            _delay_ms(1000); // Keep the LED on for 1 second
            PORTB = 0x00; // Turn off the LED
        }
        else
        {
            // Password is incorrect, handle accordingly
            PORTB = 0x02; // For example, turn on another LED connected to PORTD
            lcd_cmd(0x01); // Clear LCD
            _delay_ms(2); // Wait for the clear command to complete
            lcd_cmd(0x80); // Clear LCD

            lcd_print("Access Denied");
            _delay_ms(1000); // Keep the LED on for 1 second
            PORTB = 0x00; // Turn off the LED
        }
        entered_password[0] = '\0'; // Reset entered password after checking
        password_length = 0; // Reset password length
    }
    else if (key == '*') // Check if the '*' key is pressed to reset the entered password
    {
        entered_password[0] = '\0'; // Reset entered password
        password_length = 0; // Reset password length
    }
    else if(key == 'A') 
    {
        // Handle 'A' key press, for example, you can implement a specific action
    }
    else if(key == 'B') 
    {
        // Handle 'B' key press, for example, you can implement a specific action
    }
    else if(key == 'C') 
    {
        // Handle 'C' key press, for example, you can implement a specific action
    }
    else if(key == 'D') 
    {
        // Handle 'D' key press, for example, you can implement a specific action
    }
    else
    {
        // Append the pressed key to the entered password
        entered_password[password_length] = key; // Store the pressed key
        if (password_length < MAX_PASSWORD_LENGTH) 
        {
            password_length++; // Increment the password length
        }
        entered_password[password_length] = '\0'; // Null-terminate the string
    }

}


int main(void)
{
    keypad_init(); // Initialize the keypad
    LCD_DDR_DATA |= 0xF0; // D4-D7 ouotput
    LCD_DDR_CTRL |= (1 << RS) | (1 << E); // PC6, PC7 output
    lcd_init();
    lcd_print("Enter PIN:");
    while (1) 
    {
        keypad_scan(); // Scan the keypad for key presses
        _delay_ms(50); // Add a small delay to debounce the keys
    }
}

