#ifndef MainState_h
#define MainState_h
#include <elapsedMillis.h>
#include <SubState.h>
#include <SharedState.h>
#include <ButtonHandler.h>
#include <EncoderHandler.h>
#include <Adafruit_Neopixel.h>

enum GlobalMode {
    SETUP_MODULE,   // when setting up a specific module
    G_GEAR,         // default mode         
    G_PROGRESS,     // show progress of all items 
    CONFIRM,        // show warning confirm statement
    TEST            // colour picker
};

class MainState: public ButtonHandler, public EncoderHandler {
    public:
        MainState(uint8_t pinA, uint8_t pinB, uint8_t ledAmount, uint8_t ringPin) : 
            EncoderHandler(pinA,pinB), _ring(ledAmount, ringPin), LED_AMNT(ledAmount) {
            Reset();  
        };

        // input
        void Reset(){
            _gmode = G_GEAR;
            SaveMode();
            Today = 0;
            resetModules();
            ResetEncoder();
            _ring.begin();
            _ring.clear();
            n_active_modules=0;
            n_setup_and_active_modules=0;

        }
        uint8_t Today;
        uint8_t LED_AMNT;
        uint8_t n_active_modules;
        uint8_t n_setup_and_active_modules;
        
        void NextDay(){
            Today += 1;
            printDay();
        }
        void NextWeek(){
            Today += 7;
            printDay();
        }
        void NextMonth(){
            Today+=31;
            printDay();
        }

        GlobalMode SavedMode(){ return _prev_gmode; }
        void SaveMode(){ _prev_gmode = _gmode; }

        uint8_t Momentum(){ return constrain(int(_momentum) + EncoderSpeed() * 100 , 0,255); }
        // global momentum is average of all sub modules
        void CalculateMomentum(){
            SubState *sub;
            float res = 0.;
            float div = 1.0 / n_active_modules;
            for(int i=0; i < MODULE_COUNT; i++){
                sub = &modules[i];
                if(sub->IsConnected() && sub->IsSetup()){
                    res += div * sub->LocalMomentum();

                }
            }
            _momentum = uint8_t(res);
        }
        
