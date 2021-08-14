#ifndef __SDCARD_H__
#define __SDCARD_H__

#include <Arduino.h>

bool setupSD(uint8_t csPin);

bool childDir();
bool parentDir();
void nextFile();
void prevFile();

bool openFile();
void closeFile();

bool isDir();
bool isFileExists();
bool checkFileExt(const char* szExt);

const char* getFileName();
uint32_t getFileSize();

int readByte();
int readWord();
int readLong();
int readDword();
bool checkFileHeader(const void* headerPtr, size_t sizeToRead, size_t sizeToCheck);

// direct access for TZXFile.cpp (bad solution)

extern unsigned long bytesRead;
extern byte outByte;
extern word outWord;
extern unsigned long outLong;

#endif // __SDCARD_H__