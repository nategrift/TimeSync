/**
 * Driver for liquid crystal arduino library
 * @source https://github.com/maxsydney/ESP32-HD44780
 * 
 * This is a basic port of the liquid crystal arduino library for driving an LCD 
 * using the HD44780 driver and PC8574 I2C I/O expander.
*/
#ifndef HD44780_H
#define HD44780_H

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations or any other declarations that need to be shared.
void LCD_init(uint8_t addr, uint8_t dataPin, uint8_t clockPin, uint8_t cols, uint8_t rows);
void LCD_setCursor(uint8_t col, uint8_t row);
void LCD_writeChar(char c);
void LCD_writeStr(char* str);
void LCD_home(void);
void LCD_clearScreen(void);

#ifdef __cplusplus
}
#endif

#endif // HD44780_H