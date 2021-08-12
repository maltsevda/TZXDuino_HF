
#include "TZXDuino.h"

#define btnPlay        4            //Play Button
#define btnStop        5            //Stop Button
#define btnUp          2            //Up button
#define btnDown        3            //Down button

const int rs = A0, en = A1, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // set the LCD address to 0x27 for a 16 chars and 2 line display
uint8_t SpecialChar[8] = {0x00, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00};

SdFat sd;     //Initialise Sd card
SdFile entry; //SD card file

#define filenameLength 100 //Maximum length for scrolling filename

char fileName[filenameLength + 1]; //Current filename
char sfileName[13];                //Short filename variable
char prevSubDir[3][25];
int subdir = 0;
unsigned long filesize;    // filesize used for dimensioning AY files
const int chipSelect = 10; //Sd card chip select pin

byte start = 0;             //Currently playing flag
byte pauseOn = 0;           //Pause state
int currentFile = 1;        //Current position in directory
int maxFile = 0;            //Total number of files in directory
byte isDir = 0;             //Is the current file a directory
unsigned long timeDiff = 0; //button debounce
unsigned long timeDiff2 = 0;
unsigned int lcdsegs = 0;

char PlayBytes[16];
byte currpct = 100;
byte newpct = 0;

byte UP = 0; //Next File, down button pressed

void TZXSetup();
void TZXPlay(char *filename);
void TZXLoop();
void TZXStop();
void stopFile();

