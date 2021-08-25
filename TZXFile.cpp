#include "Config.h"
#include "TZXFile.h"
#include "Texts.h"
#include "SDCard.h"
#include "Sound.h"

PROGMEM const char TZXTape[7] = {'Z', 'X', 'T', 'a', 'p', 'e', '!'};
PROGMEM const char TAPcheck[7] = {'T', 'A', 'P', 't', 'a', 'p', '.'};
PROGMEM const char ZX81Filename[9] = {'T', 'Z', 'X', 'D', 'U', 'I', 'N', 'O', 0x9D};
PROGMEM const char AYFile[8] = {'Z', 'X', 'A', 'Y', 'E', 'M', 'U', 'L'};                                                                  // added additional AY file header check
PROGMEM const char TAPHdr[20] = {0x0, 0x0, 0x3, 'Z', 'X', 'A', 'Y', 'F', 'i', 'l', 'e', ' ', ' ', 0x1A, 0xB, 0x0, 0xC0, 0x0, 0x80, 0x6E}; //
//const char TAPHdr[24] = {0x13,0x0,0x0,0x3,' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',0x1A,0xB,0x0,0xC0,0x0,0x80,0x52,0x1C,0xB,0xFF};
PROGMEM const char UEFFile[9] = {'U', 'E', 'F', ' ', 'F', 'i', 'l', 'e', '!'};

//Keep track of which ID, Task, and Block Task we're dealing with
byte currentID = 0;
byte currentTask = 0;
byte currentBlockTask = 0;

//Temporarily store for a pulse period before loading it into the buffer.
word currentPeriod = 1;

//Main Variables
byte AYPASS = 0;
byte hdrptr = 0;
byte blkchksum = 0;
word ayblklen = 0;
byte btemppos = 0;
byte copybuff = LOW;
unsigned long bytesToRead = 0;
byte pulsesCountByte = 0;
word pilotPulses = 0;
word pilotLength = 0;
word sync1Length = 0;
word sync2Length = 0;
word zeroPulse = 0;
word onePulse = 0;
byte usedBitsInLastByte = 8;
word loopCount = 0;
byte seqPulses = 0;
byte input[11];

byte forcePause0 = 0;
byte firstBlockPause = false;
unsigned long loopStart = 0;
word pauseLength = 0;
word temppause = 0;
byte count = 128;
volatile byte currentBit = 0;
volatile byte currentByte = 0;
volatile byte currentChar = 0;
byte pass = 0;
unsigned long debugCount = 0;
byte EndOfFile = false;
byte lastByte;

int TSXspeedup = 1;
int BAUDRATE = 1200;

word chunkID = 0;
byte uefTurboMode = 0;
float outFloat;
byte UEFPASS = 0;
byte passforZero = 2;
byte passforOne = 4;

byte PauseAtStart = false;

byte wibble = 1;
byte parity = 0;      //0:NoParity 1:ParityOdd 2:ParityEven (default:0)
byte bitChecksum = 0; // 0:Even 1:Odd number of one bits

//
// TZXProcessing.ino
//

word TickToUs(word ticks)
{
    return (word)((((float)ticks) / 3.5) + 0.5);
}

void checkForEXT()
{
    if (checkFileExt(PSTR(".tap")))
    { //Check for Tap File.  As these have no header we can skip straight to playing data
        currentTask = PROCESSID;
        currentID = TAP;
        if ((readByte()) == 1)
        {
            if (outByte == 0x16)
            {
                currentID = ORIC;
            }
        }
    }
    if (checkFileExt(PSTR(".p")))
    { //Check for P File.  As these have no header we can skip straight to playing data
        currentTask = PROCESSID;
        currentID = ZXP;
    }
    if (checkFileExt(PSTR(".o")))
    { //Check for O File.  As these have no header we can skip straight to playing data
        currentTask = PROCESSID;
        currentID = ZXO;
    }
    if (checkFileExt(PSTR(".ay")))
    { //Check for AY File.  As these have no TAP header we must create it and send AY DATA Block after
        currentTask = GETAYHEADER;
        currentID = AYO;
        AYPASS = 0;        // Reset AY PASS flags
        hdrptr = HDRSTART; // Start reading from position 1 -> 0x13 [0x00]
    }
    if (checkFileExt(PSTR(".uef")))
    { //Check for UEF File.  As these have no TAP header we must create it and send AY DATA Block after
        currentTask = GETUEFHEADER;
        currentID = UEF;
    }
}

void TZXPlay()
{
    stopSound();
    if (openFile())
    { //open file and check for errors
        printError(STR_ERR_OPEN_FILE);
    }
    bytesRead = 0;               //start of file
    currentTask = GETFILEHEADER; //First task: search for header
    checkForEXT();
    currentBlockTask = READPARAM; //First block task is to read in parameters
    count = 255;    //End of file buffer flush
    EndOfFile = false;
    //set 1ms wait at start of a file.
    startSound(1000);
}

void TZXStop()
{
    stopSound();
    closeFile();

    bytesRead = 0;  // reset read bytes PlayBytes
    blkchksum = 0;  // reset block chksum byte for AY loading routine
    AYPASS = 0;     // reset AY flag
}

bool TZXLoop()
{
    noInterrupts(); //Pause interrupts to prevent var reads and copy values out
    copybuff = morebuff;
    morebuff = LOW;
    isStopped = isFileStopped();
    interrupts();
    if (copybuff == HIGH)
    {
        btemppos = 0; //Buffer has swapped, start from the beginning of the new page
        copybuff = LOW;
    }

    if (btemppos < SND_BUFFSIZE) // Keep filling until full
    {
        TZXProcess(); //generate the next period to add to the buffer
        if (currentPeriod > 0)
        {
            noInterrupts();                                       //Pause interrupts while we add a period to the buffer
            wbuffer[btemppos][workingBuffer ^ 1] = currentPeriod; //add period to the buffer
            interrupts();
            btemppos += 1;
        }
        return false;
    }

    return true;
}

