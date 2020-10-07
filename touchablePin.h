//
//  touchablePin.h
//  Version 1.0
//
//  Created by David Elvig on 1/26/17.
//  Based on Paul Stofregren's touchRead.c (see .cpp file for reference
//
/********************************************************************************************************************
 Capacitive Touch pins are charged to get their full capacitance, and the time-to-charge is proportional the capacitance.
 touchablePin works by assigning an untouched timing value with a call to init()
 isTouched() then returns true as soon as the time to charge exceeds (untouchedDuration * _touchedFactor)
 ... not waiting for the pin to get fully charged.
 
 The default value for DEFAULT_TOUCHED_FACTOR is 1.3, and can be adjusted in the #define below based on trial and error against your capacitive touch pin connected hardware.
 
 touchablePin(void);                    // be sure to call setPin() before calling isTouched()
 touchablePin(uint8_t);                // sets a pin number on instantiation
 touchablePin(uint8_t, float);       // sets a pin number and an alternative DEFAULT_TOUCHED_FACTOR
                        // Appropriately small _maxFactors lead to faster isTouched() return times.
                        // Too small _maxFactor will lead to false positives for isTouched().
 
 To be useful, the pin must be untouched on start-up (or rather, when initUntouched() is called).
 
 Use the touchablePin.printPin() method to experiment with your setup.
 It will return the same value as the unmodified tpTouchPin() to check the ratio of your untouched and touched states.
 
 TODO: The first call to the private method init() returns a too-small capacitance value (and it happens too fast)
 The work-around is to call init() twice.  The To Do item is to figure out why (is there some pre-existing
 charge on the pin?), and eliminate the extra call or document the reasoning.
*********************************************************************************************************************/
#ifndef touchablePin_h
#define touchablePin_h

#define DEFAULT_TOUCHED_FACTOR 1.3f

#include <stdio.h>
#include <arduino.h>

class touchablePin {
public:
    touchablePin(void);
    touchablePin(uint8_t pin);
    touchablePin(uint8_t pin, float touchedFactor);

    void    initUntouched(void);
    bool    isTouched(void); // requires two immediately-consecutive positive tpTouchRead()'s
    int     tpTouchRead(void);
    int     touchReadTeensy(void) {   return(touchRead(pinNumber));    }
    void    setPin(uint8_t pin);
    void    setTouchedFactor(float factor) {_touchedFactor = factor;  init();  };
    void    printPin(void);
    int     pinNumber       = -1;
    
    unsigned long   firstUntouchedDuration  = 0,
                    untouchedValue          = 0,
                    untouchedDuration       = 0,
                    lastTouchedValue        = 0,
                    lastTouchedDuration     = 0,
                    touchedTargetDuration   = 0;
    unsigned long   startTouchTime          = 0,
                    endTouchTime            = 0;

private:
    bool    _isTouched = false;
    void    init(void);
    bool    initialized = false;
    float   _touchedFactor = DEFAULT_TOUCHED_FACTOR;
};

#endif /* touchablePin_h */
