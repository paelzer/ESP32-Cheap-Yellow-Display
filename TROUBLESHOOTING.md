# First, Make sure it's a CYD!

If you are having any issues, this is the first thing I would check! 

The examples and information contained on this repo are for the **ESP32-2432S028** display only. The model number is written on the back of the display in gold writting, beside the speaker connector.

# Display is not turning on

If you are having issues getting the display working, the first thing I would try is [webflashing an existing project](/PROJECTS.md#projects-1). These will be known working code, and if it works correctly, it points to a software issue, not a hardware one.

## If the webflash project displays something on the screen

- Make sure you have put the [User_Setup.h](DisplayConfig/User_Setup.h) file in the correct location [as described here](/SETUP.md#library-configuration)
- Pin 21 is the backlight pin, make sure you are not using it for something else in your sketch.

## The webflash project doesn't display on screen

- Make sure you are not connecting Pin 21 to anything. It is broken out on the connector labeled `P3`
- Try a different USB supply and or cable
- If nothing else worked, your CYD could be faulty. Contact the seller.

# Display, Touch and SD card are not working at the same time

The ESP32 offers two usable hardware SPI buses, but on the CYD each of display, touch and SD card use a different bus. To use all three devices at the same time, for one of them the SPI has to be "simulated" in software. Usually this is done for the touch device, since it doesn't require a high bandwidth. Therefor use a software SPI implementation like [XPT2046_Bitbang_Slim](https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library) and follow the [button example](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/Examples/Basics/8-Buttons)

# Display is flickering

- Try a different USB supply and or cable
- Go through the [Display is not turning on](#display-is-not-turning-on) steps
- If nothing else worked, your CYD could be faulty. Contact the seller.

# Cannot upload
- On Ubuntu and flavors disable or uninstall service `brltty` and make sure user is in group `dialout`
