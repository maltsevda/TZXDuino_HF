#include "Display.h"
#include <LiquidCrystal.h>
#include "Config.h"
#include "Texts.h"  // STR_EMPTY

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Public Functions

bool setupDisplay()
{
    lcd.begin(16, 2);
    lcd.clear();
}

size_t printAt(const __FlashStringHelper *sz, uint8_t col, uint8_t row)
{
    lcd.setCursor(col, row);
    return lcd.print(sz);
}

size_t printLine(const __FlashStringHelper *sz, uint8_t row)
{
    lcd.setCursor(0, row);
    return lcd.print(sz);
}

size_t printLine(const char* sz, uint8_t row)
{
    // clear line
    lcd.setCursor(0, row);
    lcd.print(STR_EMPTY);
    // print line
    lcd.setCursor(0, row);
    return lcd.print(sz);
}

void printCounter(unsigned int value)
{
    lcd.setCursor(13, 0);
    if (value > 999)
        lcd.print(F("999"));
    else
    {
        if (value < 10)
            lcd.print("00");
        else if (value < 100)
            lcd.print('0');
        lcd.print(value);
    }
}

void printPercentages(uint8_t value)
{
    lcd.setCursor(8, 0);
    lcd.print(value);
    lcd.print('%');
}

void printError(const __FlashStringHelper *sz)
{
    printLine(STR_ERROR, 0);
    printLine(sz, 1);
}
