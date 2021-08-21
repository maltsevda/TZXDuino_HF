#ifndef __TZXFILE_H__
#define __TZXFILE_H__

#include <Arduino.h>

//TZX block list - uncomment as supported
#define ID10                0x10    //Standard speed data block
#define ID11                0x11    //Turbo speed data block
#define ID12                0x12    //Pure tone
#define ID13                0x13    //Sequence of pulses of various lengths
#define ID14                0x14    //Pure data block
#define ID15                0x15    //Direct recording block -- TBD - curious to load OTLA files using direct recording (22KHz)
//#define ID18                0x18    //CSW recording block
#define ID19                0x19    //Generalized data block
#define ID20                0x20    //Pause (silence) ot 'Stop the tape' command
#define ID21                0x21    //Group start
#define ID22                0x22    //Group end
#define ID23                0x23    //Jump to block
#define ID24                0x24    //Loop start
#define ID25                0x25    //Loop end
#define ID26                0x26    //Call sequence
#define ID27                0x27    //Return from sequence
#define ID28                0x28    //Select block
#define ID2A                0x2A    //Stop the tape is in 48K mode
#define ID2B                0x2B    //Set signal level
#define ID30                0x30    //Text description
#define ID31                0x31    //Message block
#define ID32                0x32    //Archive info
#define ID33                0x33    //Hardware type
#define ID35                0x35    //Custom info block
#define ID4B                0x4B    //Kansas City block (MSX/BBC/Acorn/...)
#define IDPAUSE				      0x59	  //Custom Pause processing
#define ID5A                0x5A    //Glue block (90 dec, ASCII Letter 'Z')
#define ORIC                0xFA    //Oric Tap File
#define AYO                 0xFB    //AY file
#define ZXO                 0xFC    //ZX80 O file
#define ZXP                 0xFD    //ZX81 P File
#define TAP                 0xFE    //Tap File Mode
#define EOF                 0xFF    //End of file

#define UEF                 0xFA    //UEF file
// UEF chunks
#define ID0000              0x0000
#define ID0100              0x0100
#define ID0104              0x0104 // defined tape format data block: data bits per packet/parity/stop bits
#define ID0110              0x0110
#define ID0111              0x0111 // carrier tone (previously high tone) with dummy byte at byte
#define ID0112              0x0112
#define ID0114              0x0114 // Security Cycles replaced with carrier tone
#define ID0116              0x0116 // floating point gap: cycles = floatGap * this.baud
#define ID0117              0x0117 // data encoding format change for 300 bauds
#define IDCHUNKEOF          0xffff

//TZX File Tasks
#define GETFILEHEADER         0
#define GETID                 1
#define PROCESSID             2
#define GETAYHEADER           3
#define GETUEFHEADER          4
#define GETCHUNKID            5
#define PROCESSCHUNKID        6

//TZX ID Tasks
#define READPARAM             0
#define PILOT                 1
#define SYNC1                 2
#define SYNC2                 3
#define DATA                  4
#define PAUSE                 5
#define HEADER                6
#define NAME                  7
#define GAP                   8
#define SYNCLAST              9
#define NAMELAST              10

//Spectrum Standards
#define PILOTLENGTH           619
#define SYNCFIRST             191
#define SYNCSECOND            210
#define ZEROPULSE             244
#define ONEPULSE              489
#define PILOTNUMBERL          8063
#define PILOTNUMBERH          3223
#define PAUSELENGTH           1000

// UEF stuff
// For 1200 baud zero is 416us, one is 208us
// For 1500 baud zero is 333us, one is 166us
// For 1550 baud zero is 322us, one is 161us
// For 1600 baud zero is 313us, one is 156us

#define TURBOBAUD1500

// 1200 baud UEF
#define UEFPILOTPULSES                (outWord << 2)
#define UEFPILOTLENGTH                208
#define UEFZEROPULSE                  416
#define UEFONEPULSE                   208

#ifdef TURBOBAUD1500
#define UEFTURBOPILOTPULSES           320
#define UEFTURBOPILOTLENGTH           166
#define UEFTURBOZEROPULSE             333
#define UEFTURBOONEPULSE              166
#endif

#ifdef TURBOBAUD1550
#define UEFTURBOPILOTPULSES           320
#define UEFTURBOPILOTLENGTH           161
#define UEFTURBOZEROPULSE             322
#define UEFTURBOONEPULSE              161
#endif

#ifdef TURBOBAUD1600
#define UEFTURBOPILOTPULSES           320
#define UEFTURBOPILOTLENGTH           156
#define UEFTURBOZEROPULSE             313
#define UEFTURBOONEPULSE              156
#endif

//ZX81 Standards
#define ZX80PULSE                 160
#define ZX80TURBOPULSE            120
#define ZX80BITGAP                1442
#define ZX80TURBOBITGAP           500
//ZX81 Pulse Patterns - Zero Bit  - HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, GAP
//                    - One Bit   - HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, LOW, HIGH, GAP

// AY Header offset start
#define HDRSTART              0

// Oric parameters
#define ORICZEROPULSE     416
#define ORICZEROLOWPULSE  208
#define ORICZEROHIGHPULSE 416
#define ORICONEPULSE      208
#define ORICONELOWPULSE   208
#define ORICONEHIGHPULSE  208

// Temp Files

void checkForEXT();
void TZXProcess();
void wave();
void StandardBlock();
void writeData();
void writeData4B();
void writeHeader();
void ZX81FilenameBlock();
void ZX8081DataBlock();
void ZX80ByteWrite();
void PureToneBlock();
void PureDataBlock();
void PulseSequenceBlock();
void ReadTZXHeader();
void ReadAYHeader();
void ReadUEFHeader();
void UEFCarrierToneBlock();
void writeUEFData();
void OricDataBlock();
void OricBitWrite();
void DirectRecording();
void writeSampleData();

// temp temp files

bool isFileStopped();
void stopFile();
void printError(const __FlashStringHelper *sz);

#endif // __TZXFILE_H__