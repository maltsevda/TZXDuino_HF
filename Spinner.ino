#include <LiquidCrystal.h>
#include "TZXDuino.h"

extern LiquidCrystal lcd;
const char indicators[] = {'|', '/', '-', 0};

void lcdSpinner()
{
    if (millis() - timeDiff2 > 1000)
    {                         // check switch every 50ms
        timeDiff2 = millis(); // get current millisecond count

        lcd.setCursor(15, 0);
        lcd.print(indicators[spinpos]);

        ++spinpos;
        if (spinpos > 3)
        {
            spinpos = 0;
        }
    }
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