        char* spellGMode(){
            switch (_gmode)
            {
                case SETUP_MODULE:
                    return "SETUP_MODULE";
                case G_GEAR:
                    return "GEAR";            
                case G_PROGRESS:
                    return "PROGRESS";
                case CONFIRM:
                    return "CONFIRM";
                case TEST:
                    return "TEST";
            }
        }
        void Update() {
            SubState *sub;
            switch(_gmode){
                case SETUP_MODULE: {
                    if(BState() == B_UP){
                        active_module->onCompleteSetup();
                        SwitchToGear();
                        active_module = nullptr;
                    }
                    // handle turning
                    if(HasEncoderChanged()){
                        active_module->SetupEncoderChange(WheelPosition());
                    }
                    break;
                }
                case G_GEAR: {
                    if(BState() == B_UP){
                        _gmode = G_PROGRESS;
                        for(int i=0; i < MODULE_COUNT; i++){
                            sub = &modules[i];
                            if(sub->IsConnected() && sub->IsSetup()){
                                sub->SetMode(PROGRESS);
                            }
                        }
                        break;
                    } 
                    // handle turning
                    if(HasEncoderChanged()){
                        GearChangeEncoder();
                    }
                    // handle subs
                    allComplete = n_active_modules > 0;
                    for(int i=0; i < MODULE_COUNT; i++){
                        sub = &modules[i];
                        if(sub->IsConnected()){
                            allComplete &= sub->IsComplete();
                            switch (sub->Mode())
                            {
                            // handle connected modules
                            case CONNECTED: {
                                if(sub->BState() == B_UP){
                                    SwitchToSetup(sub);
                                    n_setup_and_active_modules++;
                                }
                                break;
                            }
                            case RECONNECT: {
                                if(sub->handleReconnectTimer()){
                                    n_setup_and_active_modules++;
                                }
                                if(sub->BState() == B_DOWN){
                                    sub->SaveMode();
                                    sub->SetMode(CLEAR_ON_SETUP_WARNING); 
                                }
                                break;
                            }
                            case GEAR: {
                                if(sub->BState() == B_UP){
                                    if(HoldTime() <= SHORT_PRESS_TIME) {
                                        // clear sub
                                        sub->AddCompletion();

                                    }
                                } else {
                                    sub->checkClearProgress();
                                }
                                break;
                            }
                            case CLEAR_PROGRESS_WARNING: {
                                sub->checkCancelClearProgress();
                                break;
                            }
                            case CLEAR_ON_SETUP_WARNING: {
                                if(sub->checkCancelClearSetUpProgress()){
                                    n_setup_and_active_modules--;
                                }
                                break;
                            }
                            case COMPLETION: {
                                sub->handleCompletionTimer();
                                break;
                            }
                            default:
                                break;
                            }
                        }
                    }
                    break;
                    
                }
                case G_PROGRESS: {
                    if(BState() == B_UP && HoldTime() < SHORT_PRESS_TIME){
                        SwitchToGear();
                    } else if ((BState() == B_HIGH || BState() == B_DOWN) && HoldTime() > SHORT_PRESS_TIME) {     // also include down because there might be a (severely) delayed update
                        SaveMode();
                        _gmode = CONFIRM;
                        for(int i=0; i < MODULE_COUNT; i++){
                            sub = &modules[i];
                            if(sub->IsConnected() && sub->IsSetup()){
                                sub->SaveMode();
                                sub->SetMode(CLEAR_WARNING);
                            }
                        }
                    }
                    for(int i=0; i < MODULE_COUNT; i++) {
                        sub = &modules[i];
                        if(sub->IsConnected() && sub->IsSetup()){
                            switch(sub->Mode()){
                                case PROGRESS: {
                                    if(sub->BState() == B_UP && sub->HoldTime() < SHORT_PRESS_TIME){
                                        sub->AddCompletion();
                                    } else {
                                        sub->checkClearProgress();
                                    }
                                    break;
                                }
                                case CLEAR_PROGRESS_WARNING: {
                                    sub->checkCancelClearProgress();
                                    break;
                                }
                                case COMPLETION: {
                                    sub->handleCompletionTimer();
                                    break;
                                }
                            }
                        }
                    }
                    break;
                } 
                case CONFIRM: {
                        if(BState() == B_UP){
                            ConfirmButtonUp();
                        }
                    break;
                }
                case TEST: {
                    if(BState() == B_DOWN){
                        param_pointer = ++param_pointer %3;
                        TestParameterChange();
                    }
                    if(HasEncoderChanged()){
                        UpdateTestOnEncoderChange();
                    }
                    break;
                }
            
            }
            // house keeping
            ClearButtonEvent();
            for(int i=0; i < MODULE_COUNT; i++){
                sub = &modules[i];
                if(sub->IsConnected()){
                    if(sub->IsSetup()){
                        sub->calculateLocalMomentum();
                        sub->SetMomentum(Momentum());
                    }
                    sub->SetDay(Today);
                    sub->ClearButtonEvent();
                }
            }
            CalculateMomentum();
        }
        void Render(int sinceLastFrame) {
            // general pre render
            _ring.clear();

            // main render step
            switch (_gmode)
            {
                case SETUP_MODULE: {
                    renderSetup();
                    break;
                }
                case G_GEAR: {
                    renderGear(sinceLastFrame);
                    break;
                }
                case G_PROGRESS: {
                    renderProgress();
                    break;
                }
                case CONFIRM: {
                    renderConfirm();
                    break;
                }
                case TEST: {
                    _ring.fill(_ring.ColorHSV(uint16_t(hue) * 256, sat, bri));
                    break;
                }                /* code */
                default:
                    break;
            }

            // general post render
            for (uint8_t i =0; i < LED_AMNT; i++){
                _ring.setPixelColor(i, Adafruit_NeoPixel::gamma32(_ring.getPixelColor(i)));
            }

            _ring.show();
        }
        void handleDeviceConnect(SubState *sub){
            Serial.print("I2C device found at address: ");
            Serial.println(sub->I2C_ADR);
            sub->handleConnect();
            n_active_modules++;
        }
        void handleDeviceDisconnect(SubState *sub){
            Serial.print("I2C device disconnected: ");
            Serial.println(sub->I2C_ADR);  
            sub->handleDisconnect(); 
            SwitchToGear();
            n_active_modules--;
            n_setup_and_active_modules--;
        }
        void resetModules(){
            for(uint8_t i = 0; i < MODULE_COUNT; i++){
                modules[i].Reset(Today);
            }
            n_setup_and_active_modules =0;
        } 
        SubState modules[MODULE_COUNT] = { 
            SubState(9,BIG_MODULE_LED_COUNT), SubState(10,BIG_MODULE_LED_COUNT), SubState(11,BIG_MODULE_LED_COUNT), 
            SubState(12,SMALL_MODULE_LED_COUNT), SubState(13,SMALL_MODULE_LED_COUNT), SubState(14,SMALL_MODULE_LED_COUNT), SubState(15,SMALL_MODULE_LED_COUNT), SubState(16,SMALL_MODULE_LED_COUNT)
        }; 