void TZXProcess()
{
    byte r = 0;
    currentPeriod = 0;
    if (currentTask == GETFILEHEADER)
    {
        //grab 7 byte string
        ReadTZXHeader();
        //set current task to GETID
        currentTask = GETID;
    }
    if (currentTask == GETAYHEADER)
    {
        //grab 8 byte string
        ReadAYHeader();
        //set current task to PROCESSID
        currentTask = PROCESSID;
    }
    if (currentTask == GETUEFHEADER)
    {
        //grab 12 byte string
        ReadUEFHeader();
        //set current task to GETCHUNKID
        currentTask = GETCHUNKID;
    }
    if (currentTask == GETCHUNKID)
    {

        if (r = readWord() == 2)
        {
            chunkID = outWord;
            if (r = readDword() == 4)
            {
                bytesToRead = outLong;
                parity = 0;

                if (chunkID == ID0104)
                {
                    //bytesRead+= 3;
                    bytesToRead += -3;
                    bytesRead += 1;
                    //grab 1 byte Parity
                    if (readByte() == 1)
                    {
                        if (outByte == 'O')
                            parity = wibble ? 2 : 1;
                        else if (outByte == 'E')
                            parity = wibble ? 1 : 2;
                        else
                            parity = 0; // 'N'
                    }
                    bytesRead += 1;
                }
            }
            else
            {
                chunkID = IDCHUNKEOF;
            }
        }
        else
        {
            chunkID = IDCHUNKEOF;
        }
        if (!(uefTurboMode))
        {
            zeroPulse = UEFZEROPULSE;
            onePulse = UEFONEPULSE;
        }
        else
        {
            zeroPulse = UEFTURBOZEROPULSE;
            onePulse = UEFTURBOONEPULSE;
        }
        lastByte = 0;

        //reset data block values
        currentBit = 0;
        pass = 0;
        //set current task to PROCESSCHUNKID
        currentTask = PROCESSCHUNKID;
        currentBlockTask = READPARAM;
        UEFPASS = 0;
    }
    if (currentTask == PROCESSCHUNKID)
    {
        //CHUNKID Processing

        switch (chunkID)
        {
        case ID0000:
            bytesRead += bytesToRead;
            currentTask = GETCHUNKID;
            break;

        case ID0100:

            //bytesRead+=bytesToRead;
            writeUEFData();
            break;

        case ID0104:
            //parity = 1; // ParityOdd i.e complete with value to get Odd number of ones
            /* stopBits = */ //stopBitPulses = 1;
            writeUEFData();
            //bytesRead+=bytesToRead;
            break;

        case ID0110:
            if (currentBlockTask == READPARAM)
            {
                if (r = readWord() == 2)
                {
                    if (!(uefTurboMode))
                    {
                        pilotPulses = UEFPILOTPULSES;
                        pilotLength = UEFPILOTLENGTH;
                    }
                    else
                    {
                        // turbo mode
                        pilotPulses = UEFTURBOPILOTPULSES;
                        pilotLength = UEFTURBOPILOTLENGTH;
                    }
                }
                currentBlockTask = PILOT;
            }
            else
            {
                UEFCarrierToneBlock();
            }
            //bytesRead+=bytesToRead;
            //currentTask = GETCHUNKID;
            break;

        case ID0111:
            if (currentBlockTask == READPARAM)
            {
                if (r = readWord() == 2)
                {
                    pilotPulses = UEFPILOTPULSES; // for TURBOBAUD1500 is outWord<<2
                    pilotLength = UEFPILOTLENGTH;
                }
                currentBlockTask = PILOT;
                UEFPASS += 1;
            }
            else if (UEFPASS == 1)
            {
                UEFCarrierToneBlock();
                if (pilotPulses == 0)
                {
                    currentTask = PROCESSCHUNKID;
                    currentByte = 0xAA;
                    lastByte = 1;
                    currentBit = 10;
                    pass = 0;
                    UEFPASS = 2;
                }
            }
            else if (UEFPASS == 2)
            {
                parity = 0; // NoParity
                writeUEFData();
                if (currentBit == 0)
                {
                    currentTask = PROCESSCHUNKID;
                    currentBlockTask = READPARAM;
                }
            }
            else if (UEFPASS == 3)
            {
                UEFCarrierToneBlock();
            }
            break;

        case ID0112:
            //if(currentBlockTask==READPARAM){
            if (r = readWord() == 2)
            {
                if (outWord > 0)
                {
                    //Serial.print(F("delay="));
                    //Serial.println(outWord,DEC);
                    temppause = outWord;

                    currentID = IDPAUSE;
                    currentPeriod = temppause;
                    bitSet(currentPeriod, 15);
                    currentTask = GETCHUNKID;
                }
                else
                {
                    currentTask = GETCHUNKID;
                }
            }
            //}
            break;

        case ID0114:
            if (r = readWord() == 2)
            {
                pilotPulses = UEFPILOTPULSES;
                //pilotLength = UEFPILOTLENGTH;
                bytesRead -= 2;
            }
            UEFCarrierToneBlock();
            bytesRead += bytesToRead;
            currentTask = GETCHUNKID;
            break;

        case ID0116:
            //if(currentBlockTask==READPARAM){
            if (r = readDword() == 4)
            {
                byte *FloatB = (byte *)&outLong;
                outWord = (((*(FloatB + 2) & 0x80) >> 7) | (*(FloatB + 3) & 0x7f) << 1) + 10;
                outWord = *FloatB | (*(FloatB + 1)) << 8 | ((outWord & 1) << 7) << 16 | (outWord >> 1) << 24;
                outFloat = *((float *)&outWord);
                outWord = (int)outFloat;

                if (outWord > 0)
                {
                    //Serial.print(F("delay="));
                    //Serial.println(outWord,DEC);
                    temppause = outWord;

                    currentID = IDPAUSE;
                    currentPeriod = temppause;
                    bitSet(currentPeriod, 15);
                    currentTask = GETCHUNKID;
                }
                else
                {
                    currentTask = GETCHUNKID;
                }
            }
            //}
            break;

        case ID0117:
            if (r = readWord() == 2)
            {
                if (outWord == 300)
                {
                    passforZero = 8;
                    passforOne = 16;
                    currentTask = GETCHUNKID;
                }
                else
                {
                    passforZero = 2;
                    passforOne = 4;
                    currentTask = GETCHUNKID;
                }
            }
            break;

        case IDCHUNKEOF:
            if (!count == 0)
            {
                //currentPeriod = 32767;
                currentPeriod = 10;
                bitSet(currentPeriod, 15); //bitSet(currentPeriod, 12);
                count += -1;
            }
            else
            {
                bytesRead += bytesToRead;
                stopFile();
                return;
            }
            break;

        default:
            bytesRead += bytesToRead;
            currentTask = GETCHUNKID;
            break;
        }
    }

    if (currentTask == GETID)
    {
        //grab 1 byte ID
        if (readByte() == 1)
        {
            currentID = outByte;
        }
        else
        {
            currentID = EOF;
        }
        //reset data block values
        currentBit = 0;
        pass = 0;
        //set current task to PROCESSID
        currentTask = PROCESSID;
        currentBlockTask = READPARAM;
    }
    if (currentTask == PROCESSID)
    {
        //ID Processing
        switch (currentID)
        {
        case ID10:
            //Process ID10 - Standard Block
            switch (currentBlockTask)
            {
            case READPARAM:
                if (r = readWord() == 2)
                {
                    pauseLength = outWord;
                }
                if (r = readWord() == 2)
                {
                    bytesToRead = outWord + 1;
                }
                if (r = readByte() == 1)
                {
                    if (outByte == 0)
                    {
                        pilotPulses = PILOTNUMBERL;
                    }
                    else
                    {
                        pilotPulses = PILOTNUMBERH;
                    }
                    bytesRead += -1;
                }
                pilotLength = PILOTLENGTH;
                sync1Length = SYNCFIRST;
                sync2Length = SYNCSECOND;
                zeroPulse = ZEROPULSE;
                onePulse = ONEPULSE;
                currentBlockTask = PILOT;
                usedBitsInLastByte = 8;
                break;

            default:
                StandardBlock();
                break;
            }

            break;

        case ID11:
            //Process ID11 - Turbo Tape Block
            switch (currentBlockTask)
            {
            case READPARAM:
                if (r = readWord() == 2)
                {
                    pilotLength = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    sync1Length = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    sync2Length = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    zeroPulse = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    onePulse = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    pilotPulses = outWord;
                }
                if (r = readByte() == 1)
                {
                    usedBitsInLastByte = outByte;
                }
                if (r = readWord() == 2)
                {
                    pauseLength = outWord;
                }
                if (r = readLong() == 3)
                {
                    bytesToRead = outLong + 1;
                }
                currentBlockTask = PILOT;
                break;

            default:
                StandardBlock();
                break;
            }

            break;
        case ID12:
            //Process ID12 - Pure Tone Block
            if (currentBlockTask == READPARAM)
            {
                if (r = readWord() == 2)
                {
                    pilotLength = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    pilotPulses = outWord;
                    //DebugBlock("Pilot Pulses", pilotPulses);
                }
                currentBlockTask = PILOT;
            }
            else
            {
                PureToneBlock();
            }
            break;

        case ID13:
            //Process ID13 - Sequence of Pulses
            if (currentBlockTask == READPARAM)
            {
                if (r = readByte() == 1)
                {
                    seqPulses = outByte;
                }
                currentBlockTask = DATA;
            }
            else
            {
                PulseSequenceBlock();
            }
            break;

        case ID14:
            //process ID14 - Pure Data Block
            if (currentBlockTask == READPARAM)
            {
                if (r = readWord() == 2)
                {
                    zeroPulse = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    onePulse = TickToUs(outWord);
                }
                if (r = readByte() == 1)
                {
                    usedBitsInLastByte = outByte;
                }
                if (r = readWord() == 2)
                {
                    pauseLength = outWord;
                }
                if (r = readLong() == 3)
                {
                    bytesToRead = outLong + 1;
                }
                currentBlockTask = DATA;
            }
            else
            {
                PureDataBlock();
            }
            break;

        case ID15:
            //process ID15 - Direct Recording
            if (currentBlockTask == READPARAM)
            {
                if (r = readWord() == 2)
                {
                    //Number of T-states per sample (bit of data) 79 or 158 - 22.6757uS for 44.1KHz
                    TstatesperSample = TickToUs(outWord);
                }
                if (r = readWord() == 2)
                {
                    //Pause after this block in milliseconds
                    pauseLength = outWord;
                }
                if (r = readByte() == 1)
                {
                    //Used bits in last byte (other bits should be 0)
                    usedBitsInLastByte = outByte;
                }
                if (r = readLong() == 3)
                {
                    // Length of samples' data
                    bytesToRead = outLong + 1;
                }
                currentBlockTask = DATA;
            }
            else
            {
                currentPeriod = TstatesperSample;
                bitSet(currentPeriod, 14);
                DirectRecording();
            }
            break;

        case ID19:
            //Process ID19 - Generalized data block
            switch (currentBlockTask)
            {
            case READPARAM:

                if (r = readDword() == 4)
                {
                    //bytesToRead = outLong;
                }
                if (r = readWord() == 2)
                {
                    //Pause after this block in milliseconds
                    pauseLength = outWord;
                }
                bytesRead += 86; // skip until DataStream filename
                //bytesToRead += -88 ;    // pauseLength + SYMDEFs
                currentBlockTask = DATA;
                break;

            case DATA:

                ZX8081DataBlock();

                break;
            }
            break;

        case ID20:
            //process ID20 - Pause Block
            if (r = readWord() == 2)
            {
                if (outWord > 0)
                {
                    temppause = outWord;
                    currentID = IDPAUSE;
                }
                else
                {
                    currentTask = GETID;
                }
            }
            break;

        case ID21:
            //Process ID21 - Group Start
            if (r = readByte() == 1)
            {
                bytesRead += outByte;
            }
            currentTask = GETID;
            break;

        case ID22:
            //Process ID22 - Group End
            currentTask = GETID;
            break;

        case ID24:
            //Process ID24 - Loop Start
            if (r = readWord() == 2)
            {
                loopCount = outWord;
                loopStart = bytesRead;
            }
            currentTask = GETID;
            break;

        case ID25:
            //Process ID25 - Loop End
            loopCount += -1;
            if (loopCount != 0)
            {
                bytesRead = loopStart;
            }
            currentTask = GETID;
            break;

        case ID2A:
            //Skip//
            bytesRead += 4;
            currentTask = GETID;
            break;

        case ID2B:
            //Skip//
            bytesRead += 5;
            currentTask = GETID;
            break;

        case ID30:
            //Process ID30 - Text Description
            if (r = readByte() == 1)
            {
                //Show info on screen - removed until bigger screen used
                //byte j = outByte;
                //for(byte i=0; i<j; i++) {
                //  if(readByte()==1) {
                //    lcd.print(char(outByte));
                //  }
                //}
                bytesRead += outByte;
            }
            currentTask = GETID;
            break;

        case ID31:
            //Process ID31 - Message block
            if (r = readByte() == 1)
            {
                // dispayTime = outByte;
            }
            if (r = readByte() == 1)
            {
                bytesRead += outByte;
            }
            currentTask = GETID;
            break;

        case ID32:
            //Process ID32 - Archive Info
            //Block Skipped until larger screen used
            if (readWord() == 2)
            {
                bytesRead += outWord;
            }
            currentTask = GETID;
            break;

        case ID33:
            //Process ID32 - Archive Info
            //Block Skipped until larger screen used
            if (readByte() == 1)
            {
                bytesRead += (long(outByte) * 3);
            }
            currentTask = GETID;
            break;

        case ID35:
            //Process ID35 - Custom Info Block
            //Block Skipped
            bytesRead += 0x10;
            if (r = readDword() == 4)
            {
                bytesRead += outLong;
            }
            currentTask = GETID;
            break;

        case ID4B:
            //Process ID4B - Kansas City Block (MSX specific implementation only)
            switch (currentBlockTask)
            {
            case READPARAM:
                if (r = readDword() == 4)
                { // Data size to read
                    bytesToRead = outLong - 12;
                }
                if (r = readWord() == 2)
                { // Pause after block in ms
                    pauseLength = outWord;
                }
                if (TSXspeedup == 0)
                {
                    if (r = readWord() == 2)
                    { // T-states each pilot pulse
                        pilotLength = TickToUs(outWord);
                    }
                    if (r = readWord() == 2)
                    { // Number of pilot pulses
                        pilotPulses = outWord;
                    }
                    if (r = readWord() == 2)
                    { // T-states 0 bit pulse
                        zeroPulse = TickToUs(outWord);
                    }
                    if (r = readWord() == 2)
                    { // T-states 1 bit pulse
                        onePulse = TickToUs(outWord);
                    }
                    readWord();
                }
                else
                {
                    //Fixed speedup baudrate, reduced pilot duration
                    pilotPulses = 10000;
                    bytesRead += 10;
                    switch (BAUDRATE)
                    {

                    case 1200:
                        pilotLength = onePulse = TickToUs(729);
                        zeroPulse = TickToUs(1458);
                        break;

                    case 2400:
                        pilotLength = onePulse = TickToUs(365);
                        zeroPulse = TickToUs(730);
                        break;

                    case 3600:
                        pilotLength = onePulse = TickToUs(243);
                        zeroPulse = TickToUs(486);
                        break;

                    case 3760:
                        pilotLength = onePulse = TickToUs(233);
                        zeroPulse = TickToUs(466);
                        break;
                    }

                } //TSX_SPEEDUP

                currentBlockTask = PILOT;
                break;

            case PILOT:
                //Start with Pilot Pulses
                if (!pilotPulses--)
                {
                    currentBlockTask = DATA;
                }
                else
                {
                    currentPeriod = pilotLength;
                }
                break;

            case DATA:
                //Data playback
                writeData4B();
                break;

            case PAUSE:
                //Close block with a pause
                temppause = pauseLength;
                currentID = IDPAUSE;
                break;
            }
            break;

        case TAP:
            //Pure Tap file block
            switch (currentBlockTask)
            {
            case READPARAM:
                pauseLength = PAUSELENGTH;
                if (r = readWord() == 2)
                {
                    bytesToRead = outWord + 1;
                }
                if (r = readByte() == 1)
                {
                    if (outByte == 0)
                    {
                        pilotPulses = PILOTNUMBERL + 1;
                    }
                    else
                    {
                        pilotPulses = PILOTNUMBERH + 1;
                    }
                    bytesRead += -1;
                }
                pilotLength = PILOTLENGTH;
                sync1Length = SYNCFIRST;
                sync2Length = SYNCSECOND;
                zeroPulse = ZEROPULSE;
                onePulse = ONEPULSE;
                currentBlockTask = PILOT;
                usedBitsInLastByte = 8;
                break;

            default:
                StandardBlock();
                break;
            }
            break;

        case ZXP:
            switch (currentBlockTask)
            {
            case READPARAM:
                pauseLength = PAUSELENGTH * 5;
                currentChar = 0;
                currentBlockTask = PILOT;
                break;

            case PILOT:
                ZX81FilenameBlock();
                break;

            case DATA:
                ZX8081DataBlock();
                break;
            }
            break;

        case ZXO:
            switch (currentBlockTask)
            {
            case READPARAM:
                pauseLength = PAUSELENGTH * 5;
                currentBlockTask = DATA;
                break;

            case DATA:
                ZX8081DataBlock();
                break;
            }
            break;

        case AYO: //AY File - Pure AY file block - no header, must emulate it
            switch (currentBlockTask)
            {
            case READPARAM:
                pauseLength = PAUSELENGTH; // Standard 1 sec pause
                                           // here we must generate the TAP header which in pure AY files is missing.
                                           // This was done with a DOS utility called FILE2TAP which does not work under recent 32bit OSs (only using DOSBOX).
                                           // TAPed AY files begin with a standard 0x13 0x00 header (0x13 bytes to follow) and contain the
                                           // name of the AY file (max 10 bytes) which we will display as "ZXAYFile " followed by the
                                           // length of the block (word), checksum plus 0xFF to indicate next block is DATA.
                                           // 13 00[00 03(5A 58 41 59 46 49 4C 45 2E 49)1A 0B 00 C0 00 80]21<->[1C 0B FF<AYFILE>CHK]
                //if(hdrptr==1) {
                //bytesToRead = 0x13-2; // 0x13 0x0 - TAP Header minus 2 (FLAG and CHKSUM bytes) 17 bytes total
                //}
                if (hdrptr == HDRSTART)
                {
                    //if (!AYPASS) {
                    pilotPulses = PILOTNUMBERL + 1;
                }
                else
                {
                    pilotPulses = PILOTNUMBERH + 1;
                }
                pilotLength = PILOTLENGTH;
                sync1Length = SYNCFIRST;
                sync2Length = SYNCSECOND;
                zeroPulse = ZEROPULSE;
                onePulse = ONEPULSE;
                currentBlockTask = PILOT; // now send pilot, SYNC1, SYNC2 and DATA (writeheader() from String Vector on 1st pass then writeData() on second)
                if (hdrptr == HDRSTART)
                    AYPASS = 1; // Set AY TAP data read flag only if first run
                if (AYPASS == 2)
                { // If we have already sent TAP header
                    blkchksum = 0;
                    bytesRead = 0;
                    bytesToRead = ayblklen + 2; // set length of file to be read plus data byte and CHKSUM (and 2 block LEN bytes)
                    AYPASS = 5;                 // reset flag to read from file and output header 0xFF byte and end chksum
                }
                usedBitsInLastByte = 8;
                break;

            default:
                StandardBlock();
                break;
            }
            break;

        case ORIC:
            //readByte();
            //OricByteWrite();
            switch (currentBlockTask)
            {
            case READPARAM: // currentBit = 0 y count = 255
            case SYNC1:
                if (currentBit > 0)
                    OricBitWrite();
                else
                {
                    //if (count >0) {
                    readByte();
                    currentByte = outByte;
                    currentBit = 11;
                    bitChecksum = 0;
                    lastByte = 0;
                    if (currentByte == 0x16)
                        count--;
                    else
                    {
                        currentBit = 0;
                        currentBlockTask = SYNC2;
                    } //0x24
                    //}
                    //else currentBlockTask=SYNC2;
                }
                break;
            case SYNC2:
                if (currentBit > 0)
                    OricBitWrite();
                else
                {
                    if (count > 0)
                    {
                        currentByte = 0x16;
                        currentBit = 11;
                        bitChecksum = 0;
                        lastByte = 0;
                        count--;
                    }
                    else
                    {
                        count = 1;
                        currentBlockTask = SYNCLAST;
                    } //0x24
                }
                break;

            case SYNCLAST:
                if (currentBit > 0)
                    OricBitWrite();
                else
                {
                    if (count > 0)
                    {
                        currentByte = 0x24;
                        currentBit = 11;
                        bitChecksum = 0;
                        lastByte = 0;
                        count--;
                    }
                    else
                    {
                        count = 9;
                        lastByte = 0;
                        currentBlockTask = HEADER;
                    }
                }
                break;

            case HEADER:
                if (currentBit > 0)
                    OricBitWrite();
                else
                {
                    if (count > 0)
                    {
                        readByte();
                        currentByte = outByte;
                        currentBit = 11;
                        bitChecksum = 0;
                        lastByte = 0;
                        if (count == 5)
                            bytesToRead = 256 * outByte;
                        else if (count == 4)
                            bytesToRead += (outByte + 1);
                        else if (count == 3)
                            bytesToRead -= (256 * outByte);
                        else if (count == 2)
                            bytesToRead -= outByte;
                        count--;
                    }
                    else
                        currentBlockTask = NAME;
                }
                break;

            case NAME:
                if (currentBit > 0)
                    OricBitWrite();
                else
                {
                    readByte();
                    currentByte = outByte;
                    currentBit = 11;
                    bitChecksum = 0;
                    lastByte = 0;
                    if (currentByte == 0x00)
                    {
                        count = 1;
                        currentBit = 0;
                        currentBlockTask = NAMELAST;
                    }
                }
                break;

            case NAMELAST:
                if (currentBit > 0)
                    OricBitWrite();
                else
                {
                    if (count > 0)
                    {
                        currentByte = 0x00;
                        currentBit = 11;
                        bitChecksum = 0;
                        lastByte = 0;
                        count--;
                    }
                    else
                    {
                        count = 100;
                        lastByte = 0;
                        currentBlockTask = GAP;
                    }
                }
                break;

            case GAP:
                if (count > 0)
                {
                    currentPeriod = ORICONEPULSE;
                    count--;
                }
                else
                {
                    currentBlockTask = DATA;
                }
                break;

            case DATA:
                OricDataBlock();
                break;

            case PAUSE:
                //currentPeriod = 100; // 100ms pause
                //bitSet(currentPeriod, 15);
                if (!count == 0)
                {
                    currentPeriod = 32769;
                    count += -1;
                }
                else
                {
                    count = 255;
                    currentBlockTask = SYNC1;
                }
                break;
            }
            break;

        case IDPAUSE:

            if (temppause > 0)
            {
                if (temppause > 8300)
                {
                    //Serial.println(temppause, DEC);
                    currentPeriod = 8300;
                    temppause += -8300;
                }
                else
                {
                    currentPeriod = temppause;
                    temppause = 0;
                }
                bitSet(currentPeriod, 15);
            }
            else
            {
                currentTask = GETID;
                if (EndOfFile == true)
                    currentID = EOF;
            }
            break;

        case EOF:
            //Handle end of file
            if (!count == 0)
            {
                //currentPeriod = 32767;
                currentPeriod = 10;
                //bitSet(currentPeriod, 15); bitSet(currentPeriod, 12);
                count += -1;
            }
            else
            {
                stopFile();
                return;
            }
            break;

        default:

            //ID Not Recognised - Fall back if non TZX file or unrecognised ID occurs
            printError(STR_ERR_ID);

            // lcd.clear();
            // lcd.setCursor(0, 0);
            // lcd.print("ID? ");
            // lcd.setCursor(4, 0);
            // lcd.print(String(currentID, HEX));
            // lcd.setCursor(0, 1);
            // lcd.print(String(bytesRead, HEX) + " - L: " + String(loopCount, DEC));

            delay(5000);
            stopFile();
            break;
        }
    }
}

