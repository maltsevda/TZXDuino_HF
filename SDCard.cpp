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

void resetCurrentFileInfo()
{
    *fileName = 0;
    *shortFileName = 0;
    fileSize = 0;
    fileIsDir = false;
}

bool isRootPath(const char* path)
{
    return path[0] == '/' && path[1] == 0;
}

bool changeDir(const char* path, bool updateWorkDir)
{
    resetCurrentFileInfo();

    if (path[0] == 0 || isRootPath(path))
    {
        // switch to root folder
        sd.chdir(true);
        FatFile::cwd()->rewind();
        if (updateWorkDir)
            strcpy(workDir, "/");
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
                    strncpy(workDir, path, SD_MAX_WORKDIR - 1);
                else
                {
                    // TODO: unsafe - out of range SD_MAX_WORKDIR
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
    else
        resetCurrentFileInfo();
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
    if (isRootPath(workDir))
        return false;

    // workDir looks like '/sub_folder/subsub_folder1/'
    // we need to find previous slash ↑ not the last ↑
    uint8_t index = 0;
    for (uint8_t i = 0; workDir[i]; ++i)
    {
        if (workDir[i] == '/' && workDir[i + 1])
            index = i + 1;
    }
    workDir[index] = 0;

    return changeDir(workDir, false);
}

bool nextFile()
{
    uint32_t goodPosition = FatFile::cwd()->curPosition();
    if (file.openNext(FatFile::cwd(), O_READ))
    {
        readCurrentFileInfo();
        file.close();
        return true;
    }
    else
        FatFile::cwd()->seekSet(goodPosition);
    return false;
}

bool prevFile()
{
    bool fileNotFound = true;
    uint32_t goodPosition = FatFile::cwd()->curPosition();
    uint16_t index = goodPosition / 32;

    // skip self index
    if (index > 0)
        --index;

    while (index > 0)
    {
        --index;
        if (file.open(FatFile::cwd(), index, O_READ))
        {
            fileNotFound = false;
            readCurrentFileInfo();
            file.close();
            break;
        }
    }

    if (fileNotFound)
        FatFile::cwd()->seekSet(goodPosition);

    return !fileNotFound;
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

// I am not sure with bytes alignment, so I left the old code for WORD, LONG and DWORD

int readByte()
{
    int i = 0;
    if (file.seekSet(bytesRead))
    {
        i = file.read(&outByte, 1);
        if (i == 1)
            bytesRead += 1;
    }
    return i;
}

int readWord()
{
    int i = 0;
    byte out[2];
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 2);
        if (i == 2)
        {
            outWord = word(out[1], out[0]);
            bytesRead += 2;
        }
    }
    return i;
}

int readLong()
{
    int i = 0;
    byte out[3];
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 3);
        if (i == 3)
        {
            outLong = (word(out[2], out[1]) << 8) | out[0];
            bytesRead += 3;
        }
    }
    return i;
}

int readDword()
{
    int i = 0;
    byte out[4];
    if (file.seekSet(bytesRead))
    {
        i = file.read(out, 4);
        if (i == 4)
        {
            outLong = (word(out[3], out[2]) << 16) | word(out[1], out[0]);
            bytesRead += 4;
        }
    }
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