    private:
        Adafruit_NeoPixel _ring;

        GlobalMode _gmode;
        GlobalMode _prev_gmode;
        
        elapsedMillis time;
        // input params
        SubState *active_module = nullptr;

        // render params
        // MODE TEST
        uint8_t param_pointer;

        uint8_t hue;
        uint8_t sat;
        uint8_t bri;
        // MODE GEAR
        uint8_t displacement;
        uint8_t _momentum;

        uint8_t MINIMUM_MOMENTUM = 30;
        float gear_t =0;
        bool allComplete = false;
        // state change
        void setTestDefault() {
            hue = 0;
            sat = 200;
            bri = 100;
        }
        
        // RENDER functions
        void renderSetup() {
            uint8_t g = active_module->Goal();  // goal amount
            uint8_t w = LED_AMNT / g;           // width
            uint8_t r = LED_AMNT % g;           // remainder
            uint8_t s = progSpaceBetween(w,r, g);                  // space, default is one, plus one if the width is more then 4, plus one if the remainder is more then half of width

            for(uint8_t i = 0; i< g; i++){
                _ring.fill(ColourForFrequency(active_module->Freq()), progStartPoint(i,w,r,g),g>1?w-s:w);
            }
        }
        uint8_t scaleBalanced(uint8_t val, float scalar){
            uint8_t dv = 255 - val;
            return (scalar > 0)? val+ scalar * dv: val - val*scalar;
        }

        uint32_t ColourForFrequency(Frequency f, float sat = 0., float bri = 0.) {
            uint8_t h = HueForFreq(f);
            uint8_t s = SatForFreq(f);
            uint8_t b = DEFAULT_BRIGHTNESS;
            // s = scaleBalanced(s, sat);
            // b = scaleBalanced(b, bri);
            return _ring.ColorHSV(h<<8,s,b); // hue + sat + bri
        }

        void renderGear(int sinceLastFrame) {
            gear_t += sinceLastFrame * (1 + (Momentum()*4. / 255.)) / GEAR_TIME_DIV;
            uint8_t hue = allComplete ? COMPLETION_HUE: HueForFreq(Day);
            uint8_t sat = allComplete? 200:35 + map(Momentum(), 0, 185);
            if(Momentum() < MINIMUM_MOMENTUM && n_setup_and_active_modules > 0){
                hue = WARNING_HUE;
                sat = (MINIMUM_MOMENTUM - Momentum()) * 255 / MINIMUM_MOMENTUM;
            }
            for (int i = 0; i < LED_AMNT / 4; i++) {
                // loop over display
                for (int j = 0; j < 3; j++) {
                int head = (i * 4 + j + displacement + int(gear_t)) % LED_AMNT;
                float alpha = fmod(gear_t, 1.0);
                float brightness = 1;
                if (j != 1) {
                    // 0 for first, 1 for second
                    int ind = (j / 2) % 2;
                    brightness = (1 - ind) + (-1 + 2 * ind) * alpha;
                    brightness = pow(brightness, 2.5);
                }
                
                _ring.setPixelColor(head, _ring.ColorHSV(hue<<8, sat, uint8_t(brightness * DEFAULT_BRIGHTNESS )));
                }
            }
        }