void StandardBlock()
{
    //Standard Block Playback
    switch (currentBlockTask)
    {
    case PILOT:
        //Start with Pilot Pulses
        currentPeriod = pilotLength;
        pilotPulses += -1;
        if (pilotPulses == 0)
        {
            currentBlockTask = SYNC1;
        }
        break;

    case SYNC1:
        //First Sync Pulse
        currentPeriod = sync1Length;
        currentBlockTask = SYNC2;
        break;

    case SYNC2:
        //Second Sync Pulse
        currentPeriod = sync2Length;
        currentBlockTask = DATA;
        break;

    case DATA:
        //Data Playback
        if ((AYPASS == 0) | (AYPASS == 4) | (AYPASS == 5))
            writeData(); // Check if we are playing from file or Vector String and we need to send first 0xFF byte or checksum byte at EOF
        else
        {
            writeHeader(); // write TAP Header data from String Vector (AYPASS=1)
        }
        break;

    case PAUSE:
        //Close block with a pause
        // DEBUG
        //lcd.setCursor(0,1);
        //lcd.print(blkchksum,HEX); lcd.print("ck ptr:"); lcd.print(hdrptr);

        if ((currentID != TAP) && (currentID != AYO))
        { // Check if we have !=AYO too
            temppause = pauseLength;
            currentID = IDPAUSE;
        }
        else
        {
            currentPeriod = pauseLength;
            bitSet(currentPeriod, 15);
            currentBlockTask = READPARAM;
        }
        if (EndOfFile == true)
            currentID = EOF;
        break;
    }
}

