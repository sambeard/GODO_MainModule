#include <Wire.h>
#include <arduino.h>
#include <MainState.h>

#define BUTTON_PIN 17
#define ENCODER_A_PIN 3
#define ENCODER_B_PIN 4
#define MAIN_RING_PIN 16

#define DEBUG true

// frequency constants for time based operations
#define POLL_CONNECTION_FREQ 10
#define POLL_INPUT_FREQ 120
#define UPDATE_FREQ 60
#define PUSH_DEVICE_FREQ 20
#define RENDER_FREQ 60

// amount of leds in ring
#define MAIN_RING_LED_AMOUNT 24

void handleButtonUp();
void handleButtonDown();

// state object
MainState state(ENCODER_A_PIN, ENCODER_B_PIN, MAIN_RING_LED_AMOUNT, MAIN_RING_PIN);

// timers
elapsedMillis pollDeviceTimer;
elapsedMillis pollButtonTimer;
elapsedMillis pushDeviceTimer;
elapsedMillis updateTimer;
elapsedMillis renderTimer;

void setup() {
  Serial.begin(9600);  // start serial for output
  pinMode(BUTTON_PIN,INPUT_PULLUP);
  Wire.begin();        // join i2c bus (address optional for master)
}

// main loop
void loop() {
  // handle input
  while(Serial.available()){
    switch(Serial.read()){
      case 'd': {
        state.NextDay();
        break;
      }
      case 'w': {
        state.NextWeek();
        break;
      }
      case 'm': {
        state.NextMonth();
        break;
      }
    }
  }
  // polling for newly (dis)connected devices
  if(pollDeviceTimer> 1000 / POLL_CONNECTION_FREQ){
    pollConnectionStates();
    pollDeviceTimer =0; 
  }
  // polling for button state and rotary encoder changes
  if(pollButtonTimer> 1000 / POLL_INPUT_FREQ){
    pollButtonStates();
    state.handleButton(!digitalRead(BUTTON_PIN));
    state.ServiceEncoder();
    pollButtonTimer =0; 
  }
  // update the logic
  if(updateTimer> 1000 / UPDATE_FREQ){
    state.Update();
    updateTimer =0;
  }
  // push changes to sub nodes
  if(pushDeviceTimer> 1000 / PUSH_DEVICE_FREQ){
    // action
    updateDevices();
    pushDeviceTimer =0; //reset
  }

  // render 
  if(renderTimer> 1000 / RENDER_FREQ){
    state.Render(renderTimer);
    renderTimer =0;
  }

}

void updateDevices(){
  SubState *sub;
  uint8_t error;
  for(uint8_t i =0; i < MODULE_COUNT; i++) {
    sub = &state.modules[i];
    if(sub->IsConnected() && sub->HasUpdate()) { 
      if(DEBUG){
        Serial.print("Updating device..");
        Serial.println(sub->I2C_ADR);
      }
      uint8_t *data = sub->Updates(); 
      Wire.beginTransmission(sub->I2C_ADR);
      if (DEBUG){

      for(int i = 0; i < PACKET_SIZE; i++){
          if(i< 2){
            Serial.print(data[i] >> 4, BIN);
            Serial.print(" ");
            Serial.print(data[i] & 0xF, BIN);
          } else {
            Serial.print(data[i], DEC);

          }
          Serial.print("\t\\\t");
        }
        Serial.println();
      }
      Wire.write(data, PACKET_SIZE);
      error = Wire.endTransmission();
      switch(error){
        case 0:
          sub->ClearUpdateFlag();
          break;
        default:
        if(DEBUG){
          Serial.print("Error ");
          Serial.print(error);
          Serial.print(" in communicating with device:");
          Serial.println(sub->I2C_ADR);
        }
      }
      // cleanup memory
      delete data;
    }
  }
}

void pollButtonStates(){
  SubState *sub;
  for(uint8_t i =0; i < MODULE_COUNT; i++) {
    sub = &state.modules[i];
    if(sub->IsConnected()){
      Wire.requestFrom(sub->I2C_ADR, 1);    // request 1 bytes from slave device
      while(Wire.available()){
        byte c = Wire.read();     // receive a byte as character
        bool bState = c&1;
        sub->handleButton(bState);
      }  
    }
  }
}

void pollConnectionStates(){
  byte error;
  SubState *sub;
  for(uint8_t i =0; i < MODULE_COUNT; i++) {
    sub = &state.modules[i];
    Wire.beginTransmission(sub->I2C_ADR);
    error = Wire.endTransmission();
    // connect new device
    if (error == 0 && !sub->IsConnected())
    {
      state.handleDeviceConnect(sub);
    }
    // disconnect device
    else if (error > 0 && sub->IsConnected())
    {
      state.handleDeviceDisconnect(sub);
    }

  }

}