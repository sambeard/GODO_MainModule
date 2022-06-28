#ifndef ButtonHandler_h
#define ButtonHandler_h
#include <elapsedMillis.h>

enum ButtonState {
    B_LOW  = 0,      // constant low
    B_HIGH = 1,     // constant HIGH
    B_UP,       // event up
    B_DOWN      // event down
};

class ButtonHandler {
    public: 
        ButtonState BState(){ return _bstate;}
        void handleButton(bool isPressed){
            // when not pressed and not low and not already B_UP
            if(!isPressed && _bstate != B_LOW && _bstate != B_UP){
                _bstate = B_UP;
                _holdDuration = _holdTimer;
            }
            // when not pressed and not low and not already B_UP
            else if(isPressed && _bstate != B_HIGH && _bstate != B_DOWN){
                _holdTimer = 0;
                _bstate = B_DOWN;
            }
        }
        void ClearButtonEvent() {
            switch (_bstate)
            {
                case B_DOWN:{
                    _bstate = B_HIGH;
                    break;
                }
                case B_UP:{
                    _bstate = B_LOW;
                    break;
                }
                default:
                    break;
            }
        }
        uint32_t HoldTime(){ return (_bstate == B_HIGH || _bstate == B_DOWN)? uint32_t(_holdTimer):_holdDuration; }
    private:
        volatile ButtonState _bstate = B_LOW;
        elapsedMillis _holdTimer = 0;
        uint32_t _holdDuration  = 0;
};

#endif