void PureToneBlock()
{
    //Pure Tone Block - Long string of pulses with the same length
    currentPeriod = pilotLength;
    pilotPulses += -1;
    if (pilotPulses == 0)
    {
        currentTask = GETID;
    }
}

void PulseSequenceBlock()
{
    //Pulse Sequence Block - String of pulses each with a different length
    //Mainly used in speedload blocks
    byte r = 0;
    if (r = readWord() == 2)
    {
        currentPeriod = TickToUs(outWord);
    }
    seqPulses += -1;
    if (seqPulses == 0)
    {
        currentTask = GETID;
    }
}

void PureDataBlock()
{
    //Pure Data Block - Data & pause only, no header, sync
    switch (currentBlockTask)
    {
    case DATA:
        writeData();
        break;

    case PAUSE:
        temppause = pauseLength;
        currentID = IDPAUSE;
        break;
    }
}

void writeData4B()
{
    //Convert byte (4B Block) from file into string of pulses.  One pulse per pass
    byte r;
    byte dataBit;

    //Continue with current byte
    if (currentBit > 0)
    {

        //Start bit (0)
        if (currentBit == 11)
        {
            currentPeriod = zeroPulse;
            pass += 1;
            if (pass == 2)
            {
                currentBit += -1;
                pass = 0;
            }
        }
        else
            //Stop bits (1)
            if (currentBit <= 2)
        {
            currentPeriod = onePulse;
            pass += 1;
            if (pass == 4)
            {
                currentBit += -1;
                pass = 0;
            }
        }
        else
        //Data bits
        {
            dataBit = currentByte & 1;
            currentPeriod = dataBit == 1 ? onePulse : zeroPulse;
            pass += 1;
            if ((dataBit == 1 && pass == 4) || (dataBit == 0 && pass == 2))
            {
                currentByte >>= 1;
                currentBit += -1;
                pass = 0;
            }
        }
    }
    else if (currentBit == 0 && bytesToRead != 0)
    {
        //Read new byte
        if (r = readByte() == 1)
        {
            bytesToRead += -1;
            currentByte = outByte;
            currentBit = 11;
            pass = 0;
        }
        else if (r == 0)
        {
            //End of file
            currentID = EOF;
            return;
        }
    }

    //End of block?
    if (bytesToRead == 0 && currentBit == 0)
    {
        temppause = pauseLength;
        currentBlockTask = PAUSE;
    }
}

