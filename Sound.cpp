#include "Sound.h"
#include "Config.h"
#include <TimerOne.h>

// ISR Variables
volatile byte pos = 0;
volatile word wbuffer[SND_BUFFSIZE][2] = { 0 };
volatile byte morebuff = HIGH;
volatile byte workingBuffer = 0;
volatile byte isStopped = false;
volatile byte pinState = LOW;
volatile byte isPauseBlock = false;
volatile byte wasPauseBlock = false;
// ID15 switch
volatile byte ID15switch = 0;
volatile word TstatesperSample = 0;
// This variable can be changed in menu (not realized yet)
const bool flipPolarity = false;

// Private Functions

void soundISR();

// Common sound functions

void sound(uint8_t val)
{
    digitalWrite(SND_OUTPUT, val);
}

// Sound Timer

void setupSound()
{
    pinState = LOW;
    isStopped = true;

    // setup sound
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
    isStopped = false;
    // clear sound buffer
    for (int i = 0; i < SND_BUFFSIZE; ++i)
    {
        wbuffer[i][0] = 0;
        wbuffer[i][1] = 0;
    }
    // set PIN and timer
    sound(pinState);
    Timer1.setPeriod(microseconds);
}

void stopSound()
{
    Timer1.stop();
    isStopped = true;
    ID15switch = 0; // ID15switch
}

void soundISR()
{
    //ISR Output routine
    //unsigned long fudgeTime = micros();         //fudgeTime is used to reduce length of the next period by the time taken to process the ISR
    word workingPeriod = wbuffer[pos][workingBuffer];
    byte pauseFlipBit = false;
    unsigned long newTime = 1;
    if (isStopped == 0 && workingPeriod >= 1)
    {
        if bitRead (workingPeriod, 15)
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
            //if (wasPauseBlock==true && isPauseBlock==false) wasPauseBlock=false;
        }

        if (ID15switch == 1)
        {
            if (bitRead(workingPeriod, 14) == 0)
            {
                //pinState = !pinState;
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
                workingPeriod = TstatesperSample;
            }
        }
        else
        {
            //pinState = !pinState;
            sound(pinState);
        }

        if (pauseFlipBit == true)
        {
            newTime = 1500; //Set 1.5ms initial pause block
            pinState = flipPolarity ? HIGH : LOW;
            wbuffer[pos][workingBuffer] = workingPeriod - 1; //reduce pause by 1ms as we've already pause for 1.5ms
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
            pos += 1;
            if (pos >= SND_BUFFSIZE) //Swap buffer pages if we've reached the end
            {
                pos = 0;
                workingBuffer ^= 1;
                morebuff = HIGH; //Request more data to fill inactive page
            }
        }
    }
    else if (workingPeriod <= 1 && isStopped == 0)
    {
        newTime = 1000; //Just in case we have a 0 in the buffer
        pos += 1;
        if (pos >= SND_BUFFSIZE)
        {
            pos = 0;
            workingBuffer ^= 1;
            morebuff = HIGH;
        }
    }
    else
    {
        newTime = 1000000; //Just in case we have a 0 in the buffer
    }
    Timer1.setPeriod(newTime + 4); //Finally set the next pulse length
}
