
////////////////////////////////////
//     SOMFY RTS CONTROLLER       //
////////////////////////////////////

#include "extras/RFControl.h"
#include "RFM69.h"
#include <nvs_flash.h>

char cBuf[128];                       // general string buffer for formatting output when needed

PushButton upButton(UP_BUTTON);
PushButton myButton(MY_BUTTON);
PushButton downButton(DOWN_BUTTON);

#define RF_FREQUENCY  433.42    // RF frequency (in MHz) for Somfy-RTS system

#define SOMFY_STOP    0   
#define SOMFY_RAISE   1
#define SOMFY_LOWER   2
#define SOMFY_PROGRAM 3

const char *label[]={"STOPPING","RAISING","LOWERING","PROGRAMMING"};

RFControl rf(RFM_SIGNAL_PIN);
RFM69 rfm69(RFM_CHIP_SELECT,RFM_RESET_PIN);
nvs_handle somfyNVS;
char serialNum[32];

//////////////////////////////////////

struct DEV_Somfy : Service::WindowCovering {    

  SpanCharacteristic *current;
  SpanCharacteristic *target;
  SpanCharacteristic *indicator;
   
  double velocity=0;
  uint32_t startTime=0;
  boolean recalibrate=false;
  char *sChannel;
  uint8_t channel;
  uint32_t address;

  struct ShadeData {
    uint16_t rollingCode=0xFF;           // arbitrary starting code
    uint32_t raiseTime;                  // time to raise shade from fully closed to fully open (in milliseconds)
    uint32_t lowerTime;                  // time to lower shade from fully open to fully closed (in milliseconds)
  } shadeData;

  static vector<DEV_Somfy *> shadeList;     // store a list of all shades so we can scroll through selection with progButton
  static int selectedShade;                 // selected shade in shadeList
  
//////////////////////////////////////
  
  DEV_Somfy(uint8_t channel, char *channel_s, uint32_t raiseTime=10000, uint32_t lowerTime=10000) : Service::WindowCovering(){       // constructor() method

    this->channel=channel;                        // channel number (1-32)
    shadeData.raiseTime=raiseTime;                // time (in milliseconds) to fully open
    shadeData.lowerTime=lowerTime;                // time (in milliseconds) to fully close
    address=(SOMFY_ADDRESS & 0x7FFFF)*32+channel; // Somfy address for this channel
    sChannel=channel_s;                           // string name for this channel used to index NVS and display as model
    
    current=new Characteristic::CurrentPosition(0,true);    // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)    
    target=new Characteristic::TargetPosition(0,true);      // Windows Shades have positions that range from 0 (fully lowered) to 100 (fully raised)
    indicator=new Characteristic::ObstructionDetected(0);   // use this as a flag to indicate to user that this channel has been selected

    size_t len;
    if(!nvs_get_blob(somfyNVS,sChannel,NULL,&len)){                       // if found data for this channel in NVS
      nvs_get_blob(somfyNVS,sChannel,&shadeData,&len);                    // retrieve data
    } else {
      nvs_set_blob(somfyNVS,sChannel,&shadeData,sizeof(ShadeData));       // save data
      nvs_commit(somfyNVS);                                               // commit to NVS
    }
       
    Serial.printf("Configuring Somfy Window Shade %s:  Address=%06X  RollingCode=%04X  RaiseTime=%d ms  LowerTime=%d ms\n",sChannel,address,shadeData.rollingCode,shadeData.raiseTime,shadeData.lowerTime);

    shadeList.push_back(this);

  } // end constructor

//////////////////////////////////////

  boolean update(){                              // update() method

    int estimatedPosition;

    estimatedPosition=current->getVal<double>()+velocity*double(millis()-startTime);
    if(estimatedPosition>100)
      estimatedPosition=100;
    if(estimatedPosition<0)
      estimatedPosition=0;    
      
    if(target->getNewVal() > estimatedPosition && velocity<=0){

      transmit(SOMFY_RAISE);
           
      if(velocity<0)
        current->setVal(estimatedPosition);
        
      velocity=100.0/shadeData.raiseTime;
      startTime=millis();
      
    } else
    
    if(target->getNewVal() < estimatedPosition && velocity>=0){

      transmit(SOMFY_LOWER);

      if(velocity>0)
        current->setVal(estimatedPosition);
        
      velocity=-100.0/shadeData.lowerTime;
      startTime=millis();
    }
        
    return(true);
  
  } // update

//////////////////////////////////////