void DirectRecording()
{
    //Direct Recording - Output bits based on specified sample rate (Ticks per clock) either 44.1KHz or 22.05
    switch (currentBlockTask)
    {
    case DATA:
        writeSampleData();
        break;

    case PAUSE:
        temppause = pauseLength;
        currentID = IDPAUSE;
        break;
    }
}

void ZX81FilenameBlock()
{
    //output ZX81 filename data  byte r;
    if (currentBit == 0)
    { //Check for byte end/first byte
        //currentByte=ZX81Filename[currentChar];
        currentByte = pgm_read_byte(ZX81Filename + currentChar);
        currentChar += 1;
        if (currentChar == 10)
        {
            currentBlockTask = DATA;
            return;
        }
        currentBit = 9;
        pass = 0;
    }

    ZX80ByteWrite();
}

void ZX8081DataBlock()
{
    byte r;
    if (currentBit == 0)
    { //Check for byte end/first byte
        if (r = readByte() == 1)
        { //Read in a byte
            currentByte = outByte;
            bytesToRead += -1;
        }
        else if (r == 0)
        {
            //EndOfFile=true;
            //temppause = 3000;
            temppause = pauseLength;
            currentID = IDPAUSE;
            //return;
        }
        currentBit = 9;
        pass = 0;
    }

    ZX80ByteWrite();
}

