#ifndef EncoderHandler_h
#define EncoderHandler_h
#include <Encoder.h>
class EncoderHandler {
    public:
        EncoderHandler(uint8_t pinA, uint8_t pinB, int initial =0): 
            _encoder(pinA,pinB), _prevPosition(initial){
        }
        // divide by four due to quadrature encoding
        int WheelPosition() {return _encoder.read() / 4;}
        uint8_t WheelPositionU() {return (255+(WheelPosition())%255)%255;}
        void ServiceEncoder() {
            int n = _encoder.read();
            int d = 0;
            if(n!= _prevPosition){
                _encoderChanged = true;
                d = n-_prevPosition;
                _prevPosition = n;
            }
            _speed = _alpha * _speed + (1.-_alpha) * d; 
        }
        bool HasEncoderChanged(){
            bool ret = _encoderChanged;
            _encoderChanged = false;
            return ret;
        }
        // multiply by four due to quadrature encoding
        void SetEncoderValue(int v){ _encoder.write(v*4);}
        void ResetEncoder(){
            _prevPosition =0;
            _encoder.write(_prevPosition);
        }
        // crude approximation of Speed
        float EncoderSpeed(){ return _speed; }
    private:
        Encoder _encoder;
        float _alpha = 0.998;
        float _speed = 0.;
        int _prevPosition =0;
        bool _encoderChanged = false;
};
#endif
