/*
*  touchablePin.cpp
*  Version 1.0 - David Elvig on 1/2017
*  Refactored 10/2020
* based on Paul Stofregren's touchRead.c from ...
* ------------------------------------------------------------------------
* Teensyduino Core Library
* http://www.pjrc.com/teensy/
* Copyright (c) 2013 PJRC.COM, LLC.
* ------------------------------------------------------------------------
 */
#include "touchablePin.h"
#include "core_pins.h"

#if defined(HAS_KINETIS_TSI) || defined(HAS_KINETIS_TSI_LITE)

#if defined(__MK20DX128__) || defined(__MK20DX256__)
// These settings give approx 0.02 pF sensitivity and 1200 pF range
// Lower current, higher number of scans, and higher prescaler
// increase sensitivity, but the trade-off is longer measurement
// time and decreased range.
#define CURRENT   2 // 0 to 15 - current to use, value is 2*(current+1)
#define NSCAN     9 // number of times to scan, 0 to 31, value is nscan+1
#define PRESCALE  2 // prescaler, 0 to 7 - value is 2^(prescaler+1)
static const uint8_t pin2tsi[] = {
    //0    1    2    3    4    5    6    7    8    9
    9,  10, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255,  13,   0,   6,   8,   7,
    255, 255,  14,  15, 255,  12, 255, 255, 255, 255,
    255, 255,  11,   5
};