void ZX80ByteWrite()
{
    if (uefTurboMode == 1)
    {
        currentPeriod = ZX80TURBOPULSE;
        if (pass == 1)
        {
            currentPeriod = ZX80TURBOBITGAP;
        }
    }
    else
    {
        currentPeriod = ZX80PULSE;
        if (pass == 1)
        {
            currentPeriod = ZX80BITGAP;
        }
    }
    if (pass == 0)
    {
        if (currentByte & 0x80)
        { //Set next period depending on value of bit 0
            pass = 19;
        }
        else
        {
            pass = 9;
        }
        currentByte <<= 1; //Shift along to the next bit
        currentBit += -1;
        currentPeriod = 0;
    }
    pass += -1;
}

void writeData()
{
    //Convert byte from file into string of pulses.  One pulse per pass
    byte r;
    if (currentBit == 0)
    { //Check for byte end/first byte
        if (r = readByte() == 1)
        { //Read in a byte
            currentByte = outByte;
            if (AYPASS == 5)
            {
                currentByte = 0xFF; // Only insert first DATA byte if sending AY TAP DATA Block and don't decrement counter
                AYPASS = 4;         // set Checksum flag to be sent when EOF reached
                bytesRead += -1;    // rollback ptr and compensate for dummy read byte
                bytesToRead += 2;   // add 2 bytes to read as we send 0xFF (data flag header byte) and chksum at the end of the block
            }
            else
            {
                bytesToRead += -1;
            }
            blkchksum = blkchksum ^ currentByte; // keep calculating checksum
            if (bytesToRead == 0)
            {                    //Check for end of data block
                bytesRead += -1; //rewind a byte if we've reached the end
                if (pauseLength == 0)
                { //Search for next ID if there is no pause
                    currentTask = GETID;
                }
                else
                {
                    currentBlockTask = PAUSE; //Otherwise start the pause
                }
                return; // exit
            }
        }
        else if (r == 0)
        { // If we reached the EOF
            if (AYPASS != 4)
            { // Check if need to send checksum
                EndOfFile = true;
                if (pauseLength == 0)
                {
                    currentTask = GETID;
                }
                else
                {
                    currentBlockTask = PAUSE;
                }
                return; // return here if normal TAP or TZX
            }
            else
            {
                currentByte = blkchksum; // else send calculated chksum
                bytesToRead += 1;        // add one byte to read
                AYPASS = 0;              // Reset flag to end block
            }
            //return;
        }
        if (bytesToRead != 1)
        { //If we're not reading the last byte play all 8 bits
            currentBit = 8;
        }
        else
        {
            currentBit = usedBitsInLastByte; //Otherwise only play back the bits needed
        }
        pass = 0;
    }
    if (currentByte & 0x80)
    { //Set next period depending on value of bit 0
        currentPeriod = onePulse;
    }
    else
    {
        currentPeriod = zeroPulse;
    }
    pass += 1; //Data is played as 2 x pulses
    if (pass == 2)
    {
        currentByte <<= 1; //Shift along to the next bit
        currentBit += -1;
        pass = 0;
    }
}

