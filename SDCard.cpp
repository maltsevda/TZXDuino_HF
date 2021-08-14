#include "SDCard.h"
#include <SdFat.h>

SdFat sd;
FatFile file;

// File System Info

#define SD_MAX_WORKDIR      128
#define SD_MAX_FILENAME     32

char workDir[SD_MAX_WORKDIR] = { '/' };
char fileName[SD_MAX_FILENAME];
char shortFileName[13];
uint32_t fileSize;
bool fileIsDir;

// TXZFile Interface

unsigned long bytesRead;
byte outByte;
word outWord;
unsigned long outLong;

// Private Functions

bool changeDir(const char* path, bool updateWorkDir)
{
    if (path[0] == 0 || (path[0] == '/' && path[1] == 0))
    {
        // switch to root folder
        sd.chdir(true);
        FatFile::cwd()->rewind();
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
            FatFile::cwd()->rewind();
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

void readCurrentFileInfo()
{
    if (file.isOpen())
    {
        file.getName(fileName, SD_MAX_FILENAME);
        file.getSFN(shortFileName);
        fileSize = file.fileSize();
        fileIsDir = file.isDir();
    }
}

// Public Functions

bool setupSD(uint8_t csPin)
{
    pinMode(csPin, OUTPUT);
    if (sd.begin(csPin, SPI_FULL_SPEED))
    {
        changeDir("/", true);
        return true;
    }
    return false;
}

bool childDir()
{
    return changeDir(shortFileName, true);
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

void nextFile()
{
    if (file.openNext(FatFile::cwd(), O_READ))
    {
        readCurrentFileInfo();
        file.close();
    }
}

void prevFile()
{
    uint16_t index = FatFile::cwd()->curPosition() / 32;

    // skip self index
    if (index > 0)
        --index;

    while (index > 0)
    {
        --index;
        if (file.open(FatFile::cwd(), index, O_READ))
        {
            readCurrentFileInfo();
            file.close();
            break;
        }
    }
}

bool openFile()
{
    return file.open(shortFileName, O_READ);
}

void closeFile()
{
    file.close();
}

bool isDir()
{
    return fileIsDir;
}

bool isFileExists()
{
    return FatFile::cwd()->exists(shortFileName);
}

bool checkFileExt(const char* szExt)
{
    char* szFileExt = strrchr(shortFileName, '.');
    if (szFileExt)
        return strcasecmp_P(szFileExt, szExt) == 0;
    return false;
}

const char* getFileName()
{
    return fileName;
}

uint32_t getFileSize()
{
    return fileSize;
}

//Read a DATA from the file, and move file position on N bytes if successful

int readByte()
{
    byte out[1];
    int i = 0;
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 1);
        if (i == 1)
            bytesRead += 1;
    }
    outByte = out[0];
    //blkchksum = blkchksum ^ out[0];
    return i;
}

int readWord()
{
    byte out[2];
    int i = 0;
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 2);
        if (i == 2)
            bytesRead += 2;
    }
    outWord = word(out[1], out[0]);
    //blkchksum = blkchksum ^ out[0] ^ out[1];
    return i;
}

int readLong()
{
    byte out[3];
    int i = 0;
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 3);
        if (i == 3)
            bytesRead += 3;
    }
    outLong = (word(out[2], out[1]) << 8) | out[0];
    //blkchksum = blkchksum ^ out[0] ^ out[1] ^ out[2];
    return i;
}

int readDword()
{
    byte out[4];
    int i = 0;
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 4);
        if (i == 4)
            bytesRead += 4;
    }
    outLong = (word(out[3], out[2]) << 16) | word(out[1], out[0]);
    //blkchksum = blkchksum ^ out[0] ^ out[1] ^ out[2] ^ out[3];
    return i;
}

bool checkFileHeader(const void* headerPtr, size_t sizeToRead, size_t sizeToCheck)
{
    char buf[16] = { 0 };
    if (file.seekSet(0))
    {
        bytesRead = file.read(buf, sizeToRead);
        return memcmp_P(buf, headerPtr, sizeToCheck) == 0;
    }
    return false;
}
