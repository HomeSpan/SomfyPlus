# SomfyPlus - A HomeSpan Project

SomfyPlus is a universal, multi-channel, HomeKit Controller for Somfy RTS Motorized Window Shades and Patio Screens. Runs on an ESP32 device as an Arduino sketch.  Built with [HomeSpan](https://github.com/HomeSpan/HomeSpan).

## Overview

Somfy motors are widely used in automated window shades, patios screens, and porch awnings.  And though there are many different models, almost all are controlled with an RF system called RTS ([Radio Technology Somfy](https://asset.somfy.com/Document/dcb579ff-df8d-47d8-a288-01e06a4480ab_RTS_Brochure_5-2019.pdf)) that uses Somfy RF controllers, such as the 5-channel [Somfy Tellis RTS](https://www.somfysystems.com/en-us/products/1810633/telis-rts).

All Somfy remotes feature:

* an UP button that typically raises the window shade or screen until it is fully opened
* a DOWN button that typically lowers the window shade or screen until it is fully closed
* a button labeled "MY" that serves two purposes - 
  * if the shade is moving, pressing the MY button stops the motor
  * if the shade it stopped, pressing the MY button moves the shade to a predefined position (the "MY" position)
* a PROGRAM button that is used to put the motor into programming mode so you can add additional remotes
* a channel SELECTOR button, if the remote allows the user to control more than one shade or screen

Based on the **superb** work by [Pushstack](https://pushstack.wordpress.com/somfy-rts-protocol/) and other contributors, who reverse-engineered and documented the Somfy-RTS protcols (much thanks!), we can construct a fully-functional, *HomeKit-enabled*, multi-channel Somfy remote using an ESP32, a simple RF transmitter, and the Arduino [HomeSpan Library](https://github.com/HomeSpan/HomeSpan).

Apart from the obvious benefit of having HomeKit (and Siri!) control of your Somfy shades and screens, a SomfyPlus remote also includes two additional benefits:

* **Control up to 32 channels!**  Have 20 window shades spread across 5 rooms?  No problem - you can operate all of them with a single SomfyPlus device.

* **Use HomeKit to set the absolute position of your window shade or screen!**  HomeKit natively supports sliders that allow you to specify the exact position of a window shade, from fully open (100%) to fully closed (0%) in increments of 1%.  Unfortunately, the Somfy RTS system does not generally support two way communications, nor do the motors transmit any status about the position of the shade or screen.  However, some clever logic inside the sketch and a few timing parameters is all that is needed to configure SomfyPlus to track and directly set a window shade to any desired target position.

## Before You Begin

This is an intermediate-level project that assumes you are already familiar with HomeSpan, including how to: compile Arduino sketches using the HomeSpan Library; configure a HomeSpan device with your home network's WiFi Credentials; pair a HomeSpan device to HomeKit; use the HomeSpan Command Line Interface (CLI); use the HomeSpan Control Button and Status LED.  If you are unfamiliar with these processes, or just need a refresher, please visit the [HomeSpan](https://github.com/HomeSpan/HomeSpan) and review [Getting Started with HomeSpan](https://github.com/HomeSpan/HomeSpan/blob/master/docs/GettingStarted.md) before tackling this project.  Note SomfyPlus does not require you to develop any of your own HomeSpan code (the sketch contains everything your need), but you nevertheless may want to first try out some of the [HomeSpan Tutorials](https://github.com/HomeSpan/HomeSpan/blob/master/docs/Tutorials.md) to ensure HomeSpan operates as expected in your environment.

## Step 1: Configuring the Software

SomfyPlus is designed to operate as a bridge where each each window shade or screen is implemented as a separate HomeKit Accessory containing a single instance of a *Window Covering Service*.  The logic for each Somfy shade or screen is encapsulated in the `SomfyShade()` class.

To customize SomfyPlus for your own home, simply modify the *SomfyPlus.ino* sketch file so that you have an instance of `SomfyShade()` for each Somfy shade or screen you want to control with with SomfyPlus as follows: 

`new SomfyShade(uint8_t channel, char *name, uint32_t raiseTime=10000, uint32_t lowerTime=10000);`

* *channel* - the channel number you want to assign to the window shade or screen.  Must be between 1 and 32
* *name* - the name of the Somfy Shade as it will appear in the Home App on your iPhone

For example, the code snippet below would be used to create 4 window shade/screens in SomfyPlus.  Note that the channel numbers you specify do not need to be consecutive - they can be in any order:

```C++
new SomfyShade(1,"Screen Door");
new SomfyShade(2,"Living Room Window Shade");
new SomfyShade(6,"Den Blinds");
new SomfyShade(3,"Den Curtains");
```


To add and configure The only configuration required is to instantiate a Somfy Shade Accessory for each window shade or screen you want to control with the HomeSpan Somfy device, using the following class:





You can add, remove, or modify your channel configuration at any time, even after HomeSpan Somfy has been paired with HomeKit.  Changes you make will automatically be reflected in the Home App on your iOS device.

## Constructing the Hardware


Hardware used for this project:

* An ESP32 board, such as the [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)
* An RFM69 Transceiver, such as this [RFM69HCW FeatherWing](https://www.sparkfun.com/products/10534) from Adafruit
* Three pushbuttons (normally-open) to serve as the Somfy UP, DOWN, and MY buttons (the MY button also serves as the HomeSpan Control Button)
* One LED and current-limiting resistor to serve as the HomeSpan Status LED
* One LED and current-limiting resistor to provide visual feedback when RFM69 is transmitting Somfy RTS signals

In addition to an ESP32 board, SomfyPlus requires a "433 MHz" transmitter.  However, rather than using a standard carrier frequency of 433.92 MHz, Somfy RTS uses a carrier frequency of 433.42 MHz, which is 0.5 MHz lower than the standard.  Though it is possble to use a standard 433.92 MHz transmitter (such as the one used to construct a HomeSpan remote control for a [Zephyr Kitchen Vent Hood](https://github.com/HomeSpan/ZephyrVentHood)), there is no guarantee that the Somfy motor will accurately receive the RF signal, or that the range will allow for whole-home coverage.

Instead, this project uses an RFM69 *programmable* 434 MHz transceiver that can be configured to generate a carrier frequency of 433.42 MHz to properly match the Somfy RTS system.  The ESP32 communicates with the RFM69 via the ESP32's external SPI bus.  This requires you to connect the MOSI, MISO, and SCK pins on your ESP32 to those same pins on your RFM69.  If you are using Adafruit's RFM69 FeatherWing in combination with Adafruit's ESP32 Feather Board, these connections are already hardwired for you.  However, you'll also need to make these 3 other connections between the ESP32 and the RFM69 (see diagram below):

* The SPI Chip Select ("CS") Pin on the RMF69 needs to be connected to a pin on the ESP32 that will be used to enable the RMF69 SPI bus.  This sketch uses GPIO pin 33 on the ESP32 for the RFM69 Chip Select.  If you are using the AdaFruit combination of boards above, simply solder a jumper wire between the through-holes labeled "CS" and "B" ("B" is conveniently hardwired to GPIO pin 33) on the RFM69 FeatherWing.

* The Reset Pin on the of the RFM69 needs to be connected to a pin on the ESP32 that will be used to reset the configuration of the RFM69 settings.  This sketch uses GPIO pin 27.  If you are using the AdaFruit combination of boards above, simply solder a jumper wire between the through-holes labeled "RST" and "A" ("A" is conveniently hardwired to GPIO pin 27) on the RFM69 FeatherWing.

* The DIO2 Pin on the RFM69 needs to be connected to a pin on the ESP32 that will be used to output the Somfy RF codes generated by SomfyPlus so they can be read by the RFM69 and converted to 433.42 MHz signals.  This sketch uses GPIO pin 4.  If you are using the AdaFruit combination of boards above, simply solder a jumper wire between the through-holes labeled "DIO2" and "F" ("F" is conveniently hardwired to GPIO pin 4) on the RFM69 FeatherWing.

You can of course use different pins for any of the above connections.  Just make sure to update the pin definitions at the top of the sketch to match whatever pins you have chosen:

```C++
// Assign pins for RFM69 Transceiver

#define RFM_SIGNAL_PIN    4       // this is the pin on which HomeSpan RFControl will generate a digital RF signal.  MUST be connected to the DIO2 pin on the RFM69
#define RFM_CHIP_SELECT   33      // this is the pin used for SPI control.  MUST be connected to the SPI Chip Select pin on the RFM69
#define RFM_RESET_PIN     27      // this is the pin used to reset the RFM.  MUST be connected to the RESET pin on the RFM69
```

Finally, don't forget to solder an antenna wire (approximately 16.5cm in length) to the antenna pad, pin, or through-hole on the RFM69.

SomfyPlus requires 3 normally-open pushbutton switches to function as the Somfy UP, DOWN, and MY buttons. Each pushbutton installed should connect a particular ESP32 pin (see diagram below) to ground when pressed.  HomeSpan takes care of debouncing the pushbuttons so no additional hardware is needed.  This sketch uses the following pins:

```C++
// Assign pins for the physical Somfy pushbuttons

#define DOWN_BUTTON   16         
#define MY_BUTTON     22  
#define UP_BUTTON     25
```

You can of course choose your own pins for any button provided you update the definitions accordingly.

Finally, SomfyPlus utilizes two LEDs (each with a current-limiting resistor).  One LED serves as the HomeSpan Status LED and is generally connected to pin 13, which is HomeSpan's default.  The second serves as a visual indicator of transmissions and should be connected to the RFM_SIGNAL_PIN defined above. 

If using the Adafruit RFM69 FeatherWing, this is what the default wiring above will look like:

![RFM69 Wiring](images/RFM69.png)


## Operating your HomeSpan Somfy Device and Linking to Window Shades/Screens

SomfyPlus is designed to (mostly) operate just as any Somfy multi-channel remote:

* a short press of the UP button raises the shade
* a short press of the DOWN button lowers the shade
* a short press of the MY button stops the shade while it is moving

One difference between SomfyPlus and normal SOmfy remote is that a short press of the MY button when the shade is not movig does not have any effect.  On a normal Somfy remote the MY button can be programmed to move the shade to a pre-programmed location.  For reasons you will see below, this is not needed in SomfyPlus.

A second difference is that there is no distinct PROG button in SomfyPlus to send a "programming" signal to a shade.  Instead, to send a programming signal, press and hold the UP and DOWN buttons *at the same time* for 3 seconds.  The programming signal operates the same as a normal Somfy remote: it is used to link and unlink the remote to a window shade.

To link SomfyPlus to the first shade listed in your sketch, use the original Somfy remote for that shade to send a programmingt signal according to the instructions for that remote.  The shade itself should perform slight up/down movement (a "jog") confirming it has received a programming signal from the original remote.  Next, send a programming signal from SomfyPlus following the procedure above.  The shade should confirm receipt of this signal with a second "jog" and then return to normal operation.  Congratulations, you have just linked your first shade to SomfyPlus. Short-press the UP button and the shade should rise.  Short-press the DOWN button and the shade should lower.  The LED connected to RFM_SIGNAL_PIN should flash with each button press indicating a signal is being transmitted.  Note that SomfyPlus only transmits signals when there is an action to be taken.  For example, if you press the DOWN button when the shade is fully lowered (or SomfyPlus "thinks" it's lowered) a signal will *not* be transmitted.

Once you've confirmed you can operate your shade, it's time to connect SomfyPlus to your home WiFi network and pair it to HomeKit so you can operate it with the Home App on your iPhone, iPad, or Mac (or with Siri).  To do so, please visit the [HomeSpan Library Repository](https://github.com/HomeSpan/HomeSpan) for complete instructions on how to configure and pair HomeSpan devices.  Don't worry if you have more than one shade to configure - this will be done *after* SomfyPlus is paired your Home App.

NOTE: As the HomeSpan instructions explain, HomeSpan devices can be configured using the [HomeSpan Command Line Interface](https://github.com/HomeSpan/HomeSpan/blob/master/docs/CLI.md) or using the HomeSpan Control Button as described in the [HomeSpan User Guide](https://github.com/HomeSpan/HomeSpan/blob/master/docs/UserGuide.md).  SomfyPlus does not include a separate HomeSpan Control Button.  Instead, the MY button serves double-duty and acts as the HomeSpan Control Button.  For example, pressing and holding the MY button will cause SomfyPlus to enter the HomeSpan Command Mode, just as described in the HomeSpan User Guide.

Upon successfully pairing SomfyPlus with HomeKit, each SomfyShade you instantiated in your sketch should appear as a separate tile in the Home App of your iPhone with the name specified in your sketch.  Press the tile corresponding the first shade you instantiated in the sketch.  If everything is working correctly, that shade should now open and close every time you press the tile.

If you have more instantiated more than one shade in your sketch, the next step is to link SomfyPlus to those other shades.  Otherwise the tiles for those shades will operate as normal from your Home App, but there will be no shade listening for the signals SomfyPlus transmits.

On normal Somfy multi-channel remotes there is a selector switch that allows you to choose choose which channel to use.  SomfyPlus does not have a separate selector switch but instead uses the Home App tiles to provide a visual cue indicating whih shade it currently "selected."  To turn on the visual cue, double-press the MY button.  This should cause an warning to appear on the tile for the first shade in your Home App.  This is how you know which shade is currently being selected by SomfyPlus.  Double-press the MY button a second time and this visual cue should advance to the next tile.  If you now press the UP or DOWN buttons on SomfyPlus, it will transmit signals coded for the channel you defined for the second shade.

To link SomfyPlus to the second stage, repeat the same steps you performed for the first shade: use the original remote for that shade to send a programming signal; confirm the second shade makes a small jog.  Press and hold both the UP and DOWN button on SomfyPlus for 3 seconds, and confirm the shade makes another small jog.  That's it - SomfyPlus is now linked to the second shade.  Confirm everything is working by pressing either the first or second tile in the Home App to operate either the first or second shade.  Repeat these steps as many times as needed untl you've linked all the shades instantiated in your sketch.

Note that double-pressing the MY button to select a different shade only effects which shade the UP and DOWN buttons on the SomfyPlus device control.  Once all your shades are linked, you can control them all directly from the Home App.  You only need to "select" a shade by double-pressing the MY button if you want to operate a share using the UP and DOWN buttons. 

Calibrating the Shade (TBD)


Note that HomeSpan Somfy will track the position of the window shade whether you operate the shade via the Home App or by pressing the UP, DOWN, or MY pushbuttons on the HomeSpan Somfy device.  However, HomeSpan Somfy will lose calibration of the window shade's position if you also operate the shade with your original Somfy remote.  If you do use the original Somfy remote to move the window shade and the position estimate in HomeSpan Somfy gets out of sync with the actual position of the shade, simply use the Home App or the pushbuttons on the HomeSpan Somfy device to fully raise or fully lower the shade, at which point HomeSpan Somfy resets its estimate of the window shade's position to either fully open or fully closed.

## Tips and Tricks

* If the window shade is *not* fully closed but pressing the DOWN pushbutton on the HomeSpan Somfy device does nothing, check the Home App tile.  If it indicates the shade is already closed, this means the device's position estimate has gotten out of sync with the actual position of the shade.  To recalibrate, use the UP button on the HomeSpan Somfy device, or the tile in your Home App that corresponds to the window shade, to raise the shade until it is fully open.

* Alternatively, the problem may be that you are looking at the wrong shade, and are trying to lower a shade that is in fact already fully closed.  To check which shade is selected, press and release the PROG/SELECTOR button on the HomeSpan Somfy device and see which tile in your Home app lights up with the obstruction icon.   If you've selected the wrong shade, press and release the PROG/SELECTOR button again until the correct shade is selected.

* Similarly, if the window shade is *not* fully open but pressing the UP pushbutton on the HomeSpan Somfy device does nothing, follow the same steps as above, except that to recalibrate, move the window shade to the fully closed position.

* Note that the slider in the Home App represents the desired **target** position of the shade, which is not necessarily the same as the **current** position of the shade while it is moving.  For example, if the shade is open and you press the DOWN button, the slider in the Home App will jump immediately to the fully closed position, since this is the new target state, even though the shade will obviously take some time to fully close.

* If you stop the shade while it is moving by pressing the MY button on the HomeSpan Somfy device, the target position will be updated to reflect this new position, and the slider in the Home App will adjust accordingly.

* HomeSpan Somfy visually indicates the current channel selected by flashing an "obstruction" icon on the corresponding tile in your Home App.  This does *not* mean your shade or screen has hit an actual obstruction.  HomeSpan Somfy cannot detect actual obstructions.  Note the obstruction indicator will disappear from the selected Home App tile upon the next press of either the UP or DOWN button.

* To "unlink" a channel from window shade or screen, follow the normal Somfy procedures:  press and hold the PROG button on the **original** Somfy remote until the shade jogs up and down.  Next, short-press the PROG/SELECTOR button on the HomeSpan Somfy device to select the channel you want to unlink from the shade (as visually indicated with an "obstruction" icon on the corresponding Home App tile).  Finally, press and hold the PROG/SELECTOR button on the HomeSpan Somfy device until the shade jogs up and down a second time.   The channel has now been unlinked and will not operate the shade until it is re-linked, or linked to a different shade.

* Even if a channel is not linked to an actual shade, the UP, DOWN and MY buttons, as well as the corresponding tile in the Home App, will operate as normal and the RFM69 will continue to broadcast UP, DOWN, and MY signals, even though there is no window shade or screen listening for this signal.

* Removing a channel from the sketch by deleting its CREATE_SOMFY() macro **does not** unlink the channel from the actual shade.  If you later add that channel back into the sketch, it will resume operating the same window shade or screen.  To unlink the channel from the window shade, you must either peform the steps described above, or reset the shade itself using other Somfy procedures.  Leaving a link in place even though you've deleted the channel from the sketch does not generally produce any problems, though it is good practice to first unlink the channel from the shade or screen before deleting it from the sketch, if you do not intend of re-using that channel with this same shade in the future.

* Like all Somfy remotes, you can link a single HomeSpan Somfy channel to more than one window shade or screen, in which case they will appear as a single tile and operate in tandem (both shades respond to the same RF signal).  Note that linking one channel to two shades will not allow you specify different raiseTimes and lowerTimes for each shade, which may be fine if the shades are identical and the timings are the same.

* Also note that linking more than one shade or screen to the same channel to have them operate in tandem is *not* necessary when using HomeSpan Somfy.  This is because you can instead use the full power of HomeKit to create scenes that control multiple shades!  You can even use HomeKit automations to have different shades raise or lower to different positions at different times of the day. There are countless possibilities.

* Performing a Factory Reset on the HomeSpan Somfy device **does not** delete linking information.  This allows you to reset the device's WiFi and HomeKit pairing info, but retain all the linking data between your channels and your window shades.  Upon reconfiguring the device's WiFi and re-pairing to HomeKit, the same shades and will continue to operate as they did before the Factory Reset.

* On the other hand, fully erasing the HomeSpan Somfy device's non-volatile-storage ("NVS") using the "E" command from within the [HomeSpan Command Line Interface](https://github.com/HomeSpan/HomeSpan/blob/master/docs/CLI.md) **deletes** all linkage information from the device.  Upon re-starting the sketch, all channels will need to be re-linked to a shade or screen.

> Developer's note:  the NVS is used to store the Somfy rolling code for each Somfy channel address.  If the rolling codes are deleted, the Somfy addresses can't be used with the same shades since the rolling codes won't match (though each address could be used with a different shade).  To ensure you can re-link the same channel to the same shade, HomeSpan Somfy re-generates a new set of Somfy channel addresses upon re-start if the NVS is erased.  Technically, previously-linked window shades and screens still retain knowledge of the prior linking, but this should have no effect on the operation of the shade.  If you'd like to see the actual address assigned to any given channel, open the settings page for that channel's tile in the Home App. The original channel number should be listed under "Model" and the 3-byte Somfy address associated with that channel should be listed under the "Serial Number" with the prefix "RTS".

## Ideas, Options and Possibilities

* Use separate pushbuttons for the PROG and SELECTOR functions.  This is readily done with some small modifications to the code.

* Connect an LED from the RFM_SIGNAL_PIN to ground through a 330Ω resistor.  The LED will flash in sync with the RF transmitter providing some visual feedback whenever HomeSpan Somfy opens, closes, or stops a shade, as well as when it sends a PROG signal.

* Power the device with a large, rechargable battery pack so it can be used as a more portable remote (though why bother since you can control everything from the Home App and/or via voice with Siri).

* Start a campaign to convince Apple to add a HomeKit option that allows a user to hold/stop a moving shade from within the Home App itself (and via Siri) so you don't need to push the "MY" button on the HomeSpan Somfy device.  This feature is actually listed as a Service in Apple's HomeKit documentation, but it does not appear to be implemented in the Home App.

* Add a small OLED or 7-segment LED display to the device to provide a more direct indication of the selected channel, instead of needing to light up the "obstruction" icon of the selected tile in the Home App as the only a visual indicator.

* Use (or create) a HomeKit-enabled sun-sensor to trigger HomeSpan Somfy to raise and lower shades.

* Expand the functionality of HomeSpan's embedded web server (which is optionally used to configure a device's WiFi credentials and HomeKit Setup Pairing Code) to allow the creation/modification/deletion of Somfy channels *without the need to ever open the sketch or use the CREATE_SOMFY() macro*.

---

### Feedback or Questions?

Please consider adding to the [Discussion Board](https://github.com/HomeSpan/HomeSpan/discussions), or email me directly at [homespan@icloud.com](mailto:homespan@icloud.com).