void writeHeader()
{
    //Convert byte from HDR Vector String into string of pulses and calculate checksum. One pulse per pass
    if (currentBit == 0)
    { //Check for byte end/new byte
        if (hdrptr == 19)
        { // If we've reached end of header block send checksum byte
            currentByte = blkchksum;
            AYPASS = 2;               // set flag to Stop playing from header in RAM
            currentBlockTask = PAUSE; // we've finished outputting the TAP header so now PAUSE and send DATA block normally from file
            return;
        }
        hdrptr += 1; // increase header string vector pointer
        if (hdrptr < 20)
        { //Read a byte until we reach end of tap header
            //currentByte = TAPHdr[hdrptr];
            currentByte = pgm_read_byte(TAPHdr + hdrptr);
            if (hdrptr == 13)
            { // insert calculated block length minus LEN bytes
                currentByte = lowByte(ayblklen - 3);
            }
            else if (hdrptr == 14)
            {
                currentByte = highByte(ayblklen);
            }
            blkchksum = blkchksum ^ currentByte; // Keep track of Chksum
                                                 //}
                                                 //if(hdrptr<20) {               //If we're not reading the last byte play all 8 bits
                                                 //if(bytesToRead!=1) {                      //If we're not reading the last byte play all 8 bits
            currentBit = 8;
        }
        else
        {
            currentBit = usedBitsInLastByte; //Otherwise only play back the bits needed
        }
        pass = 0;
    } //End if currentBit == 0
    if (currentByte & 0x80)
    { //Set next period depending on value of bit 0
        currentPeriod = onePulse;
    }
    else
    {
        currentPeriod = zeroPulse;
    }
    pass += 1; //Data is played as 2 x pulses
    if (pass == 2)
    {
        currentByte <<= 1; //Shift along to the next bit
        currentBit += -1;
        pass = 0;
    }
} // End writeHeader()

void ReadTZXHeader()
{
    if (!checkFileHeader(TZXTape, 10, 7))
    {
        printError(STR_ERR_NOTTZX);
        TZXStop();
    }
}

void ReadAYHeader()
{
    if (!checkFileHeader(AYFile, 8, 8))
    {
        printError(STR_ERR_NOTAY);
        TZXStop();
    }
    // TODO: is it a bug?
    bytesRead = 0;
}

void writeSampleData()
{
    //Convert byte from file into string of pulses.  One pulse per pass
    byte r;
    ID15switch = 1;
    if (currentBit == 0)
    { //Check for byte end/first byte
        if (r = readByte() == 1)
        { //Read in a byte
            currentByte = outByte;
            bytesToRead += -1;
            if (bytesToRead == 0)
            {                    //Check for end of data block
                bytesRead += -1; //rewind a byte if we've reached the end
                if (pauseLength == 0)
                { //Search for next ID if there is no pause
                    currentTask = GETID;
                }
                else
                {
                    currentBlockTask = PAUSE; //Otherwise start the pause
                }
                return;
            }
        }
        else if (r == 0)
        {
            EndOfFile = true;
            if (pauseLength == 0)
            {
                //ID15switch = 0;
                currentTask = GETID;
            }
            else
            {
                currentBlockTask = PAUSE;
            }
            return;
        }
        if (bytesToRead != 1)
        { //If we're not reading the last byte play all 8 bits
            currentBit = 8;
        }
        else
        {
            currentBit = usedBitsInLastByte; //Otherwise only play back the bits needed
        }
        pass = 0;
    }
    if bitRead (currentPeriod, 14)
    {
        //bitWrite(currentPeriod,13,currentByte&0x80);
        if (currentByte & 0x80)
            bitSet(currentPeriod, 13);
        pass += 2;
    }
    else
    {
        if (currentByte & 0x80)
        { //Set next period depending on value of bit 0
            currentPeriod = onePulse;
        }
        else
        {
            currentPeriod = zeroPulse;
        }
        pass += 1;
    }
    if (pass == 2)
    {
        currentByte <<= 1; //Shift along to the next bit
        currentBit += -1;
        pass = 0;
    }
}

//
// UEFProcessing.ino
//

void ReadUEFHeader()
{
    //Read and check first 12 bytes for a UEF header
    if (!checkFileHeader(UEFFile, 9, 9))
    {
        printError(STR_ERR_NOTUEF);
        TZXStop();
    }
    bytesRead = 12;
}

