#include "TZXDuino.h"
#include "Button.h"

// PIN Configuration

#define BTN_PLAY    4
#define BTN_STOP    5
#define BTN_UP      2
#define BTN_DOWN    3

#define LCD_RS      A0
#define LCD_EN      A1
#define LCD_D4      A2
#define LCD_D5      A3
#define LCD_D6      A4
#define LCD_D7      A5

#define SD_CHIPSELECT   10

#define SND_OUTPUT  9

// Strings

#define STR_EMPTY           F("                ")
#define STR_NO_SD_CARD      F("Err: No SD Card ")
#define STR_DIR             F("Dir             ")
#define STR_FILE            F("File            ")
#define STR_NO_FILE         F("No File Selected")
#define STR_PLAYING_FULL    F("Playing         ")
#define STR_STOPPED_FULL    F("Stopped         ")
#define STR_PLAYING_8       F("Playing ")
#define STR_PAUSED_8        F("Paused  ")

// Global Object

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

SdFat sd;

Button<BTN_PLAY> buttonPlay(INPUT);
Button<BTN_STOP> buttonStop(INPUT);
Button<BTN_UP>   buttonUp(INPUT);
Button<BTN_DOWN> buttonDown(INPUT);

// File System Info

#define SD_MAX_WORKDIR      128
#define SD_MAX_FILENAME     32

char workDir[SD_MAX_WORKDIR] = { '/' };
char fileName[SD_MAX_FILENAME];
char shortFileName[13];
uint32_t fileSize;

// -->  Garbage  <--

byte start = 0;             //Currently playing flag
byte pauseOn = 0;           //Pause state
unsigned long timeDiff2 = 0;
unsigned int lcdsegs = 0;
byte currpct = 100;
byte newpct = 0;

void TZXSetup();
void TZXPlay(char *filename);
void TZXLoop();
void TZXStop();
void playFile();
void stopFile();

//
// Sound Functions
//

void sound(uint8_t val)
{
    digitalWrite(SND_OUTPUT, val);
}

//
// LCD Functions
//

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
        lcd.print("999");
    else
    {
        if (value < 10)
            lcd.print("00");
        else if (value < 100)
            lcd.print('0');
        lcd.print(value);
    }
}

void printProgress(uint8_t value, bool eraseTail = false)
{
    lcd.setCursor(8, 0);
    lcd.print(value);
    lcd.print('%');
    if (eraseTail)
    {
        if (value < 10)
            lcd.print("  ");
        else if (value < 100)
            lcd.print(' ');
    }
}

//
// SD Card Functions
//

bool changeDir(const char* path, bool updateWorkDir = true)
{
    if (path[0] == 0 || (path[0] == '/' && path[1] == 0))
    {
        // switch to root folder
        sd.chdir(true);
        SdFile::cwd()->rewind();
        if (updateWorkDir)
        {
            workDir[0] = '/';
            workDir[1] = 0;
        }
        return true;
    }
    else
    {
        if (sd.chdir(path, true))
        {
            SdFile::cwd()->rewind();
            if (updateWorkDir)
            {
                if (path[0] == '/')
                    strcpy(workDir, path);
                else
                {
                    strcat(workDir, path);
                    strcat(workDir, "/");
                }
            }
            return true;
        }
    }
    return false;
}

bool parentDir()
{
    if (workDir[0] == '/' && workDir[1] == 0)
        return false;

    uint8_t index = 0;
    for (uint8_t i = 0; workDir[i]; ++i)
    {
        if (workDir[i] == '/' && workDir[i + 1])
            index = i + 1;
    }
    workDir[index] = 0;

    return changeDir(workDir, false);
}

void readCurrentFileInfo(SdFile* entry)
{
    entry->getName(fileName, SD_MAX_FILENAME);
    entry->getSFN(shortFileName);
    fileSize = entry->isDir() ? 0 : entry->fileSize();
    ayblklen = fileSize + 3; // add 3 file header, data byte and chksum byte to file length

    if (entry->isDir())
        printLine(STR_DIR, 0);
    else
        printLine(STR_FILE, 0);
    printLine(fileName, 1);
}

