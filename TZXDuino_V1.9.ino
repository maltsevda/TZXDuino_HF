#include "TZXDuino.h"

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
SdFat sd;     //Initialise Sd card
SdFile entry; //SD card file

// -->  Garbage  <--

#define filenameLength 100 //Maximum length for scrolling filename

char fileName[filenameLength + 1]; //Current filename
char sfileName[13];                //Short filename variable
char prevSubDir[3][25];
int subdir = 0;
unsigned long filesize;    // filesize used for dimensioning AY files

byte start = 0;             //Currently playing flag
byte pauseOn = 0;           //Pause state
int currentFile = 1;        //Current position in directory
int maxFile = 0;            //Total number of files in directory
byte isDir = 0;             //Is the current file a directory
unsigned long timeDiff = 0; //button debounce
unsigned long timeDiff2 = 0;
unsigned int lcdsegs = 0;

byte currpct = 100;
byte newpct = 0;

byte UP = 0; //Next File, down button pressed

void TZXSetup();
void TZXPlay(char *filename);
void TZXLoop();
void TZXStop();
void stopFile();

//
// Sound Functions
//

void sound(uint8_t val)
{
    digitalWrite(SND_OUTPUT, val);
}

//
// SD Card Functions
//

bool changeDir(const char* path, bool setCWD = true)
{
    bool isRoot = (path == nullptr) || (path[0] == 0)
        || (path[0] == '/' && path[1] == 0);

    if (isRoot)
    {
        sd.vwd()->close();
        sd.vwd()->openRoot(&sd);
    }
    else
    {
        FatFile dir;
        if (!dir.open(sd.vwd(), path, O_READ))
            return false;
        if (!dir.isDir())
            return false;
        *(sd.vwd()) = dir;
    }

    if (setCWD)
        FatFile::setCwd(sd.vwd());

    return true;
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
        changeDir("/");
    else
        printLine(STR_NO_SD_CARD, 0);

    // Buttons

    pinMode(BTN_PLAY, INPUT);
    pinMode(BTN_STOP, INPUT);
    pinMode(BTN_UP,   INPUT);
    pinMode(BTN_DOWN, INPUT);

    // Output

    pinMode(SND_OUTPUT, OUTPUT);
    sound(LOW);

    // Garbage

    TZXSetup();             //Setup TZX specific options
    getMaxFile();           //get the total number of files in the directory
    seekFile(currentFile);  //move to the first file in the directory
}

void loop(void)
{

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
            newpct = (100 * bytesRead) / filesize;
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

    if (millis() - timeDiff > 50)
    {                        // check switch every 50ms
        timeDiff = millis(); // get current millisecond count

        if (digitalRead(BTN_PLAY) == HIGH)
        {
            //Handle Play/Pause button
            if (start == 0)
            {
                //If no file is play, start playback
                playFile();
                delay(200);
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
            while (digitalRead(BTN_PLAY) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }

        if (digitalRead(BTN_STOP) == HIGH && start == 1)
        {
            stopFile();
            while (digitalRead(BTN_STOP) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }
        if (digitalRead(BTN_STOP) == HIGH && start == 0 && subdir > 0)
        {
            fileName[0] = '\0';
            prevSubDir[subdir - 1][0] = '\0';
            subdir--;
            switch (subdir)
            {
            case 1:
                changeDir(strcat(strcat(fileName, "/"), prevSubDir[0]));
                break;
            case 2:
                changeDir(strcat(strcat(strcat(strcat(fileName, "/"), prevSubDir[0]), "/"), prevSubDir[1]));
                break;
            case 3:
                changeDir(strcat(strcat(strcat(strcat(strcat(strcat(fileName, "/"), prevSubDir[0]), "/"), prevSubDir[1]), "/"), prevSubDir[2]));
                break;
            default:
                changeDir("/");
            }
            //Return to prev Dir of the SD card.
            getMaxFile();
            currentFile = 1;
            seekFile(currentFile);
            while (digitalRead(BTN_STOP) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }

        if (digitalRead(BTN_UP) == HIGH && start == 0)
        {
            //Move up a file in the directory
            upFile();
            while (digitalRead(BTN_UP) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }

        if (digitalRead(BTN_DOWN) == HIGH && start == 0)
        {
            //Move down a file in the directory
            downFile();
            while (digitalRead(BTN_DOWN) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }
    }
}

void upFile()
{
    //move up a file in the directory
    currentFile--;
    if (currentFile < 1)
    {
        getMaxFile();
        currentFile = maxFile;
    }
    UP = 1;
    seekFile(currentFile);
}

void downFile()
{
    //move down a file in the directory
    currentFile++;
    if (currentFile > maxFile)
    {
        currentFile = 1;
    }
    UP = 0;
    seekFile(currentFile);
}

void seekFile(int pos)
{
    //move to a set position in the directory, store the filename, and display the name on screen.
    if (UP == 1)
    {
        entry.cwd()->rewind();
        for (int i = 1; i <= currentFile - 1; i++)
        {
            entry.openNext(entry.cwd(), O_READ);
            entry.close();
        }
    }

    if (currentFile == 1)
    {
        entry.cwd()->rewind();
    }
    entry.openNext(entry.cwd(), O_READ);
    entry.getName(fileName, filenameLength);
    entry.getSFN(sfileName);
    filesize = entry.fileSize();
    ayblklen = filesize + 3; // add 3 file header, data byte and chksum byte to file length
    if (entry.isDir() || !strcmp(sfileName, "ROOT"))
    {
        isDir = 1;
    }
    else
    {
        isDir = 0;
    }
    entry.close();

    if (isDir == 1)
        printLine(STR_DIR, 0);
    else
        printLine(STR_FILE, 0);
    printLine(fileName, 1);
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
    if (isDir == 1)
    {
        //change directory, if fileName="ROOT" then return to the root directory
        //SDFat has no easy way to move up a directory, so returning to root is the easiest way.
        //each directory (except the root) must have a file called ROOT (no extension)

        if (!strcmp(fileName, "ROOT"))
        {
            subdir = 0;
            changeDir("/");
        }
        else
        {
            if (subdir > 0)
                entry.cwd()->getName(prevSubDir[subdir - 1], filenameLength);
            changeDir(fileName);
            subdir++;
        }
        getMaxFile();
        currentFile = 1;
        seekFile(currentFile);
    }
    else
    {
        if (entry.cwd()->exists(sfileName))
        {
            printLine(STR_PLAYING_FULL, 0);
            pauseOn = 0;
            printLine(fileName, 1);
            currpct = 100;
            lcdsegs = 0;
            TZXPlay(sfileName); //Load using the short filename
            start = 1;
        }
        else
        {
            printLine(STR_NO_FILE, 1);
        }
    }
}

void getMaxFile()
{
    //gets the total files in the current directory and stores the number in maxFile

    entry.cwd()->rewind();
    maxFile = 0;
    while (entry.openNext(entry.cwd(), O_READ))
    {
        entry.close();
        maxFile++;
    }
    entry.cwd()->rewind();
}
