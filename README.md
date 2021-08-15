## CREDITS

### TZXduino

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

* Menu System:\
TODO: add ORIC and ATARI tap support, clean up code, sleep

* V1.0\
Motor Control Added.
High compatibility with Spectrum TZX, and Tap files
and CPC CDT and TZX files.

* V1.32\
Added direct loading support of AY files using the SpecAY loader
to play Z80 coded files for the AY chip on any 128K or 48K with AY
expansion without the need to convert AY to TAP using FILE2TAP.EXE.
Download the AY loader from http://www.specay.co.uk/download
and load the LOADER.TAP AY file loader on your spectrum first then
simply select any AY file and just hit play to load it. A complete
set of extracted and DEMO AY files can be downloaded from
http://www.worldofspectrum.org/projectay/index.htm
Happy listening!

* V1.8.1\
TSX support for MSX added by Natalia Pujol

* V1.8.2\
Percentage counter and timer added by Rafael Molina Chesserot along with a reworking of the OLED1306 library.
Many memory usage improvements as well as a menu for TSX Baud Rates and a refined directory controls.

* V1.8.3\
PCD8544 library changed to use less memory. Bitmaps added and Menu system reduced to a more basic level.
Bug fixes of the Percentage counter and timer when using motor control.

* V1.11\
Added unzipped UEF playback and turbo UEF to the Menu thatks to the kind work by kernal@kernalcrash.com
Supports Gunzipped UEFs only.

* v1.13\
HQ.UEF support added by Rafael Molina Chesserot of Team MAXDuino
v1.13.1 Removed digitalWrite in favour of a macro suggested by Ken Forster
Some MSX games will now work at higher Baudrates than before.

* v1.13.2\
Added a fix to some Gremlin Loaders which reverses the polarity of the block.
New menu Item added. "Gremlin Loader" turned on will allow Samurai Trilogy and Footballer of the Year II
CDTs to load properly.

* 1.14\
ID15 code adapted from MAXDuino by Rafael Molina Chasserot.
Not working 100% with CPC Music Loaders but will work with other ID15 files.

* 1.14.2\
Added an ID15 switch to Menu as ID15 being enabled was stopping some files loading properly.

* 1.14.3\
Removed the switch in favour of an automatic system of switching ID15 routine on and off.

* 1.15\
Added support for the Surenoo RGB Backlight LCD using an adapted version of the Grove RGBLCD library.
Second counter not currently working. Also some memory saving tweaks.

* 1.15.3\
Adapted the MAXDuino ID19 code and TurboMode for ZX80/81
Also added UEF Chunk 117 which allows for differing baudrates in BBC UEFs.
Added a Spectrum Font for OLED 1306 users converted by Brendan Alford
Added File scrolling by holding up or down buttons. By Brendan Alford.

* 1.16\
Fixed a bug that was stopping Head Over Heels (and probably others)loading on +2 Spectrum. Seems to have made
ZX80/81 playback alot more stable too.

* 1.17\
Added ORIC TAP file playback from the Maxduino team.

### Casduino Menu Code

Menu Button (was motor controll button) opens menu up/down move through menu,
play select, stop back Menu Options:

*Baud:*
* 1200
* 2400
* 2700
* 3600

*MotorControl:*
* On
* Off

Save settings to eeprom on exit.

### EncButton

Ультра лёгкая и быстрая библиотека для энкодера, энкодера с кнопкой или просто кнопки

Документация:\
GitHub: https://github.com/GyverLibs/EncButton

Возможности:
- Максимально быстрое чтение пинов для AVR (ATmega328/ATmega168, ATtiny85/ATtiny13)
- Оптимизированный вес
- Быстрые и лёгкие алгоритмы кнопки и энкодера
- Энкодер: поворот, нажатый поворот, быстрый поворот, счётчик
- Кнопка: антидребезг, клик, несколько кликов, счётчик кликов, удержание, режим step
- Подключение - только HIGH PULL!
- Опциональный режим callback (+22б SRAM на каждый экземпляр)

```
    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License
```

**Версии:**
* v1.1 - пуллап отдельныи методом
* v1.2 - можно передать конструктору параметр INPUT_PULLUP / INPUT(умолч)
* v1.3 - виртуальное зажатие кнопки энкодера вынесено в отдельную функцию + мелкие улучшения
* v1.4 - обработка нажатия и отпускания кнопки
* v1.5 - добавлен виртуальный режим
* v1.6 - оптимизация работы в прерывании
* v1.6.1 - PULLUP по умолчанию
* v1.7 - большая оптимизация памяти, переделан FastIO
* v1.8 - индивидуальная настройка таймаута удержания кнопки (была общая на всех)