void openNext()
{
    SdFile file;
    if (file.openNext(SdFile::cwd(), O_READ))
    {
        readCurrentFileInfo(&file);
        file.close();
    }
    else
    {
        // TODO: fix this
        printLine(STR_EMPTY, 0);
        printLine(STR_EMPTY, 1);
    }
}

void openPrev()
{
    SdFile file;
    uint16_t index = SdFile::cwd()->curPosition() / 32;

    if (index > 0)
        --index;

    while (index > 0)
    {
        --index;
        if (file.open(SdFile::cwd(), index, O_READ))
        {
            readCurrentFileInfo(&file);
            file.close();
            break;
        }
    }

    if (index == 0)
    {
        // TODO: fix this
        printLine(STR_EMPTY, 0);
        printLine(STR_EMPTY, 1);
    }
}

//
// -->  Garbage  <--
//

void setup()
{
    // LCD

    lcd.begin(16, 2);
    lcd.clear();

    // SD

    pinMode(SD_CHIPSELECT, OUTPUT); //Setup SD card chipselect pin
    if (sd.begin(SD_CHIPSELECT, SPI_FULL_SPEED))
    {
        changeDir("/");
        openNext();
    }
    else
        printLine(STR_NO_SD_CARD, 0);

    // Output

    pinMode(SND_OUTPUT, OUTPUT);
    sound(LOW);

    // Garbage

    TZXSetup();             //Setup TZX specific options
}

void loop(void)
{
    buttonPlay.tick();
    buttonStop.tick();
    if (start == 0)
    {
        buttonUp.tick();
        buttonDown.tick();
    }

    if (start == 1)
    {
        //TZXLoop only runs if a file is playing, and keeps the buffer full.
        TZXLoop();

        // percent of playing and right-side counter 000
        if (btemppos > buffsize)
        {
            if ((pauseOn == 0) && (currpct < 100))
            {
                if (millis() - timeDiff2 > 1000)
                {                         // check switch every second
                    timeDiff2 = millis(); // get current millisecond count
                    printCounter(lcdsegs);
                    lcdsegs++;
                }
            }
            newpct = (100 * bytesRead) / fileSize;
            if (currpct == 100)
            {
                printProgress(newpct);
                currpct = 0;
            }
            if ((newpct > currpct) && (newpct % 1 == 0))
            {
                printProgress(newpct);
                currpct = newpct;
            }
        }
    }
    else
    {
        sound(LOW); //Keep output LOW while no file is playing.
    }

    if (buttonPlay.press())
    {
        //Handle Play/Pause button
        if (start == 0)
        {
            if (fileSize == 0)
            {
                changeDir(shortFileName);
                openNext();
            }
            else
                playFile();
        }
        else
        {
            //If a file is playing, pause or unpause the file
            if (pauseOn == 0)
            {
                printAt(STR_PAUSED_8, 0, 0);
                pauseOn = 1;
            }
            else
            {
                printAt(STR_PLAYING_8, 0, 0);
                pauseOn = 0;
            }
        }
    }

    if (buttonStop.press())
    {
        if (start == 1)
        {
            stopFile();
        }
        else
        {
            if (parentDir())
                openNext();
        }
    }

    if (buttonUp.press() && start == 0)
        openPrev();

    if (buttonDown.press() && start == 0)
        openNext();
}

void stopFile()
{
    TZXStop();
    if (start == 1)
    {
        printLine(STR_STOPPED_FULL, 0);
        start = 0;
    }
}

void playFile()
{
    if (SdFile::cwd()->exists(shortFileName))
    {
        printLine(STR_PLAYING_FULL, 0);
        pauseOn = 0;
        printLine(fileName, 1);
        currpct = 100;
        lcdsegs = 0;
        TZXPlay(shortFileName); //Load using the short filename
        start = 1;
    }
    else
    {
        printLine(STR_NO_FILE, 1);
    }
}
