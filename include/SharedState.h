#ifndef SharedState_h
#define SharedState_h

static uint8_t const BIG_MODULE_LED_COUNT = 16;
static uint8_t const SMALL_MODULE_LED_COUNT = 12;
static uint8_t const MODULE_COUNT = 8;
static uint8_t const BIG_MODULE_COUNT = 3;
static uint8_t const PACKET_SIZE = 5;

static float const GEAR_TIME_DIV = 5000.;
static uint8_t const BLINK_TIME_DIV = 9;
static uint8_t const BLINK_FAST_TIME_DIV = 3;

static uint8_t const MAX_DAYS = 31;
static uint8_t const MAX_COMPLETIONS = 6;

static uint8_t const DEFAULT_BRIGHTNESS = 130;

const static int SETUP_WAIT_TIME = 5000;
static int const LONG_PRESS_TIME = 3500;
static int const SHORT_PRESS_TIME = 500;

static uint32_t const COMPLETION_HUE = 80;
static uint32_t const WARNING_HUE = 255;

enum LMODE {
    GEAR,
    CONNECTED,
    SETUP,
    RECONNECT,
    CLEAR_WARNING,
    CLEAR_ON_SETUP_WARNING,
    CLEAR_PROGRESS_WARNING,
    //LOADING,
    PROGRESS,
    COMPLETION,
};

enum Frequency {
    Day,
    Week,
    Month
};

uint8_t HueForFreq(Frequency f){
    switch (f)
    {
    case Day:
        return 150;
    case Week:
        return 250;
    case Month:
        return 200;
    }
}
uint8_t SatForFreq(Frequency f){
    switch (f)
    {
    case Day:
        return 135;
    case Week:
        return 135;
    case Month:
        return 220;
    }
}

uint8_t progSpaceBetween(uint8_t w, uint8_t r, uint8_t d){
    return constrain(1 +  w/4 + (1-(r + d/2) / d),1,w-1);
}
uint8_t progStartPoint(uint8_t i, uint8_t w, uint8_t r, uint8_t d){
    return i*w + (r*i)/d;
}
uint8_t map(uint8_t in, uint8_t min, uint8_t max){
  int d = max-min;
  return uint8_t(min + (int(in)*d)/255);
}

#endif