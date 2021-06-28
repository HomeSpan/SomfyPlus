# SomfyRTS
A universal, multi-channel, HomeKit Controller for Somfy RTS Motorized Window Shades and Patio Screens. Runs on an ESP32 device as an Arduino sketch using the Arduino [HomeSpan Library](https://github.com/HomeSpan/HomeSpan).

Hardware used for this project:

* An ESP32 board, such as the [Adafruit HUZZAH32 – ESP32 Feather Board](https://www.adafruit.com/product/3405)
* An RFM69 Transceiver, such as this [RFM69HCW FeatherWing](https://www.sparkfun.com/products/10534) from Adafruit
* One regular-size pushbutton (normally-open) to serve as both the Somfy PROG button and the channel SELECTOR button
* Three regular-size pushbuttons (normally-open) to serve as the Somfy UP, DOWN, and MY buttons (optional)
* One small pushbutton (normally-open) to serve as the HomeSpan Control Button (optional)

## Overview

Somfy motors are widely used in automated window shades, patios screens, and porch awnings.  And though there are many different models, almost all are controlled with an RF system called RTS ([Radio Technology Somfy](https://asset.somfy.com/Document/dcb579ff-df8d-47d8-a288-01e06a4480ab_RTS_Brochure_5-2019.pdf)) that uses Somfy RF controllers, such as the 5-channel [Somfy Tellis RTS](https://www.somfysystems.com/en-us/products/1810633/telis-rts).

All Somfy remotes feature:

* an UP button that typically raises the window shade or screen until it is fully opened
* a DOWN button that typically lowers the window shade or screen until it is fully closed
* a button labeled "MY" that serves two purposes - 
  * if the shade is moving, pressing the MY button stops the motor
  * if the shade it stopped, pressing the MY button moves the shade to a predefined position (the "MY" position)
* a PROG button that is used to put the motor into programming mode so you can add additional remotes
* a channel SELECTOR button, if the remote allows the user to control more than one shade or screen

Based on the **superb** work by [Pushstack](https://pushstack.wordpress.com/somfy-rts-protocol/) and other contributors, who reverse-engineered and documented the Somfy-RTS protcols (much thanks!), we can construct a fully-functional, *HomeKit-enabled*, multi-channel Somfy remote using an ESP32, a simple transmitter, and the Arduino HomeSpan Library.

Apart from the obvious benefit of having HomeKit control of your Somfy shades and screens, our HomeSpan version of the Somfy remote also includes two additional benefits:

* The remote allows for an arbitrary number of channels.  Have 20 window shades spread across 5 rooms?  No problem - you can control all of them with a single HomeSpan Somfy device.

* **Use HomeKit to set the absolute position of your window shade or screen!**  HomeKit natively supports a slider that allows you to specify the exact position of a window shade, from fully open (100%) to fully closed (0%) in increments of 1%.  Unfortunately, the Somfy RTS system does not generally support two way communications, nor do the motors transmit any status about the position of the shade or screen.  However, some clever logic inside the sketch and a few timing parameters is all that is needed to configure the HomeSpan Somfy device to track and directly set a window shade to any desired target position.

## Constructing the Hardware

In addition to an ESP32 board, HomeSpan Somfy requires a "433 MHz" transmitter.  However, rather than using a standard carrier frequency of 433.92 MHz, Somfy RTS uses a carrier frequency of 433.42 MHz, which is 0.5 MHz lower than the standard.  Though it is possble to use a standard 433.92 MHz transmitter (such as the one used to construct a HomeSpan remote control for a [Zephyr Kitchen Vent Hood](https://github.com/HomeSpan/ZephyrVentHood)), there is no guarantee that the Somfy motor will accurately receive the RF signal, or that the range will allow for whole-home coverage.

Instead, this project uses an RFM69 *programmable* 434 MHz transceiver that can be configured to use a carrier frequency of 433.42 MHz to properly match the Somfy RTS system.  The ESP32 communicates with the RFM69 via the ESP32's external SPI bus.  This requires you to connect the MOSI, MISO, and SCK pins on your ESP32 to those same pins on your RFM69.  If you are using Adafruit's RFM69 FeatherWing in combination with Adafruit's ESP32 Feather Board, these connections are already hardwired for you.  You'll also need to make these 3 other connections between the ESP32 and the RFM69:

* The SPI Chip Select ("CS") Pin on the RMF69 needs to be connected to a pin on the ESP32 that will be used to enable the RMF69 SPI bus.  This sketch uses GPIO pin 33 on the ESP32 for the RFM69 Chip Select.  If you are using the AdaFruit combination of boards above, simply solder a jumper wire between the through-holes labeled "CS" and "B" ("B" is conveniently hardwired to GPIO pin 33) on the RFM69 FeatherWing.

* The Reset Pin on the of the RFM69 needs to be connected to a pin on the ESP32 that will be used to reset the configuration of the RFM69 settings.  This sketch uses GPIO pin 27.  If you are using the AdaFruit combination of boards above, simply solder a jumper wire between the through-holes labeled "RST" and "A" ("A" is conveniently hardwired to GPIO pin 27) on the RFM69 FeatherWing.

* The DIO2 Pin on the RFM69 needs to be connected to a pin on the ESP32 that will be used to output the Somfy RF codes generated by our sketch so they can be read by the RFM69 and converted to 433.42 MHz signals.  This sketch uses GPIO pin 4.  If you are using the AdaFruit combination of boards above, simply solder a jumper wire between the through-holes labeled "DIO2" and "F" ("F" is conveniently hardwired to GPIO pin 4) on the RFM69 FeatherWing.

You can of course use different pins for any of the above connections.  Just make sure to update the pin definitions at the top of the sketch to match whatever pins you have chosen:

```C++
// Assign pins for RFM69 Transceiver

#define RFM_SIGNAL_PIN    4       // this is the pin on which HomeSpan RFControl will generate a digital RF signal.  MUST be connected to the DIO2 pin on the RFM69
#define RFM_CHIP_SELECT   33      // this is the pin used for SPI control.  MUST be connected to the SPI Chip Select pin on the RFM69
#define RFM_RESET_PIN     27      // this is the pin used to reset the RFM.  MUST be connected to the RESET pin on the RFM69
```
NOTE:  If instead of using an RFM69 you decide to try a standard, non-programmable 433.92 MHz transmiter, you can skip all the connections above except for the RF Signal, which should still be connected from pin 4 on the ESP32 (or any alternative pin you chose) to the signal input pin of your transmitter.  The sketch will warn you that it cannot find the RFM69 when it first runs, but should work fine without modification.

Finally, don't forget to solder an antenna wire (approximately 16.5cm in length) to the antenna pad, pin, or through-hole on the RFM69.

The HomeSpan Somfy device also makes use of 5 pushbutton switches (4 are optional, 1 required).  The required pushbutton serves as both the Somfy PROG button as well as the device's channel SELECTOR button.  Three additional pushbutton switches serve as the Somfy UP, DOWN, and MY buttons.  These are optional and only need to be installed if you want to control a window shade or screen manually with pushbuttons in addition to using HomeKit.  The fifth pushbutton serves as the HomeSpan Control Button.  It is also optional since all of its functions can be alternatively accessed from the Arduino Serial Monitor using the [HomeSpan Command Line Interface](https://github.com/HomeSpan/HomeSpan/blob/master/docs/CLI.md).

Each pushbutton installed should connect a particular ESP32 pin (see below) to ground when pressed.  HomeSpan takes case of debouncing the pushbuttons so no additional hardware is needed.  The pin definitions for the HomeSpan Somfy device are defined in the sketch as follows:

```C++
// Assign pins for the physical Somfy pushbuttons

#define PROG_BUTTON   17      // must have a button to enable programming remote (also serves as the channel SELECTOR button)
#define UP_BUTTON     26      // button is optional
#define MY_BUTTON     25      // button is optional
#define DOWN_BUTTON   23      // button is optional
```

You can of course choose your own pins for any button provided you update the definitions accordingly.

HomeSpan uses GPIO pin 21 as the default for connecting the HomeSpan Control Button, but this too can be changed by calling `homeSpan.setControlPin(pin)` somewhere at the top of the sketch *before* the call to `homeSpan.begin()`.

If using the Adafruit RFM69 FeatherWing, this is what the default wiring above will look like:

![RFM69 Wiring](images/RFM69.png)

Note this diagram includes an optional LED wired to the signal pin that will flash whenever the RFM69 transmits data.

## Configuring the Software

Apart from possibly changing the default pin definitions above, the only other configuration required is to instantiate a Somfy Service for each window shade or screen you want to control with the HomeSpan Somfy device, using the following CREATE_SOMFY macro:

`CREATE_SOMFY(uint32_t channel, uint32_t raiseTime, uint32_t lowerTime`

* *channel* - the channel number you want to assign to the window shade or screen.  Must be greater than zero and less than 2^32-1
* *raiseTime* - the time it takes (in milliseconds) for the shade or screen to raise from fully closed to fully open
* *lowerTime* - the time it takes (in milliseconds) for the shade or screen to lower from fully open to fully closed

Call the CREATE_SOMFY macro for each shade or screen you want to control with HomeSpan Somfy as such:

```C++
CREATE_CHANNEL(1,21000,19000);          // assign a shade to Somfy Channel #1 with raiseTime=21000 ms and lowerTime=19000 ms
CREATE_CHANNEL(2,11000,10000);          // assign a second shade to Somfy Channel #2 with raiseTime=11000 ms and lowerTime=10000 ms
CREATE_CHANNEL(607,11000,10000);        // assign a third shade to Somfy Channel #607 (channel numbers do not need to be consecutive)
CREATE_CHANNEL(14,11000,10000);         // assign a fourth shade to Somfy Channel #14 (channel numbers do not need to be in any order)

CREATE_CHANNEL(2,5000,3000);            // BAD! Cannot re-use channel numbers within one HomeSpan Somfy device.  Will throw a run-time error.
CREATE_CHANNEL(0,11000,10000);          // BAD! Cannot use zero as a channel number.  Will throw a run-time error.
```

You can add, remove, or modify your channel configuration at any time, even after HomeSpan Somfy has been paired with HomeKit.  Changes you make will automatically be reflected in the Home App on your iOS device.

## Operating your HomeSpan Somfy Device and Linking to Window Shades/Screens

HomeSpan Somfy is designed to operate just as any Somfy multi-channel remote, with the one exception that there are no LEDs or LCD displays to indicate which channel you have selected.  Instead, HomeSpan Somfy visually indicates the selected channel from within the Home App itself.  If you only have instantiated a single channel, there is nothing you need to select, and you can (temporarily) skip the next steps.  But if you created more than one channel, your next steps are to connect HomeSpan Somfy to your WiFi network and then pair the the device to HomeKit.  To do so, follow the general instructions for all HomeSpan devices and configure HomeSpan Somfy either using the [HomeSpan Command Line Interface](https://github.com/HomeSpan/HomeSpan/blob/master/docs/CLI.md) or using the HomeSpan Control Button (if you elected to installed one) as described in the [HomeSpan User Guide](https://github.com/HomeSpan/HomeSpan/blob/master/docs/UserGuide.md).

Upon pairing with HomeKit, each channel you instantiated should appear as a separate tile in your Home App with a default name indicating the channel number (e.g. *Channel-1*, *Channel-607*).  As with all HomeKit tiles, you may rename this to anything you'd like, such as *Living Room Window Shade*.  Regardless of the name, HomeKit retains the proper connection back to the original channel number you specified in the sketch.

Once pairing is complete (or if you skipped the WiFi configuration and pairing steps since you only have instatiated one channel) you are now ready to link your channels to actual window shades or screens.

To select the channel you want to link, briefly press and release the PROG/SELECTOR button on the HomeSpan Somfy.  This should light up one of the tiles just created in the Home App with an "obstruction" icon, which is your visual cue that it has been selected.  Repeated press-and-releases of the PROG/SELECTOR button causes the indicator to cycle through each of the tiles.  Once you've selected the desired channel, the rest of the process follows standard Somfy procedures.

1. Find the Somfy remote you currently use to operate your window shade.  If it's a multi-channel remote, make sure you select the channel corresponding the window shade you want to link to the HomeSpan Somfy device.  Press and HOLD (for about 6 seconds) the PROG button on your existing Somfy remote (it is usually on the back of the remote and can be pressed with a paper clip or the tip of a pen) until the window shade or screen "jogs" up and down, which indicates it is now in "programming mode".  The shade will remain in this mode for about two minutes before it automatically falls back to normal operations.

2. Press and HOLD the PROG/SELECTOR button on the HomeSpan Somfy device until the window shade or screen once again jogs up and down.

Congratulations!  HomeSpan Somfy is now linked to your window shade or screen.  Press the UP button on the HomeSpan Somfy device (if you elected to install one), and the shade should rise.  Let it rise all the way the first time until it is fully open.  Next, press the DOWN button, and the shade should start to lower.  Press the MY button while the shade is moving, and the shade should stop.  Press the DOWN button again and let the shade fully close.

Now try operating the shade through your Home App (if you skipped the WiFi and HomeKit pairing step above, you'll need complete that before proceeding).  Press the tile corresponding to the window shade.  The shade should start rising and the tile should indicate it is "opening".  Shortly after the shade reaches the top, the tile icon changes from "opening" to "open" to indicate the shade is fully open.  Pressing the tile a second time should cause the shade to lower and the tile should say "closing".  Before it reaches the bottom, press the MY button on the HomeSpan Somfy device to stop the shade.  The tile icon should change to indicate the shades's position as a percentage from 0-100%.

Finally, long-press the tile icon to bring up the Home App slider control, which should reflect the current position of the shade.  **Move the slider either up or down and the window shade will follow, stopping at whatever target position you set from fully open to fully closed, or anywhere in between!**

How does HomeSpan Somfy do this?  It keep track of how long the shade has been moving up or down, and estimates its position based on how long it takes for the shade or screen to fully raise or fully close.  This is why you needed to specify these two parameters in the CREATE_SOMFY macro.  

This is of course just an estimate, since the window shade or screen is not actually broadcasting its position.  To ensure tracking is as accurate as possible, HomeSpan Somfy resets its estimate of the shade's position to either fully open or fully closed, every time you move the shade to the fully open or fully closed position and allow it to remain in that state for a few seconds.

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




