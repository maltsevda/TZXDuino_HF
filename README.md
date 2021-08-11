// ---------------------------------------------------------------------------------
// DO NOT USE CLASS-10 CARDS on this project - they're too fast to operate using SPI
// ---------------------------------------------------------------------------------
/*
 *                                    TZXduino
 *                             Written and tested by
 *                          Andrew Beer, Duncan Edwards
 *                          www.facebook.com/Arduitape/
 *
 *              Designed for TZX files for Spectrum (and more later)
 *              Load TZX files onto an SD card, and play them directly
 *              without converting to WAV first!
 *
 *              Directory system allows multiple layers,  to return to root
 *              directory ensure a file titles ROOT (no extension) or by
 *              pressing the Menu Select Button.
 *
 *              Written using info from worldofspectrum.org
 *              and TZX2WAV code by Francisco Javier Crespo
 *
 *              ***************************************************************
 *              Menu System:
 *                TODO: add ORIC and ATARI tap support, clean up code, sleep
 *
 *              V1.0
 *                Motor Control Added.
 *                High compatibility with Spectrum TZX, and Tap files
 *                and CPC CDT and TZX files.
 *
 *                V1.32 Added direct loading support of AY files using the SpecAY loader
 *                to play Z80 coded files for the AY chip on any 128K or 48K with AY
 *                expansion without the need to convert AY to TAP using FILE2TAP.EXE.
 *                Download the AY loader from http://www.specay.co.uk/download
 *                and load the LOADER.TAP AY file loader on your spectrum first then
 *                simply select any AY file and just hit play to load it. A complete
 *                set of extracted and DEMO AY files can be downloaded from
 *                http://www.worldofspectrum.org/projectay/index.htm
 *                Happy listening!
 *
 *                V1.8.1 TSX support for MSX added by Natalia Pujol
 *
 *                V1.8.2 Percentage counter and timer added by Rafael Molina Chesserot along with a reworking of the OLED1306 library.
 *                Many memory usage improvements as well as a menu for TSX Baud Rates and a refined directory controls.
 *
 *                V1.8.3 PCD8544 library changed to use less memory. Bitmaps added and Menu system reduced to a more basic level.
 *                Bug fixes of the Percentage counter and timer when using motor control/
 */

/**************************************************************
 *                  Casduino Menu Code:
 *  Menu Button (was motor controll button) opens menu
 *  up/down move through menu, play select, stop back
 *  Menu Options:
 *  Baud:
 *    1200
 *    2400
 *    2700
 *    3600
 *
 *  MotorControl:
 *    On
 *    Off
 *
 *  Save settings to eeprom on exit.
 */
