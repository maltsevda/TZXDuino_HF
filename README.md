![Final Result](./Images/FinalResult.jpg "Final Result")

# ОПИСАНИЕ

Рефакторинг проекта TZXduino от Andrew Beer и Duncan Edwards. Особенности моей
версии - hardware часть построена на готовых модулях с Aliexpress. Полный
рефакторинг всего кода, кроме части отвественной за формат файла TZX.

Разводку платы, необходимые компоненты и gerber-файлы можно найти в этом
репозитории:
[https://github.com/maltsevda/TZXDuino_HFHW](https://github.com/maltsevda/TZXDuino_HFHW)

# CREDITS

### TZXduino 1.18

```
Written and tested by
Andrew Beer, Duncan Edwards
www.facebook.com/Arduitape
```

Designed for TZX files for Spectrum (and more later) Load TZX files onto an SD
card, and play them directly without converting to WAV first! Directory system
allows multiple layers,  to return to root directory ensure a file titles ROOT
(no extension) or by pressing the Menu Select Button. Written using info from
worldofspectrum.org and TZX2WAV code by Francisco Javier Crespo

### EncButton 1.8

GitHub: https://github.com/GyverLibs/EncButton

```
AlexGyver, alex@alexgyver.ru
https://alexgyver.ru/
MIT License
```

Ультра лёгкая и быстрая библиотека для энкодера, энкодера с кнопкой или просто
кнопки.

### SdFat 1.1.4

GitHub: https://github.com/greiman/SdFat

```
Bill Greiman <fat16lib@sbcglobal.net>
Copyright (c) 2011..2017 Bill Greiman
MIT License
```

The Arduino SdFat library provides read/write access to FAT16/FAT32
file systems on SD/SDHC flash cards.

### TimerOne 1.1.0

GitHub: https://github.com/PaulStoffregen/TimerOne

```
Paul Stoffregen
http://www.pjrc.com/teensy/
CC BY 3.0 US
```

TimerOne is free software. You can redistribute it and/or modify it under the
terms of Creative Commons Attribution 3.0 United States License.

Paul Stoffregen's modified TimerOne. This version provides 2 main benefits:
* Optimized inline functions - much faster for the most common usage
* Support for more boards (including ATTiny85 except for the PWM functionality)

### Liquid Crystal 1.0.7

URL: http://www.arduino.cc/en/Reference/LiquidCrystal

```
Arduino <info@arduino.cc>
Copyright (C) 2006-2008 Hans-Christoph Steiner. All rights reserved.
Copyright (c) 2010 Arduino LLC. All right reserved.
```

This library allows an Arduino/Genuino board to control LiquidCrystal displays
(LCDs) based on the Hitachi HD44780 (or a compatible) chipset, which is found
on most text-based LCDs. The library works with in either 4 or 8 bit mode (i.e.
using 4 or 8 data lines in addition to the rs, enable, and, optionally, the rw
control lines).
