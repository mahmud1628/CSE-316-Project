/*
 * Project.c
 *
 * Created: 04-Jul-25 7:58:21 AM
 * Author : User
 */ 
#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "lcd.h"
#include <avr/interrupt.h>

#define NOT_ARMED 0
#define PARTIALLY_ARMED 1
#define FULLY_ARMED 2
#define MAX_PASSWORD_LENGTH 16

const char * password = "1234";
const char * master_password = "0000";
char entered_password[MAX_PASSWORD_LENGTH + 1] = "\0"; // Initialize with null terminator
char master_entered_password[MAX_PASSWORD_LENGTH + 1] = "\0"; // Initialize with null terminator
uint8_t password_length = 0;
uint8_t master_password_length = 0; // Length of the master password
uint8_t incorrect_attempts = 0;
uint8_t master_incorrect_attempts = 0; // Counter for incorrect master password attempts
uint8_t is_blocked = 0; // Flag to indicate if the system is blocked
volatile uint8_t door_opened = 0; // Flag to indicate if the door is opened
uint8_t armed_status = NOT_ARMED; // Initial armed status
uint8_t is_authorized = 0; // Flag to indicate if the user is authorized
uint8_t door_opened_reed_switch = 0; // Flag to indicate if the door is opened by the reed switch
uint8_t buzzer_type = 0; // 0 no sound, 1 beep beep, 2 continuous sound
uint8_t master_login_period = 0; // Flag to indicate if the master login period is active
uint8_t is_master_logged_in = 0; // Flag to indicate if the master is logged in

volatile uint8_t is_lock_opened = 0; // Flag to indicate if the lock is opened
uint8_t mode_change_period = 0;
volatile uint8_t is_close_lock = 0; // Flag to indicate if the lock is closed
volatile uint8_t rfid_flag = 0;

void close_lock(void);
void open_lock(void);
void mode_change_initialize(void);
void write_to_lcd_according_to_mode(void);
void handle_master_login(void);
void change_mode(char key);
void beep(void);
void pir_enable(void);
void pir_disable(void);
void send_sms_unauthorized_access(void);
void send_sms_intruder(void);
void send_sms_multiple_attempts(void);
void turn_on_buzzer(void);
void turn_off_buzzer(void);

void send_sms_intruder(void)
{
    PORTB &= ~(1 << PB3);
}

void send_sms_multiple_attempts(void)
{
    PORTB &= ~(1 << PB4);
}

void send_sms_unauthorized_access(void)
{
	PORTB &= ~(1 << PB5);
}

void mode_change_initialize(void)
{
    if(mode_change_period)
    {
        mode_change_period = 0;
        lcd_write_at_first_line("Master access");
        return;
    }
    mode_change_period = 1;
    if(armed_status == NOT_ARMED)
    {
        lcd_write_at_first_line("Current: Off");
        lcd_write_at_second_line("1. Semi 2. Full");
    }
    else if(armed_status == PARTIALLY_ARMED)
    {
        lcd_write_at_first_line("Current: Semi");
        lcd_write_at_second_line("0. Off 2. Full");
    }
    else if(armed_status == FULLY_ARMED)
    {
        lcd_write_at_first_line("Current: Full");
        lcd_write_at_second_line("0. Off 1. Semi");
    }
}

void change_mode(char key)
{
    if(key == '0') // Off
    {
        armed_status = NOT_ARMED;
        pir_disable();
        lcd_write_at_first_line("Mode: Off");
    }
    else if(key == '1') // Semi
    {
        armed_status = PARTIALLY_ARMED;
        pir_disable();
        lcd_write_at_first_line("Mode: Semi");
    }
    else if(key == '2') // Full
    {
        armed_status = FULLY_ARMED;
        cli();
        pir_enable();
        lcd_write_at_first_line("Mode: Full");
    }
    mode_change_period = 0; // Reset mode change period
    _delay_ms(1000);
    lcd_write_at_first_line("Master access");
    if(key == '2')
        sei(); // Re-enable global interrupts
}

