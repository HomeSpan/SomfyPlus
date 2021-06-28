
// Basic Controls for RFM69 Transceiver using Continuous Mode via DIO2

#include <SPI.h>

struct RFM69 {

  uint8_t csPin;
  uint8_t resetPin;

//////////////////////////////////////

  RFM69(uint8_t csPin, uint8_t resetPin) {
    
    this->csPin=csPin;
    this->resetPin=resetPin;    
    
    pinMode(csPin,OUTPUT);
    digitalWrite(csPin,HIGH);
    pinMode(resetPin,OUTPUT);
  }
    
//////////////////////////////////////

  void init(){
    
    reset();        
    SPI.begin();
    delay(100);
    
    setRegister(0x02,0x68);   // set continuous mode via DIO2 without bit synchronization, OOK modulation, no shaping
    setRegister(0x11,0x5F);   // disable PA0 and enable PA1 instead - this is required (though undocumented!) to use DIO2 as continuous mode input

    if(getRegister(0x02)==0x68 && getRegister(0x11)==0x5F)
      Serial.print("Found");
    else
      Serial.print("*** Warning.  Can't find");
      
    Serial.print(" SPI Device with Chip Select Pin=");
    Serial.print(csPin);
    Serial.print("\n\n"); 
  }
    
//////////////////////////////////////

  void reset(){

    digitalWrite(resetPin,HIGH);
    delay(10);
    digitalWrite(resetPin,LOW);
    delay(100);
  }

//////////////////////////////////////

  void setFrequency(double f){
    
    uint32_t x=f/32.0*(1<<19)+0x87000000;
    uint8_t b[4];

    for(int i=0;i<4;i++)
      b[i]=(x&(0xFF<<(24-i*8)))>>(24-i*8);

    digitalWrite(csPin,LOW);
    SPI.transfer(b,4);  
    digitalWrite(csPin,HIGH);
    delay(10);
  }

//////////////////////////////////////

  void setRegister(uint8_t reg, uint8_t val){

    if(reg<1 || reg>0x71){
      Serial.print("*** Error:  Invalid RFM69 register:  \n");
      return;
    }
  
    digitalWrite(csPin,LOW);
    SPI.transfer(reg | 0x80);
    SPI.transfer(val);
    digitalWrite(csPin,HIGH);
    delay(10);
  }

//////////////////////////////////////

  uint8_t getRegister(uint8_t reg){

    uint8_t val;
    
    digitalWrite(csPin,LOW);
    SPI.transfer(reg);
    val=SPI.transfer(0);
    digitalWrite(csPin,HIGH);

    return(val);    
  }

//////////////////////////////////////

  void printRegisters(uint8_t start=1, uint8_t count=1){
  
    if(start==0)      // can't start on FIFO register
      start=1;

    int maxCount=0x71-start+1;      // 0x71 is last valid register

    if(count>maxCount)
      count=maxCount;

    count++;                        // add one for address byte
    uint8_t x[count]={start}; 
    char c[16];
    
    digitalWrite(csPin,LOW);
    SPI.transfer(x,count);
    digitalWrite(csPin,HIGH);
  
    for(int i=1;i<count;i++){
      sprintf(c,"%02X: %02X\n",i+start-1,x[i]);
      Serial.print(c);
    } 
  }  
  
};  // RFM69