void setup()
{

    lcd.begin(16, 2); //Initialise LCD (16x2 type)
    // lcd.backlight();
    lcd.clear();
    lcd.createChar(0, SpecialChar);

    pinMode(chipSelect, OUTPUT); //Setup SD card chipselect pin
    if (!sd.begin(chipSelect, SPI_FULL_SPEED))
    {
        //Start SD card and check it's working
        printtextF(PSTR("No SD Card"), 0);
        //lcd_clearline(0);
        //lcd.print(F("No SD Card"));
        return;
    }

    sd.chdir(); //set SD to root directory
    TZXSetup(); //Setup TZX specific options

    //General Pin settings
    //Setup buttons with internal pullup
    pinMode(btnPlay, INPUT);
    pinMode(btnStop, INPUT);
    pinMode(btnUp, INPUT);
    pinMode(btnDown, INPUT);

    printtextF(PSTR("Starting.."), 0);
    delay(500);

    lcd.clear();

    getMaxFile();          //get the total number of files in the directory
    seekFile(currentFile); //move to the first file in the directory
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
                lcdTime();
            newpct = (100 * bytesRead) / filesize;
            if (currpct == 100)
            {
                lcd.setCursor(8,0);
                lcd.print(newpct);
                lcd.print("%");
                currpct = 0;
            }
            if ((newpct > currpct) && (newpct % 1 == 0))
            {
                lcd.setCursor(8,0);
                lcd.print(newpct);
                lcd.print("%");
                currpct = newpct;
            }
        }

    }
    else
    {
        digitalWrite(outputPin, LOW); //Keep output LOW while no file is playing.
    }

    if (millis() - timeDiff > 50)
    {                        // check switch every 50ms
        timeDiff = millis(); // get current millisecond count

        if (digitalRead(btnPlay) == HIGH)
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
                    lcd.setCursor(0,0);
                    lcd.print(F("Paused "));
                    pauseOn = 1;
                }
                else
                {
                    lcd.setCursor(0,0);
                    lcd.print(F("Playing"));
                    pauseOn = 0;
                }
            }
            while (digitalRead(btnPlay) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }

        if (digitalRead(btnStop) == HIGH && start == 1)
        {
            stopFile();
            while (digitalRead(btnStop) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }
        if (digitalRead(btnStop) == HIGH && start == 0 && subdir > 0)
        {
            fileName[0] = '\0';
            prevSubDir[subdir - 1][0] = '\0';
            subdir--;
            switch (subdir)
            {
            case 1:
                //sprintf(fileName,"%s%s",prevSubDir[0],prevSubDir[1]);
                sd.chdir(strcat(strcat(fileName, "/"), prevSubDir[0]), true);
                break;
            case 2:
                //sprintf(fileName,"%s%s/%s",prevSubDir[0],prevSubDir[1],prevSubDir[2]);
                sd.chdir(strcat(strcat(strcat(strcat(fileName, "/"), prevSubDir[0]), "/"), prevSubDir[1]), true);
                break;
            case 3:
                //sprintf(fileName,"%s%s/%s/%s",prevSubDir[0],prevSubDir[1],prevSubDir[2],prevSubDir[3]);
                sd.chdir(strcat(strcat(strcat(strcat(strcat(strcat(fileName, "/"), prevSubDir[0]), "/"), prevSubDir[1]), "/"), prevSubDir[2]), true);
                break;
            default:
                //sprintf(fileName,"%s",prevSubDir[0]);
                sd.chdir("/", true);
            }
            //Return to prev Dir of the SD card.
            getMaxFile();
            currentFile = 1;
            seekFile(currentFile);
            while (digitalRead(btnStop) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }

        if (digitalRead(btnUp) == HIGH && start == 0)
        {
            //Move up a file in the directory
            upFile();
            while (digitalRead(btnUp) == HIGH)
            {
                //prevent button repeats by waiting until the button is released.
                delay(50);
            }
        }

        if (digitalRead(btnDown) == HIGH && start == 0)
        {
            //Move down a file in the directory
            downFile();
            while (digitalRead(btnDown) == HIGH)
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

    PlayBytes[0] = '\0';
    if (isDir == 1)
    {
        strcat_P(PlayBytes, PSTR("TZXDuino 1.9"));
    }
    else
    {
        strcat_P(PlayBytes, PSTR("Select File.."));
    }
    printtext(PlayBytes, 0);

    printtext(fileName, 1);
}

void stopFile()
{
    TZXStop();
    if (start == 1)
    {
        printtextF(PSTR("Stopped"), 0);
        start = 0;
    }
}

void playFile()
{
    if (isDir == 1)
    {
        changeDir();
    }
    else
    {
        if (entry.cwd()->exists(sfileName))
        {
            printtextF(PSTR("Playing         "), 0);
            pauseOn = 0;
            printtext(fileName, 1);
            currpct = 100;
            lcdsegs = 0;
            TZXPlay(sfileName); //Load using the short filename
            start = 1;
        }
        else
        {
            printtextF(PSTR("No File Selected"), 1);
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

void changeDir()
{
    //change directory, if fileName="ROOT" then return to the root directory
    //SDFat has no easy way to move up a directory, so returning to root is the easiest way.
    //each directory (except the root) must have a file called ROOT (no extension)

    if (!strcmp(fileName, "ROOT"))
    {
        subdir = 0;
        sd.chdir(true);
    }
    else
    {
        if (subdir > 0)
            entry.cwd()->getName(prevSubDir[subdir - 1], filenameLength); // Antes de cambiar
        sd.chdir(fileName, true);
        subdir++;
    }
    getMaxFile();
    currentFile = 1;
    seekFile(currentFile);
}

void printtextF(const char *text, int l)
{ //Print text to screen.
    lcd.setCursor(0, l);
    lcd.print(F("                    "));
    lcd.setCursor(0, l);
    lcd.print(reinterpret_cast<const __FlashStringHelper *>(text));
}

void printtext(const char *text, int l)
{ //Print text to screen.
    lcd.setCursor(0, l);
    lcd.print(F("                    "));
    lcd.setCursor(0, l);
    lcd.print(text);
}

void lcdTime()
{
    if (millis() - timeDiff2 > 1000)
    {                         // check switch every second
        timeDiff2 = millis(); // get current millisecond count

        if (lcdsegs % 10 != 0)
        {
            itoa(lcdsegs % 10, PlayBytes, 10);
            lcd.setCursor(15, 0);
            lcd.print(PlayBytes);
        } // ultima cifra 1,2,3,4,5,6,7,8,9
        else if (lcdsegs % 100 != 0)
        {
            itoa(lcdsegs % 100, PlayBytes, 10);
            lcd.setCursor(14, 0);
            lcd.print(PlayBytes);
        } // es 10,20,30,40,50,60,70,80,90,110,120,..
        else if (lcdsegs % 1000 != 0)
        {
            itoa(lcdsegs % 1000, PlayBytes, 10);
            lcd.setCursor(13, 0);
            lcd.print(PlayBytes);
        } // es 100,200,300,400,500,600,700,800,900,1100,..
        else
        {
            lcd.setCursor(13, 0);
            lcd.print("000");
        } // es 000,1000,2000,...

        lcdsegs++;
    }
}