#elif defined(__MK66FX1M0__)
#define NSCAN     9
#define PRESCALE  2
static const uint8_t pin2tsi[] = {
    //0    1    2    3    4    5    6    7    8    9
    9,  10, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255,  13,   0,   6,   8,   7,
    255, 255,  14,  15, 255, 255, 255, 255, 255,  11,
    12, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

#elif defined(__MKL26Z64__)
#define NSCAN     9
#define PRESCALE  2
static const uint8_t pin2tsi[] = {
    //0    1    2    3    4    5    6    7    8    9
    9,  10, 255,   2,   3, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255,  13,   0,   6,   8,   7,
    255, 255,  14,  15, 255, 255, 255
};

#endif // __MK....

touchablePin::touchablePin() {
    pinNumber = -1;
    init();
}

touchablePin::touchablePin(uint8_t pin) {
    pinNumber = pin;
    init();
}

touchablePin::touchablePin(uint8_t pin, float touchedFactor) {
    pinNumber = pin;
    if (touchedFactor > 0)  _touchedFactor  = touchedFactor;
    init();
}

// return the time to charge the pin, up to a maximum of touchedTargetDuration
int touchablePin::tpTouchRead(void)
{
    unsigned long touchedTargetTime = 0;
    
    uint32_t ch;
    if (pinNumber >= NUM_DIGITAL_PINS) {
        Serial.printf("PinNumber (%d) > NUM_DIGITAL_PINS (%d) in tpTouchRead\n", pinNumber, NUM_DIGITAL_PINS);
        return -1;
    }
    ch = pin2tsi[pinNumber];
    if (ch == 255) {
        Serial.printf("PinNumber (%d) does not match to a touch pin in tpTouchRead\n", pinNumber);
        return -2;
    }
    *portConfigRegister(pinNumber) = PORT_PCR_MUX(0);
    SIM_SCGC5 |= SIM_SCGC5_TSI;

#if defined(HAS_KINETIS_TSI)  // Teensy 3.2 takes this path
    TSI0_GENCS = 0;
    TSI0_PEN = (1 << ch);
    TSI0_SCANC = TSI_SCANC_REFCHRG(3) | TSI_SCANC_EXTCHRG(CURRENT);
    TSI0_GENCS = TSI_GENCS_NSCN(NSCAN) | TSI_GENCS_PS(PRESCALE) | TSI_GENCS_TSIEN | TSI_GENCS_SWTS;
    #define KINETIS_TOUCH_RETURN_VALUE  *((volatile uint16_t *)(&TSI0_CNTR1) + ch)
#elif defined(HAS_KINETIS_TSI_LITE)
    TSI0_GENCS = TSI_GENCS_REFCHRG(4) | TSI_GENCS_EXTCHRG(3) | TSI_GENCS_PS(PRESCALE)
    | TSI_GENCS_NSCN(NSCAN) | TSI_GENCS_TSIEN | TSI_GENCS_EOSF;
    TSI0_DATA = TSI_DATA_TSICH(ch) | TSI_DATA_SWTS;
    #define KINETIS_TOUCH_RETURN_VALUE  TSI0_DATA & 0xFFFF
#endif
    
    startTouchTime = micros();
    delayMicroseconds(10);
    if (!initialized) {
        while (TSI0_GENCS & TSI_GENCS_SCNIP)
            ; // wait
        delayMicroseconds(1);
        untouchedValue = KINETIS_TOUCH_RETURN_VALUE;
        untouchedDuration = micros() - startTouchTime;
        if (untouchedDuration < 0) { // if there has been a roll-over in micros()
            Serial.printf("Initialization of pinNumber %d) returned an unexpectedly negative untouchedDuration of %d\n", pinNumber, untouchedDuration);
            return(-3); // expect that a failed init() will be re-called;
        } else {
            touchedTargetDuration = untouchedDuration * _touchedFactor;
            initialized = true;
            _isTouched = false;
            return(untouchedDuration);
        }
    } else {
        touchedTargetTime = startTouchTime + touchedTargetDuration;
        if (touchedTargetTime < startTouchTime) {
            Serial.println("rollover in touchablePin::tpTouchRead() lastTouchedDuration and _isTouched will be unreliable");
        }
        while ((TSI0_GENCS & TSI_GENCS_SCNIP) && (micros() < touchedTargetTime))
            ;   // wait until fully charged (TSI0_GENCS & TSI_GENCS_SCNIP) or exceeding touchedTargetTime
                // in the former case, KINETIS_TOUCH_RETURN_VALUE will be proportional to capacitance
                // in the latter case, KINETIS_TOUCH_RETURN_VALUE appears to return zero (as assigned to lastTouchedValue below
        delayMicroseconds(1);
        lastTouchedValue = KINETIS_TOUCH_RETURN_VALUE;
        lastTouchedDuration = micros() - startTouchTime;
        _isTouched = (lastTouchedDuration >= touchedTargetDuration);
        return(lastTouchedDuration);
    }
}

void touchablePin::init(void) {
    //    first call to tpTouchRead() returns too quickly TODO: find out why.
    //    work-around - call it twice... second calling sets appropriate values
    
    if (pinNumber != -1) {
        tpTouchRead();
        initialized = false;
        tpTouchRead();
    }
}

void touchablePin::setPin(uint8_t pin) {
    pinNumber = pin;
    initialized = false;
    init();
}

bool touchablePin::isTouched(void) {
    tpTouchRead();
    if (_isTouched) {
        tpTouchRead(); // assure two in a row for true "_isTouched"
    }
    return(_isTouched);
}

void touchablePin::printPin(void) {
    int tpTR = tpTouchRead();
    unsigned long startTouchRead = micros();
    int tr = touchRead(pinNumber);
    unsigned long endTouchRead = micros();

    if (_isTouched) { // keep in mind the tpTouchRead musrt have been recently called for _isTouched to reflect the current state
        Serial.printf("%d uS - TOUCHED\n", lastTouchedDuration);
    } else {
        Serial.printf(  "%d uS untouched\n", lastTouchedDuration);
        Serial.printf(  "\tNew Value:    %d\t", lastTouchedValue);
        Serial.printf(  "(Base Value:    %d)\t", untouchedValue);
        Serial.printf(  "Value Ratio:    %3.3f\n", (float)lastTouchedValue / (float)untouchedValue);
    }
    Serial.printf(      "\tNew Duration: %d uS\t", lastTouchedDuration);
    Serial.printf(      "(Base Duration: %d uS)\t", untouchedDuration);
    Serial.printf(      "Duration Ratio: %3.3f\n", (float)lastTouchedDuration / (float)untouchedDuration);
    
    Serial.printf(      "\tPin: %d\t", pinNumber);
    if (initialized)    Serial.print("initialized\n");
    else                Serial.print("not initialized\n");

    Serial.printf("\ttpTouchRead: %d in %d uS\t", tpTR, lastTouchedDuration);
    Serial.printf("touchRead()): %d in %d uS\n", tr, endTouchRead - startTouchRead);
}

#endif // HAS_KINETIS_TSI) || defined(HAS_KINETIS_TSI_LITE
       // NOTE: The final #else condition in touchRead.c has been eliminated here.
       //       In that #else case, I expect this code will throw compile time errors.


