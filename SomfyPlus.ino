/*********************************************************************************
 *  MIT License
 *  
 *  Copyright (c) 2020-2021 Gregg E. Berman
 *  
 *  https://github.com/HomeSpan/SomfyPlus
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *  
 ********************************************************************************/

// Define Somfy 19-bit address for this device (must be unique across devices)

#define SOMFY_ADDRESS  0x1000              // 0x0000 through 0x7FFFF

// Assign pins for the physical Somfy pushbuttons
// A LONG press of the MY_BUTTON serves as the HomeSpan Control Button
// A DOUBLE press of the MY_BUTTON serve as Shade Selector
// A simultaneous LONG press of both the UP and DOWN Buttons serve as the Somfy PROG Button

#define DOWN_BUTTON   16         
#define MY_BUTTON     22  
#define UP_BUTTON     25

// Assign pins for RFM69 Transceiver

#define RFM_SIGNAL_PIN    4       // this is the pin on which HomeSpan RFControl will generate a digital RF signal.  MUST be connected to the DIO2 pin on the RFM69
#define RFM_CHIP_SELECT   33      // this is the pin used for SPI control.  MUST be connected to the SPI Chip Select pin on the RFM69
#define RFM_RESET_PIN     27      // this is the pin used to reset the RFM.  MUST be connected to the RESET pin on the RFM69

#define SKETCH_VERSION  "2.0.0"       // version of the Homespan SomfyPlus sketch
#define REQUIRED VERSION(1,3,0)       // required version of the HomeSpan Library

#include "HomeSpan.h" 
#include "DEV_Identify.h"       
#include "DEV_Somfy.h"

void setup() {
 
  Serial.begin(115200);

  homeSpan.setLogLevel(1);
  homeSpan.setControlPin(MY_BUTTON);
  homeSpan.enableOTA();
  homeSpan.setSketchVersion(SKETCH_VERSION);
  
  new SpanUserCommand('D',"- delete Somfy Shade data and Restart",SomfyShade::deleteData);

  homeSpan.begin(Category::WindowCoverings,"Somfy-HomeSpan");

  // DEFINE SOMFY SHADES HERE //
  // First argument is channel number [1-32]
  // Second argument is the name of the Somfy Shade as it will appear in the Home App on your iPhone
  
    new SomfyShade(1,"Screen Door");

//  new SomfyShade(2,"Living Room Window Shade");
//  new SomfyShade(6,"Den Blinds");
//  new SomfyShade(3,"Den Curtains");

} // end of setup()

//////////////////////////////////////

void loop(){
  
  homeSpan.poll();
  SomfyShade::poll();
  
} // end of loop()

//////////////////////////////////////
