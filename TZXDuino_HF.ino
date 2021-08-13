#include <LiquidCrystal.h>
#include <SdFat.h>
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
bool fileIsDir;

// Player State

#define MODE_BROWSE         0
#define MODE_PLAYING        1
#define MODE_PAUSED         2

uint8_t playerMode = MODE_BROWSE;

unsigned long counterTime = 0;
uint16_t counter = 0;
uint8_t percentages = 0;

// TZX Interface

void TZXSetup();
void TZXPlay(char *filename);
void TZXLoop();
void TZXStop();

extern byte isStopped;
extern unsigned long bytesRead;

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

void printPercentages(uint8_t value, bool eraseTail = false)
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

void readCurrentFileInfo(SdFile* file)
{
    file->getName(fileName, SD_MAX_FILENAME);
    file->getSFN(shortFileName);
    fileSize = file->fileSize();
    fileIsDir = file->isDir();

    if (fileIsDir)
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

    // skip self index
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
// Global File Operations
//

bool isFileStopped()
{
    return playerMode != MODE_PLAYING;
}

void stopFile()
{
    TZXStop();
    if (playerMode == MODE_PLAYING || playerMode == MODE_PAUSED)
    {
        printLine(STR_STOPPED_FULL, 0);
        playerMode = MODE_BROWSE;
    }
}

//
// Setup
//

void setup()
{
    // LCD

    lcd.begin(16, 2);
    lcd.clear();

    // SD

    pinMode(SD_CHIPSELECT, OUTPUT);
    if (sd.begin(SD_CHIPSELECT, SPI_FULL_SPEED))
    {
        changeDir("/");
        openNext();
    }
    else
        printLine(STR_NO_SD_CARD, 0);

    // Sound Output

    pinMode(SND_OUTPUT, OUTPUT);
    sound(LOW);

    //Setup TZX specific options

    TZXSetup();
}

//
// Loops and Modes
//

void loopBrowse()
{
    sound(LOW);

    buttonPlay.tick();
    buttonStop.tick();
    buttonUp.tick();
    buttonDown.tick();

    if (buttonPlay.press())
    {
        if (fileIsDir)
        {
            if (changeDir(shortFileName))
                openNext();
        }
        else
        {
            if (SdFile::cwd()->exists(shortFileName))
            {
                playerMode = MODE_PLAYING;
                counterTime = millis();
                counter = 0;
                percentages = UINT8_MAX;    // set unreal value for first printing
                TZXPlay(shortFileName);
                printLine(STR_PLAYING_FULL, 0);
                printLine(fileName, 1);
            }
            else
            {
                printLine(STR_NO_FILE, 1);
            }
        }
    }

    if (buttonStop.press())
    {
        if (parentDir())
            openNext();
    }

    if (buttonUp.press())
        openPrev();

    if (buttonDown.press())
        openNext();
}

void loopPlaying()
{
    //TZXLoop only runs if a file is playing, and keeps the buffer full.
    TZXLoop();

    buttonPlay.tick();
    buttonStop.tick();

    // percents of playing and right-side counter 000
    uint8_t newPercentages = (100 * bytesRead) / fileSize;
    if (percentages != newPercentages)
    {
        percentages = newPercentages;
        printPercentages(percentages);
    }

    if (percentages < 100)
    {
        // check switch every second
        if (millis() - counterTime > 1000)
        {
            counterTime = millis();
            printCounter(counter++);
        }
    }

    if (buttonPlay.press())
    {
        printAt(STR_PAUSED_8, 0, 0);
        playerMode = MODE_PAUSED;
    }

    if (buttonStop.press())
        stopFile();
}

void loopPaused()
{
    // Part of TZXLoop
    isStopped = isFileStopped();

    sound(LOW);

    buttonPlay.tick();
    buttonStop.tick();

    if (buttonPlay.press())
    {
        printAt(STR_PLAYING_8, 0, 0);
        playerMode = MODE_PLAYING;
    }

    if (buttonStop.press())
        stopFile();
}

void loop(void)
{
    switch (playerMode)
    {
    case MODE_BROWSE: loopBrowse(); break;
    case MODE_PLAYING: loopPlaying(); break;
    case MODE_PAUSED: loopPaused(); break;
    }
}