  void loop(){                                   // loop() method

    if(velocity==0)
      return;

    int estimatedPosition=current->getVal<double>()+velocity*double(millis()-startTime);

    int targetPosition=target->getVal();

    if(targetPosition==100)         // if request for fully open or fully close, overshoot target to make sure shade really is fully open or closed
      targetPosition=120;           
    if(targetPosition==0)
      targetPosition=-20;
    
    if((velocity>0 && estimatedPosition > targetPosition) || (velocity<0 && estimatedPosition < targetPosition)){

      if(targetPosition>100){
        sprintf(cBuf,"** Somfy %s: Fully Open\n",sChannel);
        LOG1(cBuf);
      } else if(targetPosition<0){
        sprintf(cBuf,"** Somfy %s: Fully Closed\n",sChannel);
        LOG1(cBuf);
      } else {
        transmit(SOMFY_STOP);
      }
      
      current->setVal(target->getVal());
      velocity=0;
      recalibrate=false;
    }
    
  } // loop

//////////////////////////////////////

  void transmit(uint8_t action){

    rfm69.setRegister(0x01,0x0c);           // enable transmission mode
    delay(10);
  
    sprintf(cBuf,"** Somfy %s: %s  RC=%04X\n",sChannel,label[action],++shadeData.rollingCode);
    LOG1(cBuf);

    uint8_t b[7];
  
    b[0]=0xA0;
    b[1]=1<<(4+action);
    b[2]=shadeData.rollingCode >> 8;
    b[3]=shadeData.rollingCode & 0xFF;
    b[4]=(address >> 16) & 0xFF;
    b[5]=(address >> 8) & 0xFF;
    b[6]=(address) & 0xFF;
  
    uint8_t checkSum=0; 
    for(int i=0;i<7;i++)
     checkSum ^= b[i] ^ (b[i] >> 4);
     
    b[1] |= checkSum & 0x0F;  
  
    char c[64];
    sprintf(c,"Transmitting: %02X %02X %02X %02X %02X %02X %02X\n",b[0],b[1],b[2],b[3],b[4],b[5],b[6]);
    LOG1(c);
  
    for(int i=1;i<7;i++)
      b[i] ^= b[i-1];
  
    sprintf(c,"Obfuscated:   %02X %02X %02X %02X %02X %02X %02X\n",b[0],b[1],b[2],b[3],b[4],b[5],b[6]);
    LOG1(c);
  
    rf.clear();
    
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(4550,604);
    
    for(int i=0;i<7;i++){
      for(int j=128;j>0;j=j>>1){
        rf.phase(604,(b[i]&j)?0:1);
        rf.phase(604,(b[i]&j)?1:0);
      }
    }
  
    rf.phase(30415,0);
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(2416,2416);
    rf.add(2416,2416);
         
    rf.start(3,1);                    // start transmission!  Repeat 3 times; Tick size=1 microseconds

    rfm69.setRegister(0x01,0x04);                 // re-enter stand-by mode

    nvs_set_blob(somfyNVS,sChannel,&shadeData,sizeof(ShadeData));       // save data
    nvs_commit(somfyNVS);                                               // commit to NVS
       
  } // transmit

//////////////////////////////////////

