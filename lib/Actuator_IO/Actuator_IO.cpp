#include "Actuator_IO.h"
#include "Arduino.h"

Actuator_IO::Actuator_IO(int extPin, int retPin, int enablePin, int output1Pin, int output2Pin, int extLED, int retLED){
    _extPin = extPin;
    _retPin = retPin;
    _enablePin = enablePin;
    _output1Pin = output1Pin;
    _output2Pin = output2Pin;
    _extLED = extLED;
    _retLED = retLED;

    testExtFB = false;
    testRetFB = false;

    pinMode(_extPin, INPUT_PULLUP);
    pinMode(_retPin, INPUT_PULLUP);
    pinMode(_enablePin, OUTPUT);
    pinMode(_output1Pin, OUTPUT);
    pinMode(_output2Pin, OUTPUT);
    pinMode(_extLED, OUTPUT);
    pinMode(_retLED, OUTPUT);

    setMinSpeed(5);
    setNormallyOpenFB(true);
    setHasFeedback(true);
    setFeedbackTime(3);
    setSpeed(100);
    setTimeout(3);

    readInput();
    if(_extFB){
        _state = extended;
        _prevState = extended;
    }
    else if(_retFB){
        _state = retracted;
        _prevState = retracted;
    }
    else{
        _state = relaxed;
        _prevState = relaxed;
    }
    
    cyclic();
}

void Actuator_IO::setSpeed(int speed){
    if(speed >=1 && speed <=100){
        _speed = map(speed, 1, 100, _minSpeed, 255);
    }
    else if(speed == 0){
        _speed = 0;
    }
}

void Actuator_IO::setMinSpeed(int minSpeed){
    if(minSpeed >= 0 && minSpeed <100){
        _minSpeed = map(minSpeed,0,100,0,255);
    }
    setSpeed(_speed);
}

int Actuator_IO::getSpeed(){
    if(_speed != 0){
        return map(_speed, _minSpeed, 255, 1, 100);
    }
    else{
        return 0;
    }
}

Actuator_IO::state Actuator_IO::getState(){
    return _state;
}

void Actuator_IO::setTimeout(unsigned int timeout){
    if(timeout >= 0){
        _timeout = timeout * 1000;
    }
}

void Actuator_IO::setNormallyOpenFB(bool isNormallyOpen){
    _isNO = isNormallyOpen;
}

void Actuator_IO::setHasFeedback(bool hasFeedback){
    _hasFB = hasFeedback;
}

void Actuator_IO::setFeedbackTime(unsigned int FBTime){
    if(FBTime >= 0){
        _FBTime = FBTime * 1000;
    }
}

void Actuator_IO::extend(){
    if(!_extFB){
        _state = extending;
    }
}

void Actuator_IO::retract(){
    if(!_retFB){
        _state = retracting;
    }
}

void Actuator_IO::stop(){
    _state = stopped;
}

void Actuator_IO::relax(){
    _state = relaxed;
}

void Actuator_IO::readInput(){
    _extFB = (digitalRead(_extPin) != _isNO) || testExtFB;
    _retFB = (digitalRead(_retPin) != _isNO) || testRetFB;
}

void Actuator_IO::writeOutput(){
    switch(_state){
        case extending:
            digitalWrite(_output1Pin,HIGH);
            digitalWrite(_output2Pin,LOW);
            analogWrite(_enablePin,_speed);
            break;
        case retracting:
            digitalWrite(_output1Pin,LOW);
            digitalWrite(_output2Pin,HIGH);
            analogWrite(_enablePin,_speed);
            break;
        case stopped:
        case extended:
        case retracted:
        case timedout:
            digitalWrite(_output1Pin,LOW);
            digitalWrite(_output2Pin,LOW);
            analogWrite(_enablePin,255);
            break;
        case relaxed:
            digitalWrite(_output1Pin,LOW);
            digitalWrite(_output2Pin,LOW);
            analogWrite(_enablePin,0);
            break;
    }
    digitalWrite(_extLED, _extFB);
    digitalWrite(_retLED, _retFB);
}

void Actuator_IO::cyclic(){
    static unsigned long startTime;

    readInput();
    switch(_state){
        case extended:
            if(_state != _prevState){
                _prevState = _state;
            }
            break;

        case extending:
            if(_state != _prevState){
                startTime = millis();
                _prevState = _state;
            }

            if(millis()-startTime >= _timeout && _timeout != 0 && _hasFB){
                _state = timedout;
            }
            
            if((_extFB && !_retFB && _hasFB) || (millis()-startTime >= _FBTime && !_hasFB)){
                _state = extended;
            }
            break;

        case retracted:
            if(_state != _prevState){
                _prevState = _state;
            }
            break;

        case retracting:
            if(_state != _prevState){
                startTime = millis();
                _prevState = _state;
            }

            if(millis()-startTime >= _timeout && _timeout != 0 && _hasFB){
                _state = timedout;
            }
            
            if((!_extFB && _retFB && _hasFB) || (millis()-startTime >= _FBTime && !_hasFB)){
                _state = retracted;
            }
            break;

        case timedout:
            if(_state != _prevState){
                _prevState = _state;
            }
            break;

        case stopped:
            if(_state != _prevState){
                _prevState = _state;
            }
            break;

        case relaxed:
            if(_state != _prevState){
                _prevState = _state;
            }
            break;
    }
    writeOutput();
}