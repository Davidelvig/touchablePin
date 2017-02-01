//
//  touchablePin.h
//  Version 1.0
//
//  Created by David Elvig on 1/26/17.
//  Based on Paul Stofregren's touchRead.c (see .cpp file for reference
//
/********************************************************************************************************************
Capacitive Touch pins are charged to get their capacitance, and the time-to-charge is proportional the capacitance.
touchablePin works by assigning an untouched timing value with a call to initUntouched()

isTouched() then returns true as soon as the time to charge exceeds (untouchedTime * _maxfactor)
... not waiting for the pin to get fully charged.
 
The default value for MAX_FACTOR can be adjusted in the #define below based on trial and error against your
capacitive touch pin connected hardware.
It can also me adjusted in the second version of the constructor with a second maxFactor parameter.

Appropriately small _maxFactors lead to faster isTouched() return times.
Too small _maxFactor will lead to false positives for isTouched().
 
To be useful, the pin must be untouched on start-up (or rather, when initUntouched() is called)

Use the touchablePin.touchRead() method to experiment with your setup.
It will return the same value as the unmodified touchPin() to check the ratio of your untouched and touched states.

TODO: The first call to the private method init() returns a too-small capacitive   value (and it happens too fast)
      The work-around is to call init() twice.  The To Do item is to figure out why (it there some pre-existing
      charge on the pin?), and eliminate the extra call or document the reasoning.
*********************************************************************************************************************/
#define MAX_FACTOR 1.5

#ifndef touchablePin_h
#define touchablePin_h

#include <stdio.h>
#include <arduino.h>

class touchablePin {
public:
    touchablePin(void);  // be sure to call setPin() before calling isTouched()
    touchablePin(uint8_t);
    touchablePin(uint8_t, float);
    touchablePin(uint8_t, float, int);

    void initUntouched(void);
    bool isTouched(void);
    void setMaxTime(int);
    int  touchRead(void);
    void setPin(uint8_t);
    
    int  pinNumber       = -1;
    int  untouchedValue  = -1;
    int  untouchedTime   = -1;
    
private:
    void init(void);
    int touchReadWithMax(uint8_t, bool);
    
    int _numSamples = 4; // give the touch routine this many time to sense touched.
    int targetTime;
    float _maxFactor = MAX_FACTOR;
};

#endif /* touchablePin_h */
