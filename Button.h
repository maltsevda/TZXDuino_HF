#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <Arduino.h>
#include "FastIO_v2.h"

#define EB_DEB      80      // дебаунс кнопки
#define EB_HOLD     1000    // таймаут удержания кнопки
#define EB_STEP     500     // период срабатывания степ

// флаг макро
#define _setFlag(x) (flags |= 1 << x)
#define _clrFlag(x) (flags &= ~(1 << x))
#define _readFlag(x) ((flags >> x) & 1)

// константы
#define EB_NO_PIN 255

// класс
template <uint8_t _PIN = EB_NO_PIN>
class Button
{
public:
    // можно указать режим работы пина
    Button(const uint8_t mode = INPUT_PULLUP)
    {
        if (_PIN < 252)
            pinMode(_PIN, mode);
    }

    // установить таймаут удержания кнопки для isHold(), мс (до 30 000)
    void setHoldTimeout(int tout)
    {
        _holdT = tout >> 7;
    }

    // тикер, вызывать как можно чаще или в прерывании
    // вернёт отличное от нуля значение, если произошло какое то событие
    uint8_t tick()
    {
        if (!_isrFlag)
        {
            _isrFlag = 1;

            // обработка кнопки (компилятор вырежет блок если не используется)
            // если S2 не указан (кнопка) или указан KEY или выбран вирт. энкодер с кнопкой или кнопка
            if (_PIN < 252)
            {
                _btnState = F_fastRead(_PIN);    // обычная кнопка
                poolBtn();
            }
        }
        _isrFlag = 0;
        return EBState;
    }

    // получить статус
    uint8_t getState() { return EBState; }

    // сбросить статус
    void resetState() { EBState = 0; }

    // кнопка нажата
    bool isPress() { return checkState(8); }
    bool press() { return isPress(); }

    // кнопка отпущена
    bool isRelease() { return checkState(9); }
    bool release() { return isRelease(); }

    // клик по кнопке
    bool isClick() { return checkState(5); }
    bool click() { return isClick(); }

    // кнопка удержана
    bool isHolded() { return checkState(6); }

    // кнопка удержана (грамотный аналог holded =)
    bool isHeld() { return isHolded(); }
    bool held() { return isHolded(); }

    // кнопка удерживается
    bool isHold() { return _readFlag(4); }
    bool hold() { return isHold(); }

    // режим импульсного удержания
    bool isStep() { return checkState(7); }
    bool step() { return isStep(); }

private:

    void poolBtn()
    {
        unsigned long thisMls = millis();
        unsigned long debounce = thisMls - _debTimer;
        if (_btnState) {                                                	// кнопка нажата
            if (!_readFlag(3)) {                                          	// и не была нажата ранее
                if (debounce > EB_DEB) {                                   	// и прошел дебаунс
                    _setFlag(3);                                            // флаг кнопка была нажата
                    _debTimer = thisMls;                                    // сброс таймаутов
                    EBState = 8;                                           	// кнопка нажата
                }
            } else {                                                      	// кнопка уже была нажата
                if (!_readFlag(4)) {                                        // и удержание ещё не зафиксировано
                    if (debounce < (_holdT << 7)) {                         // прошло меньше удержания
                        // if (EBState != 0 && EBState != 8) _setFlag(2);      // но энкодер повёрнут! Запомнили
                    } else {                                                // прошло больше времени удержания
                        if (!_readFlag(2)) {                                // и энкодер не повёрнут
                            EBState = 6;                                   	// значит это удержание (сигнал)
                            _setFlag(4);                                    // запомнили что удерживается
                            _debTimer = thisMls;                            // сброс таймаута
                        }
                    }
                } else {                                                    // удержание зафиксировано
                    if (debounce > EB_STEP) {                              	// таймер степа
                        EBState = 7;                                       	// сигналим
                        _debTimer = thisMls;                                // сброс таймаута
                    }
                }
            }
        } else {                                                        	// кнопка не нажата
            if (_readFlag(3)) {                                           	// но была нажата
                if (debounce > EB_DEB && !_readFlag(4) && !_readFlag(2)) {	// энкодер не трогали и не удерживали - это клик
                    EBState = 5;
                } else EBState = 9;                                         // кнопка отпущена
                flags &= ~0b00011100;                                       // clear 2 3 4
                _debTimer = thisMls;                                        // сброс таймаута
            }
        }
    }

    bool checkState(uint8_t val)
    {
        if (EBState == val)
        {
            EBState = 0;
            return 1;
        }
        return 0;
    }

    unsigned long _debTimer = 0;
    uint8_t _lastState = 0, EBState = 0;
    bool _btnState = 0, _isrFlag = 0;
    uint8_t flags = 0;
    uint8_t _holdT = EB_HOLD >> 7;

    // flags
    // 0 - enc turn
    // 1 - enc fast
    // 2 - enc был поворот
    // 3 - флаг кнопки
    // 4 - hold
    // 5 - clicks flag
    // 6 - clicks get
    // 7 - enc button hold

    // EBState
    // 0 - idle
    // 1 - right
    // 2 - left
    // 3 - rightH
    // 4 - leftH
    // 5 - click
    // 6 - holded
    // 7 - step
    // 8 - press
    // 9 - release
};

#endif