void handle_master_login(void)
{
    if(strcmp(master_entered_password, master_password) == 0) // Compare entered master password with the correct one
    {
        // Master password is correct, perform action
        lcd_write_at_first_line("Master Access");
        is_master_logged_in = 1; // Set the master login flag
        master_login_period = 0; // Reset the master login period flag
        master_incorrect_attempts = 0; // Reset incorrect attempts for master password
    }
    else
    {
        master_incorrect_attempts++; // Increment the incorrect attempts counter
        if(master_incorrect_attempts >= 3) {            // Master password is incorrect, handle accordingly
            master_incorrect_attempts = 0; // Reset attempts after showing message
            //beep(); // Call the beep function to indicate blocking
            turn_on_buzzer(); // Turn on an Buzzer connected to PB0
            return;
        }
        else {
            lcd_write_at_first_line("Access Denied");
            _delay_ms(1000); // Keep the LED on for 1 second
            lcd_write_at_first_line("Enter master PIN");
        }
    }
    master_entered_password[0] = '\0'; // Reset entered master password after checking
    master_password_length = 0; // Reset master password length
}

void write_to_lcd_according_to_mode(void)
{
    if(armed_status == NOT_ARMED)
    {
        lcd_write_at_first_line("Press # to open");
        lcd_write_at_second_line("the lock");
    }
    else if(armed_status == PARTIALLY_ARMED)
    {
        lcd_write_at_first_line("Enter PIN");
    }
    else if(armed_status == FULLY_ARMED)
    {
        lcd_write_at_first_line("Enter PIN");
    }
}

// INT0 -> reed switch (1)
ISR(INT0_vect)
{
    if(is_lock_opened)
    {
        // check if door already opened
        if(door_opened)
        {
            door_opened = 0;
            close_lock(); // Close the lock if it was opened
            is_close_lock = 1;
            _delay_ms(5000); // Keep the message on for 5 seconds
        }
        else 
        {
            door_opened = 1;
            lcd_write_at_first_line("Door Opened");
            _delay_ms(1000); // Keep the message on for 1 seconds
        }
        return;
    }
    else if(armed_status == NOT_ARMED) return;
    else if(!is_authorized){
        lcd_write_at_first_line("Unauthorized");
		lcd_write_at_second_line("access");
		send_sms_unauthorized_access();
        is_blocked = 1; // Set the blocked flag
        buzzer_type = 2; // Set buzzer type to continuous sound
        PORTB |= (1 << PB0); // Turn on an Buzzer connected to PB0
        return; // Exit the ISR if not authorized
    }
}

// INT1 -> PIR (1)
ISR(INT1_vect)
{
    if(!is_authorized)
    {
        lcd_write_at_first_line("Intruder");
        lcd_write_at_second_line("detected");
        send_sms_intruder(); // Send SMS for intruder detection
        is_blocked = 1; // Set the blocked flag
        buzzer_type = 2; // Set buzzer type to continuous sound
        PORTB |= (1 << PB0); // Turn on an Buzzer connected to PB0
        return; // Exit the ISR if not authorized
    }
}

// INT2 -> RFID
ISR(INT2_vect)
{
    if(rfid_flag)
    {
        rfid_flag = 0;
        return;
    }
    if(!is_authorized)
    {
        lcd_write_at_first_line("Access Granted");
        _delay_ms(1000); // Keep the message on for 1 second
        open_lock();
        _delay_ms(1000);
        is_authorized = 1; // Set the authorized flag
    }
}

void rfid_init(void)
{
    GICR |= (1 << INT2);
    MCUCSR &= ~(1 << ISC2);
}

void enable_int0(void)
{
    GICR |= (1 << INT0); // Enable external interrupt INT0
    MCUCR |= (1 << ISC01) | (1 << ISC00); // Set INT0 to trigger on rising edge
}

void disable_int0(void)
{
    GICR &= ~(1 << INT0); // Disable external interrupt INT0
}

void enable_int1(void)
{
    GICR |= (1 << INT1); // Enable external interrupt INT1
    MCUCR |= (1 << ISC11) | (1 << ISC10); // Set INT1 to trigger on rising edge
}

void disable_int1(void)
{
    GICR &= ~(1 << INT1); // Disable external interrupt INT1
}

void turn_on_buzzer(void)
{
    PORTB |= (1 << PB0);
}

void turn_off_buzzer(void)
{
    PORTB &= ~(1 << PB0);
}

// void interrupt_init(void)
// {
//     // GICR |= (1 << INT0);      // Enable external interrupt INT0
// 	// GICR |= (1 << INT1);
//     // MCUCR |= (0<< ISC01) | (1 << ISC00); // rising edge for INT0
//     // MCUCR |= (0 << ISC11) | (1 << ISC10); // rising edge for INT1
//     // GIFR |= (1 << INTF0);     // Clear any pending INT0 interrupt flag
//     // DDRD &= ~(1 << PD2); // Set PD2 as input for INT0
//     // PORTD |= (1 << PD2); // Enable pull-up resistor on PD2
// }

