#include "Sound.h"
#include "Config.h"
#include <TimerOne.h>

// ISR Variables
volatile word soundBuffers[SND_BUFFSIZE][2] = { 0 };
volatile byte currentPosition = 0;  // 0...SND_BUFFSIZE-1
volatile byte currentBuffer = 0;    // 0...1
volatile byte isBufferSwapped = HIGH;
volatile byte isSoundPaused = HIGH;
volatile byte pinState = LOW;
volatile bool isPauseBlock = false;
volatile bool wasPauseBlock = false;
// ID15 switch
volatile byte ID15switch = 0;
volatile word tStatesPerSample = 0;
// This variable can be changed in menu (not realized yet)
const bool flipPolarity = false;

// Private Functions

void soundISR();

// Common sound functions

void sound(uint8_t val)
{
    digitalWrite(SND_SPEAKER, val);
    digitalWrite(SND_OUTPUT, val);
}

// Sound Timer

void setupSound()
{
    pinState = LOW;
    isSoundPaused = HIGH;

    // setup sound
    pinMode(SND_SPEAKER, OUTPUT);
    pinMode(SND_OUTPUT, OUTPUT);
    sound(pinState);

    // 100ms pause prevents anything bad happening before we're ready
    Timer1.initialize(100000);
    Timer1.attachInterrupt(soundISR);
    //Stop the timer until we're ready
    Timer1.stop();
}

void startSound(unsigned long microseconds)
{
    pinState = LOW; //Always Start on a LOW output for simplicity
    isSoundPaused = LOW;
    // clear sound buffer
    for (int i = 0; i < SND_BUFFSIZE; ++i)
    {
        soundBuffers[i][0] = 0;
        soundBuffers[i][1] = 0;
    }
    // set PIN and timer
    sound(pinState);
    Timer1.setPeriod(microseconds);
}

void pauseSound(byte pause)
{
    isSoundPaused = pause;
}

void stopSound()
{
    Timer1.stop();
    isSoundPaused = HIGH;
    ID15switch = 0; // ID15switch
}

void setPeriod(byte pos, word period)
{
    soundBuffers[pos][currentBuffer ^ 1] = period; //add period to the buffer
}

bool checkIfBufferSwapped()
{
    if (isBufferSwapped == HIGH)
    {
        isBufferSwapped = LOW;
        return true;
    }
    return false;
}

// ID 15 - Direct Recording

void setID15()
{
    ID15switch = 1;
}

void setTStates(word value)
{
    tStatesPerSample = value;
}

word getTStates()
{
    return tStatesPerSample;
}

//
// Sound Timer ISR
//

void soundISR()
{
    word workingPeriod = soundBuffers[currentPosition][currentBuffer];
    byte pauseFlipBit = false;
    unsigned long newTime = 1;

    if (isSoundPaused == LOW && workingPeriod >= 1)
    {
        if (bitRead(workingPeriod, 15))
        {
            //If bit 15 of the current period is set we're about to run a pause
            //Pauses start with a 1.5ms where the output is untouched after which the output is set LOW
            //Pause block periods are stored in milliseconds not microseconds
            isPauseBlock = true;
            bitClear(workingPeriod, 15); //Clear pause block flag
            pinState = !pinState;
            pauseFlipBit = true;
            wasPauseBlock = true;
        }
        else
        {
            if (workingPeriod >= 1 && wasPauseBlock == false)
            {
                pinState = !pinState;
            }
            else if (wasPauseBlock == true && isPauseBlock == false)
            {
                wasPauseBlock = false;
            }
        }

        if (ID15switch == 1)
        {
            if (bitRead(workingPeriod, 14) == 0)
            {
                sound(pinState);
            }
            else
            {
                if (bitRead(workingPeriod, 13) == 0)
                    sound(LOW);
                else
                {
                    sound(HIGH);
                    bitClear(workingPeriod, 13);
                }
                bitClear(workingPeriod, 14); //Clear ID15 flag
                workingPeriod = tStatesPerSample;
            }
        }
        else
        {
            sound(pinState);
        }

        if (pauseFlipBit == true)
        {
            newTime = 1500; //Set 1.5ms initial pause block
            pinState = flipPolarity ? HIGH : LOW;
            soundBuffers[currentPosition][currentBuffer] = workingPeriod - 1; //reduce pause by 1ms as we've already pause for 1.5ms
            pauseFlipBit = false;
        }
        else
        {
            if (isPauseBlock == true)
            {
                newTime = long(workingPeriod) * 1000; //Set pause length in microseconds
                isPauseBlock = false;
            }
            else
            {
                newTime = workingPeriod; //After all that, if it's not a pause block set the pulse period
            }
            currentPosition += 1;
            if (currentPosition >= SND_BUFFSIZE) //Swap buffer pages if we've reached the end
            {
                currentPosition = 0;
                currentBuffer ^= 1;
                isBufferSwapped = HIGH; //Request more data to fill inactive page
            }
        }
    }
    else if (isSoundPaused == LOW && workingPeriod <= 1)
    {
        newTime = 1000; //Just in case we have a 0 in the buffer
        currentPosition += 1;
        if (currentPosition >= SND_BUFFSIZE)
        {
            currentPosition = 0;
            currentBuffer ^= 1;
            isBufferSwapped = HIGH;
        }
    }
    else
    {
        newTime = 1000000; //Just in case we have a 0 in the buffer
    }
    Timer1.setPeriod(newTime + 4); //Finally set the next pulse length
}