        void renderProgress() {
            uint8_t rnd, blink_div, blink, hue, momentum, baseBri;
            int t = time / map(_momentum, 45, 200);
            baseBri = map(_momentum, 50, 110);
            for(uint8_t i = 0; i < LED_AMNT; i++){
                rnd = map(
                (87 + i * t / 11.) - 
                (11 - t / 37.) + 
                (uint8_t(17 + t / 80.) << (i + 11)) - 
                (uint8_t(i + i*t / 52.) << (i - 13)) + 
                (uint8_t(23 + t / 80.) << (17 - i)) - 
                (uint8_t(13 - i*t / 67.) << (i - 7))
                , 0,map(_momentum,8,1));
                momentum = constrain(int(_momentum) + rnd, 0, 255);
                blink_div = map(momentum, BLINK_FAST_TIME_DIV * 2, BLINK_TIME_DIV * 2);
                blink = _ring.sine8(time/ blink_div);
                hue = map(momentum, 0, 80);
                _ring.setPixelColor(i, _ring.ColorHSV(
                    hue << 8,
                    map(momentum, 250, 120),
                    constrain(map(
                        blink, 
                        baseBri - rnd *5 - map(momentum, 5,35), 
                        baseBri- rnd *5 + map(momentum, 5,35)
                        )
                    ,0,255)));
            }
        }
        void renderConfirm(){
            int ht = (active_module == nullptr)? HoldTime(): active_module->HoldTime();
            float prog =  sqrt(float(HoldTime() - SHORT_PRESS_TIME) / (LONG_PRESS_TIME- SHORT_PRESS_TIME));
            _ring.fill(_ring.ColorHSV(WARNING_HUE << 8,255,constrain(prog*2,0,1) * 255), 0, prog * LED_AMNT);
        }
        // MODE TEST
        void UpdateTestOnEncoderChange(){
            uint8_t val = WheelPositionU();
            switch(param_pointer){
                case 0:
                    hue = val;
                    break;
                case 1:
                    sat = val;
                    break;
                case 2:
                    bri = val;
            }
            Serial.print("Current: ");
            Serial.print(param_pointer);
            Serial.print(" Hue: ");
            Serial.print(hue);
            Serial.print(" Sat: ");
            Serial.print(sat);
            Serial.print(" Bri: ");
            Serial.println(bri);
        }
        void TestParameterChange() {
            switch(param_pointer){
                case 0:
                    SetEncoderValue(hue);
                    break;
                case 1:
                    SetEncoderValue(sat);
                    break;
                case 2:
                    SetEncoderValue(bri);
                    break;
            }
        }
        void SwitchToTest(){
            _gmode = TEST;
            TestParameterChange();
        }
        
        // MODE CONFIRM
        void ConfirmButtonUp(){
            if(HoldTime()> LONG_PRESS_TIME){
                switch(SavedMode()){
                    case G_PROGRESS: {
                        // reset all
                        resetModules();
                        Reset();
                        break;
                    } 
                }
            } else {
                _gmode = SavedMode();
                SubState *sub;
                for(int i=0; i < MODULE_COUNT; i++){
                    sub = &modules[i];
                    if(sub->IsConnected() && sub->IsSetup()){
                        sub->SetMode(sub->SavedMode());
                    }
                }

            }
        }

        // MODE PROGRESS
        void ProgressClearAll(){
            SubState *sub;
            SaveMode();
            _gmode = CONFIRM;
            for(int i =0; i <MODULE_COUNT; i++){
                sub = &modules[i];
                if(sub->IsConnected() && sub->IsSetup()){
                    sub->SaveMode();
                    sub->SetMode(CLEAR_WARNING);
                }
            }
        }

        // MODE GEAR
        void SwitchToGear(){
            _gmode = G_GEAR;
            gear_t = 0;
            displacement = 0;
            SubState *sub;
            for(int i=0; i < MODULE_COUNT; i++){
                sub = &modules[i];
                if(sub->IsConnected() && sub->IsSetup()){
                    sub->switchToGear();
                }
            }
            SetEncoderValue(displacement);
        }
        void GearChangeEncoder(){
            SubState *sub;
            displacement = (LED_AMNT+ WheelPosition()%LED_AMNT)%LED_AMNT;
            for(int i=0; i < MODULE_COUNT; i++){
                sub = &modules[i];
                if(sub->IsConnected() && sub->Mode() == GEAR){
                    sub->SetDisplacement(WheelPosition());
                }
            }
        }

        // MODE SETUP
        void SwitchToSetup(SubState *active){
            active_module = active;
            SetEncoderValue(active->GetSetupEncoderValue());
            _gmode = SETUP_MODULE;
            active->SetMode(SETUP);
        }

        void printDay(){
            Serial.println("=======================");
            Serial.print("Today is day: ");
            Serial.println(Today);
            Serial.println("=======================");
        }
};

#endif