void reed_switch_enable(void)
{
    GICR |= (1 << INT0);
    MCUCR |= (0 << ISC01) | (1 << ISC00); // any logical change for reed switch
}

void pir_enable(void)
{
    GICR |= (1 << INT1);
    MCUCR |= (1 << ISC11) | (1 << ISC10); // rising edge for pir
}

void pir_disable(void)
{
    GICR &= ~(1 << INT1);
}

void keypad_init(void)
{
    DDRA = 0x0F;
    DDRB = 0xFB;
    PORTA = 0xFF;
}

void beep(void)
{
    for(int i = 0; i < 5; i++){
        turn_on_buzzer(); // Turn on the buzzer
        _delay_ms(50); // Beep duration
        turn_off_buzzer(); // Turn off the buzzer
        _delay_ms(50); // Delay between beeps
    }
}

char keypad_characters[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void open_lock(void)
{
    is_lock_opened = 1; // Set the lock opened flag
    PORTB &= ~(1 << PB1); // Open the lock by setting PB1 high
    //_delay_ms(1000); // Keep the lock open for 1 second
    //PORTB |= (1 << PB1);
    lcd_write_at_first_line("Lock Opened");
}

void close_lock(void)
{
    is_lock_opened = 0; // Reset the lock opened flag
    //is_authorized = 0; // Reset the authorized flag
    //PORTB &= ~(1 << PB1); // Close the lock by setting PB1 low
	PORTB |= (1 << PB1);
    lcd_write_at_first_line("Lock Closed");
    _delay_ms(1000); // Keep the message on for 1 second
    write_to_lcd_according_to_mode();
}

char get_keypad_key(void)
{

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
        PORTA |= (1 << row); 
    }
    
    return '\0'; // No key pressed
}

