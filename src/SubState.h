#ifndef SubState_h
#define SubState_h
#include <elapsedMillis.h>
#include <SharedState.h>
#include <ButtonHandler.h>


class SubState :  public ButtonHandler {
    public:
        SubState(uint8_t adr, uint8_t count): I2C_ADR(adr), LED_COUNT(count) {
            Reset(0);
        };
        const uint8_t I2C_ADR;
        const uint8_t LED_COUNT;     

        bool IsConnected(){ return _isConnected; }
        bool IsSetup(){ return _isSetup; }

        uint8_t DayIndex() { return DayIndex(_today); }
        uint8_t DayIndex(uint8_t day) { return (day-_dayOfReset) / _frequencyDivider(); }
        void SetDay(uint8_t t) { if(_today != t){ _today = t; _update_flag |= 1<< 1;}}
        
        LMODE SavedMode(){ return _prevMode; }
        void SaveMode(){ _prevMode = _localMode; }

        LMODE Mode() { return _localMode; }
        void SetMode(LMODE m) { if(m!= _localMode){_localMode = m; _update_flag |= 1<< 0;} }

        Frequency Freq() { return _freq; }
        void SetFreqAndGoal(Frequency f, uint8_t g) { 
            if(f != Freq() ||  g != Goal()){
                _freq = f;
                _goal = g;
                _update_flag |= 1<< 0; // update freq
                _update_flag |= 1<< 1; // update goal
                ClearAllCompletions();
                _dayOfReset = _today;
            }
        }
        
        uint8_t Goal() { return _goal; }

        uint8_t Completions(uint8_t day) { return _completions[DayIndex(day)]; }
        uint8_t Completions() { return Completions(_today); }
        void ClearCompletions() { _completions[DayIndex()] =0; _update_flag |= 1<< 1; }
        void ClearAllCompletions() { memset(_completions, 0, MAX_DAYS); _update_flag |= 1<< 1; }
        void AddCompletion() { 
            if(Completions() < Goal()){
                _completions[DayIndex()] +=1;
                _update_flag |= 1<< 1; 
            }
        }
        bool IsComplete(){ return Completions() == Goal(); }
        uint8_t Momentum() { return _momentum; }
        void SetMomentum(uint8_t m) { if(m!= Momentum()){_momentum= m; _update_flag |= 1<< 2;}}

        uint8_t LocalMomentum() { return _localMomentum; }

        uint8_t Displacement() { return _displacement; }
        void SetDisplacement(int d) { 
            if(d!= Displacement()){
                _displacement = (LED_COUNT+d%LED_COUNT)%LED_COUNT; _update_flag |= 1<< 4;
            }
            
        }

        bool HasUpdate() { return _update_flag > 0;}

        uint8_t* Updates() {
            uint8_t mask = 0xF;
            return new uint8_t[PACKET_SIZE]{
                (uint8_t(_localMode) *16) + uint8_t(Freq()),
                ((Goal() & mask) * 16) + (Completions() & mask), 
                _localMomentum,
                _momentum,
                _displacement
            };
        }

        void SetUpdateAll(){ _update_flag = 0b00001111; }
        void ClearUpdateFlag() { _update_flag = 0;}


        void calculateLocalMomentum(){
            float alpha, score;
            float res = 0.75;   // start with positive score
            bool firstIt = true;
            uint8_t fd = _frequencyDivider();
            float comp = (fd ==1)? 1 : float(( 1 + _today - _dayOfReset) % fd) / float(fd);
            for(int j = _today; j >= _dayOfReset; j-= fd){
                alpha = float(1 + _today - j) / (2 + _today - j);
                score = ((float(Completions(j)) / Goal()) / (firstIt?comp:1.));
                res = res * alpha  +  (1-alpha ) * constrain(score,0.,1.);
                firstIt = false;
            }
            _localMomentum = uint8_t(res * 255.0);
        }

        void Reset(uint8_t day){
            _localMode = CONNECTED;
            _prevMode = CONNECTED;
            _freq = Day;
            _goal = 1;
            ClearAllCompletions();
            _momentum = 0;
            _isSetup = false;
            _today = day;
            _dayOfReset = day;
            _displacement = 0;
            SetUpdateAll();
        }
        void resetReconnectTimer(){
            setupTimer = 0;
        }
        bool handleReconnectTimer(){
            if(setupTimer >  SETUP_WAIT_TIME){
               switchToGear();
               return true;
            }
            return false;
        }
        void switchToGear(){
            SetMode(GEAR);
            SetDisplacement(0);
        }
        void SetupEncoderChange(int value){
            uint8_t mod = MAX_COMPLETIONS * 3;  // length of frequency bins 
            uint8_t val = (mod + value % mod) % mod;
            Frequency f = Frequency(val/MAX_COMPLETIONS);
            uint8_t g = 1 + val % MAX_COMPLETIONS ;


            SetFreqAndGoal(f,g);
        }
        uint8_t GetSetupEncoderValue(){
            return Goal() -1 + MAX_COMPLETIONS * uint8_t(Freq()); 
        }
        void handleConnect() {
            SetMode(_isSetup?RECONNECT:CONNECTED);
            if(Mode()==RECONNECT){
                // setup timer
                resetReconnectTimer();
            } else {
                // just to be sure
                Reset(_today);
            }
            _isConnected = true;
        }
        void handleDisconnect(){
            _isConnected = false;
            SetMode(GEAR);
            SaveMode();
        }
        void checkCancelClearProgress(){
            if(BState() == B_UP){
                if(HoldTime() > LONG_PRESS_TIME) {
                    // clear sub
                    ClearCompletions();
                }
                SetMode(SavedMode());
            }
        }
        bool checkCancelClearSetUpProgress(){
            if(BState() == B_UP){
                if(HoldTime() > LONG_PRESS_TIME) {
                    // clear sub
                    Reset(_today);
                    return true;
                }
                else {
                    SetMode(SavedMode());
                    resetReconnectTimer();
                }
            }
            return false;
        }
        void onCompleteSetup(){
            _isSetup = true;
        }
        void checkClearProgress(){
            if ((BState() == B_DOWN || BState() == B_HIGH) && HoldTime() > SHORT_PRESS_TIME) {           
                SaveMode();
                SetMode(CLEAR_PROGRESS_WARNING);
            }
        }
    private:
        elapsedMillis setupTimer = SETUP_WAIT_TIME;
        uint8_t _update_flag;
        bool _isConnected = false; 
        bool _isSetup = false;
        // General variables
        uint8_t _today;
        uint8_t _dayOfReset;
        LMODE _localMode;       // Bit 0
        LMODE _prevMode; 
        Frequency _freq;        // Bit 0
        uint8_t _goal;        // Bit 1
        uint8_t _completions[MAX_DAYS];   // Bit 1
        uint8_t _momentum;       // Bit 2, 0 -> Low, 255 -> High
        uint8_t _localMomentum;        

        // Gear Variables
        uint8_t _displacement;      // Bit 3

        uint8_t _frequencyDivider(){
            switch(Freq()){
                case Day:
                    return 1;
                case Week:
                    return 7;
                case Month:
                    return 31;
            }
        }

};
#endif