void UEFCarrierToneBlock()
{
    //Pure Tone Block - Long string of pulses with the same length
    currentPeriod = pilotLength;
    pilotPulses += -1;
    if (pilotPulses == 0)
    {
        currentTask = GETCHUNKID;
    }
}

void writeUEFData()
{
    //Convert byte from file into string of pulses.  One pulse per pass
    byte r;
    if (currentBit == 0)
    { //Check for byte end/first byte

        if (r = readByte() == 1)
        { //Read in a byte
            currentByte = outByte;
            //itoa(currentByte,PlayBytes,16); printtext(PlayBytes,lineaxy);
            bytesToRead += -1;
            bitChecksum = 0;

            //blkchksum = blkchksum ^ currentByte;    // keep calculating checksum
            if (bytesToRead == 0)
            { //Check for end of data block
                lastByte = 1;
                //Serial.println(F("  Rewind bytesRead"));
                //bytesRead += -1;                      //rewind a byte if we've reached the end
                if (pauseLength == 0)
                { //Search for next ID if there is no pause
                    currentTask = PROCESSCHUNKID;
                }
                else
                {
                    currentBlockTask = PAUSE; //Otherwise start the pause
                }
                //return;                               // exit
            }
        }
        else if (r == 0)
        { // If we reached the EOF
            currentTask = GETCHUNKID;
        }

        currentBit = 11;
        pass = 0;
    }
    if ((currentBit == 2) && (parity == 0))
        currentBit = 1; // parity N
    if (currentBit == 11)
    {
        currentPeriod = zeroPulse;
    }
    else if (currentBit == 2)
    {
        //itoa(bitChecksum,PlayBytes,16);printtext(PlayBytes,lineaxy);
        currentPeriod = (bitChecksum ^ (parity & 0x01)) ? onePulse : zeroPulse;
        //currentPeriod =  bitChecksum ? onePulse : zeroPulse;
    }
    else if (currentBit == 1)
    {
        currentPeriod = onePulse;
    }
    else
    {
        if (currentByte & 0x01)
        { //Set next period depending on value of bit 0
            currentPeriod = onePulse;
        }
        else
        {
            currentPeriod = zeroPulse;
        }
    }
    pass += 1; //Data is played as 2 x pulses for a zero, and 4 pulses for a one when speed is 1200

    if (currentPeriod == zeroPulse)
    {
        if (pass == passforZero)
        {
            if ((currentBit > 1) && (currentBit < 11))
            {
                currentByte >>= 1; //Shift along to the next bit
            }
            currentBit += -1;
            pass = 0;
            if ((lastByte) && (currentBit == 0))
            {
                currentTask = GETCHUNKID;
            }
        }
    }
    else
    {
        // must be a one pulse
        if (pass == passforOne)
        {
            if ((currentBit > 1) && (currentBit < 11))
            {
                bitChecksum ^= 1;
                currentByte >>= 1; //Shift along to the next bit
            }

            currentBit += -1;
            pass = 0;
            if ((lastByte) && (currentBit == 0))
            {
                currentTask = GETCHUNKID;
            }
        }
    }
}

//
// ORICProcessing.ino
//

void OricDataBlock()
{
    //Convert byte from file into string of pulses.  One pulse per pass
    byte r;
    if (currentBit == 0)
    { //Check for byte end/first byte

        if (r = readByte() == 1)
        { //Read in a byte
            currentByte = outByte;
            bytesToRead += -1;
            bitChecksum = 0;
            if (bytesToRead == 0)
            { //Check for end of data block
                lastByte = 1;
            }
        }
        else if (r == 0)
        { // If we reached the EOF
            EndOfFile = true;
            temppause = 0;
            forcePause0 = 1;
            count = 255;
            currentID = IDPAUSE;
            return;
        }

        currentBit = 11;
        pass = 0;
    }
    OricBitWrite();
}

void OricBitWrite()
{
    if (currentBit == 11)
    { //Start Bit
        if (pass == 0)
            currentPeriod = ORICZEROLOWPULSE;
        if (pass == 1)
            currentPeriod = ORICZEROHIGHPULSE;
    }
    else if (currentBit == 2)
    { // Paridad inversa i.e. Impar
        if (pass == 0)
            currentPeriod = bitChecksum ? ORICZEROLOWPULSE : ORICONEPULSE;
        if (pass == 1)
            currentPeriod = bitChecksum ? ORICZEROHIGHPULSE : ORICONEPULSE;
    }
    else if (currentBit == 1)
    {
        currentPeriod = ORICONEPULSE;
    }
    else
    {
        if (currentByte & 0x01)
        { //Set next period depending on value of bit 0
            currentPeriod = ORICONEPULSE;
        }
        else
        {
            //currentPeriod = ORICZEROPULSE;
            if (pass == 0)
                currentPeriod = ORICZEROLOWPULSE;
            if (pass == 1)
                currentPeriod = ORICZEROHIGHPULSE;
        }
    }

    pass += 1; //Data is played as 2 x pulses for a zero, and 2 pulses for a one

    if (currentPeriod == ORICONEPULSE)
    {
        // must be a one pulse

        if ((currentBit > 2) && (currentBit < 11) && (pass == 2))
        {
            bitChecksum ^= 1;
            currentByte >>= 1; //Shift along to the next bit
            currentBit += -1;
            pass = 0;
        }
        if ((currentBit == 1) && (pass == 6))
        {
            currentBit += -1;
            pass = 0;
        }
        if (((currentBit == 2) || (currentBit == 11)) && (pass == 2))
        {
            currentBit += -1;
            pass = 0;
        }
        if ((currentBit == 0) && (lastByte))
        {
            //currentTask = GETCHUNKID;
            count = 255;
            currentBlockTask = PAUSE;
        }
    }
    else
    {
        // must be a zero pulse
        if (pass == 2)
        {
            if ((currentBit > 2) && (currentBit < 11))
            {
                currentByte >>= 1; //Shift along to the next bit
            }
            currentBit += -1;
            pass = 0;
            if ((currentBit == 0) && (lastByte))
            {
                //currentTask = GETCHUNKID;
                count = 255;
                currentBlockTask = PAUSE;
            }
        }
    }
}