void keypad_scan(void)
{
    char key = get_keypad_key();
    if( key == '\0' ) return;
    if(key == '#') // Check if the '#' key is pressed , for entering password
    {
        if(master_login_period) // If in master login period
        {
            handle_master_login();
            return;
        } // master login code completed

        if(armed_status == NOT_ARMED) 
        {
            // Handle disarmed state
            open_lock(); // open the lock
            return;
        }
        if (strcmp(entered_password, password) == 0) // Compare entered password with the correct one
        {
            // Password is correct, perform action
            lcd_write_at_first_line("Access Granted");
            _delay_ms(1000); // Keep the message on for 1 second
            open_lock();
            _delay_ms(1000);
            is_authorized = 1; // Set the authorized flag
            incorrect_attempts = 0; // Reset incorrect attempts counter
        }
        else
        {
            incorrect_attempts++; // Increment the incorrect attempts counter
            if(incorrect_attempts >= 3) {            // Password is incorrect, handle accordingly
                lcd_write_at_first_line("System Blocked");
                send_sms_multiple_attempts(); // Send SMS for multiple incorrect attempts
                incorrect_attempts = 0; // Reset attempts after showing message
                is_blocked = 1; // Set the blocked flag

                //beep(); // Call the beep function to indicate blocking
                turn_on_buzzer(); // Turn on an Buzzer connected to PB0
                return;
            }
            else {
                lcd_write_at_first_line("Access Denied");
                _delay_ms(1000); // Keep the LED on for 1 second
                lcd_write_at_first_line("Enter PIN");
            }

        }
        entered_password[0] = '\0'; // Reset entered password after checking
        password_length = 0; // Reset password length
    }
    else if (key == '*') // Check if the '*' key is pressed to reset the entered password
    {
        if(master_login_period)
        {
            master_entered_password[0] = '\0'; // Reset entered password
            master_password_length = 0; // Reset password length    
        }
        else 
        {
            entered_password[0] = '\0'; // Reset entered password
            password_length = 0; // Reset password length
        }
        lcd_write_at_second_line("                ");
    }
    else if(key == 'A') // used to close the door if it is opened
    {
    }
    else if(key == 'B') // enable system from blocked state by only master
    {
        if(is_blocked && is_master_logged_in)
        {
            turn_off_buzzer();
			PORTB |= (1 << PB5);
            PORTB |= (1 << PB3);
            PORTB |= (1 << PB4);
            is_blocked = 0; // Reset the blocked flag
            lcd_write_at_first_line("System Enabled");
            _delay_ms(500);
            lcd_write_at_first_line("Master access");
            return;            
        }
        // Handle 'B' key press, for example, you can implement a specific action
    }
    else if(key == 'C') // for mode change
    {
        if(is_master_logged_in)
        {
            mode_change_initialize();
        }
		
    }
    else if(key == 'D') // for master login and log out
    {
        if(is_master_logged_in)
        {
            is_master_logged_in = 0; // Reset the master login flag
            lcd_write_at_first_line("Normal mode");
            _delay_ms(1000); // Keep the message on for 1 second
            write_to_lcd_according_to_mode();
            return; // Exit the function after logging out
        }
        if(!master_login_period)
        {
            master_login_period = 1; // Set the master login period flag
            lcd_write_at_first_line("Enter Master PIN");
            master_entered_password[0] = '\0'; // Reset entered master password
            master_password_length = 0; // Reset master password length
            return;
        }
        if(master_login_period)
        {
            master_login_period = 0; // Reset the master login period flag
            lcd_write_at_first_line("Master Login Ended");
            _delay_ms(250); // Keep the message on for 1 second
            lcd_write_at_first_line("Enter PIN");
            master_entered_password[0] = '\0'; // Reset entered master password
            master_password_length = 0; // Reset master password length
            return; // Exit the function after ending master login
        }
    }
    else
    {
        if(mode_change_period)
        {
            change_mode(key); // Change the mode if in mode change period
        }
        // Append the pressed key to the entered password
        if(master_login_period)
        {
            master_entered_password[master_password_length] = key; // Store the pressed key
            char buffer[17];
            uint8_t star_size = master_password_length;
            if(star_size > 0){
                for(uint8_t i = 0; i < star_size; i++) {
                    buffer[i] = '*';
                }
            }
            buffer[star_size] = key;
            buffer[star_size + 1] = '\0';
            lcd_write_at_second_line(buffer); // Display the entered master password on the LCD
            _delay_ms(250);
            buffer[star_size] = '*';
            lcd_write_at_second_line(buffer); // Display the entered master password as stars on the LCD
            if (master_password_length < MAX_PASSWORD_LENGTH) 
            {
                master_password_length++; // Increment the master password length
            }
            master_entered_password[master_password_length] = '\0'; // Null-terminate the string
            return;
        }
        // for normal users
        if(is_blocked || is_master_logged_in) return; // If the system is blocked or master is logged in, do not accept any input from normal userss
        entered_password[password_length] = key; // Store the pressed key
        char buffer[17]; 
        uint8_t star_size = password_length;
        if(star_size > 0){
            for(uint8_t i = 0; i < star_size; i++) {
                buffer[i] = '*';
            }
        }
        buffer[star_size] = key;
        buffer[star_size + 1] = '\0';
        lcd_write_at_second_line(buffer); // Display the entered password on the LCD
        _delay_ms(250);

        buffer[star_size] = '*';
        lcd_write_at_second_line(buffer); // Display the entered password as stars on the LCD
        if (password_length < MAX_PASSWORD_LENGTH) 
        {
            password_length++; // Increment the password length
        }
        entered_password[password_length] = '\0'; // Null-terminate the string
    }
}


int main(void)
{
    reed_switch_enable();
    rfid_init();
    keypad_init(); // Initialize the keypad
    //PORTB &= ~(1 << PB1); // Set PB1 low to close the lock initially
    PORTB |= (1 << PB1); // Set PB1 high to close the lock initially
    PORTB |= (1 << PB2);
    PORTB |= (1 << PB3);
    PORTB |= (1 << PB4);
	PORTB |= (1 << PB5);
    LCD_DDR_DATA |= 0xF0; // D4-D7 ouotput
    PORTD |= (1 << PD3); // Enable pull-up resistor on PD3
    LCD_DDR_CTRL |= (1 << RS) | (1 << E); // PC6, PC7 output
    lcd_init();
    lcd_write_at_first_line("Press # to open");
    lcd_write_at_second_line("the lock");
	_delay_ms(1000);
    sei(); // Enable global interrupts
    while (1) 
    {
        keypad_scan(); // Scan the keypad for key presses
        _delay_ms(100); // Add a small delay to debounce the keys
        if(is_close_lock)
        {
            is_authorized = 0; // Reset the authorized flag
            is_close_lock = 0; // Reset the close lock flag
            rfid_flag = 1;
            _delay_ms(5000);
        }
    }
}