  static void poll(){

    DEV_Somfy *ss=shadeList[selectedShade];

    if(upButton.triggered(5,2000,200)){
      
      if(upButton.type()==PushButton::LONG){
        if(downButton.primed()){
          ss->indicator->setVal(0);
          ss->transmit(SOMFY_PROGRAM);
          downButton.wait();
          downButton.reset();        
          upButton.wait();        
          upButton.reset();
        } else {
          ss->target->setVal(100);
          ss->indicator->setVal(0);
          ss->shadeData.raiseTime=120000;
          ss->update();
          LOG1("** Preparing to Reset Raise Time\n");
          ss->recalibrate=true;
          upButton.wait();
          upButton.reset();
        }
        
      } else

      if(upButton.type()==PushButton::SINGLE && ss->target->getVal()<100){
        ss->target->setVal(100);
        ss->indicator->setVal(0);
        ss->update();
      }

    } else

    if(downButton.triggered(5,2000,200)){
      
      if(downButton.type()==PushButton::LONG){
        if(upButton.primed()){
          ss->indicator->setVal(0);
          ss->transmit(SOMFY_PROGRAM);
          downButton.wait();
          downButton.reset();        
          upButton.wait();        
          upButton.reset();
        } else {
          ss->target->setVal(0);
          ss->indicator->setVal(0);
          ss->shadeData.lowerTime=120000;
          ss->update();
          LOG1("** Preparing to Reset Lower Time\n");
          ss->recalibrate=true;
          downButton.wait();
          downButton.reset();
        }
       
      } else

      if(downButton.type()==PushButton::SINGLE && ss->target->getVal()>0){
        ss->target->setVal(0);
        ss->indicator->setVal(0);
        ss->update();
      }
      
    } else

    if(myButton.triggered(5,1000,200)){
      if(myButton.type()==PushButton::DOUBLE){
        if(ss->indicator->getVal()){
          ss->indicator->setVal(0);
          selectedShade=(selectedShade+1)%shadeList.size();
          ss=shadeList[selectedShade];
        }
        ss->indicator->setVal(1);
        sprintf(cBuf,"** Somfy %s: Selected\n",ss->sChannel);
        LOG1(cBuf);
        
      } else
      
      if(myButton.type()==PushButton::SINGLE && ss->velocity!=0){
        ss->indicator->setVal(0);

        if(ss->recalibrate){
          if(ss->velocity>0){         
            ss->velocity=1;
            ss->shadeData.raiseTime=millis()-ss->startTime;
            LOG1("** Reset Raise Time to ");
            LOG1(ss->shadeData.raiseTime);
            LOG1("\n");
          } else {
            ss->velocity=-1;
            ss->shadeData.lowerTime=millis()-ss->startTime;            
            LOG1("** Reset Lower Time to ");
            LOG1(ss->shadeData.lowerTime);
            LOG1("\n");
          }
          ss->recalibrate=false;
          nvs_set_blob(somfyNVS,ss->sChannel,&(ss->shadeData),sizeof(ShadeData));       // save data
          nvs_commit(somfyNVS);                                                         // commit to NVS
          
        } else {
        
          int estimatedPosition=ss->current->getVal<double>()+ss->velocity*double(millis()-ss->startTime);
          if(estimatedPosition>100)
            estimatedPosition=100;
          else if(estimatedPosition<0)
            estimatedPosition=0;
          ss->target->setVal(estimatedPosition);
        }        
      }
    }

  } // poll
  
////////////////////////////////////
    
}; // DEV_Somfy()

////////////////////////////////////

struct SomfyShade{
  char channel_s[6];

  SomfyShade(uint8_t channel, char *name, uint32_t raiseTime=10000, uint32_t lowerTime=10000){

    if(!somfyNVS){
      nvs_open("SOMFY_DATA",NVS_READWRITE,&somfyNVS);
      rfm69.init();
      rfm69.setFrequency(RF_FREQUENCY);
      sprintf(serialNum,"SMF-%05X",SOMFY_ADDRESS & 0x7FFFF);
      Serial.printf("Somfy+ Serial Number: %s\n",serialNum);

      new SpanAccessory(1);  
      new DEV_Identify("SomfyPlus","HomeSpan",serialNum,"32-Channel RTS",SKETCH_VERSION,3);
      new Service::HAPProtocolInformation();
      new Characteristic::Version("1.1.0");
    }
    
    if(channel<1 || channel>32){
      Serial.printf("\n*** WARNING.  Channel number %d is out of range [1-32].  Cannot create '%s'!\n\n",channel,name);
      return;
    }

    if(!DEV_Somfy::shadeList.empty()){
      for(int i=0;i<DEV_Somfy::shadeList.size();i++){
        if(channel==DEV_Somfy::shadeList[i]->channel){
          Serial.printf("\n*** WARNING.  Channel number %d already used.  Cannot create '%s'!\n\n",channel,name);
          return;
        }
      }
    }

    sprintf(channel_s,"CH-%02d",channel);  
    new SpanAccessory(channel+1);
    new DEV_Identify(name,"HomeSpan",channel_s,"Somfy+",SKETCH_VERSION,0);
    new DEV_Somfy(channel,channel_s,raiseTime,lowerTime);
  }

  static void poll(){
    DEV_Somfy::poll();
  }

  static void deleteData(const char *s){
    nvs_erase_all(somfyNVS);
    nvs_commit(somfyNVS);      
    Serial.print("\n*** All Somfy Data ERASED!  Re-starting...\n\n");
    delay(1000);
    ESP.restart();                                                                             // re-start device     
  }

};

//////////////////////////////////////

vector<DEV_Somfy *> DEV_Somfy::shadeList;
int DEV_Somfy::selectedShade=0;
