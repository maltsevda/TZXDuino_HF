#ifndef __SOUND_H__
#define __SOUND_H__

#include <Arduino.h>

// defines

#define SND_BUFFSIZE    64

// interface

void sound(uint8_t val);

void setupSound();
void startSound(unsigned long microseconds);
void stopSound();

// TODO: direct access to variables

extern volatile word wbuffer[SND_BUFFSIZE + 1][2];
extern volatile byte morebuff;
extern volatile byte workingBuffer;
extern volatile byte isStopped;
extern volatile byte ID15switch;
extern volatile word TstatesperSample;

#endif // __SOUND_H__