#ifndef __SOUND_H__
#define __SOUND_H__

#include <Arduino.h>

// defines

#define SND_BUFFSIZE    64

// interface

void sound(uint8_t val);

void setupSound();
void startSound(unsigned long microseconds);
void pauseSound(byte pause);
void stopSound();

void setPeriod(byte pos, word period);
bool checkIfBufferSwapped();

// ID 15 - Direct Recording

void setID15();
void setTStates(word value);
word getTStates();

#endif // __SOUND_H__