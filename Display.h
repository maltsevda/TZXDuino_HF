#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include <Arduino.h>

bool setupDisplay();

size_t printAt(const __FlashStringHelper *sz, uint8_t col, uint8_t row);
size_t printLine(const __FlashStringHelper *sz, uint8_t row);
size_t printLine(const char* sz, uint8_t row);
void printCounter(unsigned int value);
void printPercentages(uint8_t value);
void printError(const __FlashStringHelper *sz);

#endif // __DISPLAY_H__