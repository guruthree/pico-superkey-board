## Pico SuperKey Board

This project contains the code for a simple, single key keyboard based on the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/). Short the contacts, get a key press on a USB HID keyboard.

Project goals:
* Add a key on my desk for the missing super (Windows) key on my IBM Model M keyboard.
* Investigate USB device support on the Pico.
* Find an easy way to emulate a USB HID device in C/C++ on the Pico using the Pico SDK.
The last goal was the most important for me as there is (at the time of writing) currently no HID device example in [pico-examples](https://github.com/raspberrypi/pico-examples/tree/3617ade198cfdfca24c047f02a0d6948c1c8fdbf/usb/device/dev_hid_compositehttps://github.com/raspberrypi/pico-examples/commit/40b07ac921ef24d2862993f54a08dd463ebb0a32) code, referring to the standard [TinyUSB example](https://github.com/hathach/tinyusb/tree/d49938d0f5052bce70e55c652b657c0a6a7e84fe/examples/device/hid_composite), which is somewhat lacking in comments to elucidate what does what, and how to compile this with the Pico SDK.

(Note, when this project was originally carried out, there was no Arduino Core for the RP2040.)

Searching on how to use TinyUSB led me to Adafruit's [TinyUSB Arduino](https://github.com/adafruit/Adafruit_TinyUSB_Arduino/) library. This is an implementation of TinyUSB on top of the Arduino core for some chips, including the RP2040. A bit more searching revealed the [TinyUSB Mouse and Keyboard library](https://github.com/cyborg5/TinyUSB_Mouse_and_Keyboard/), a library sitting on top of the Adafruit TinyUSB libraries providing the same API for mouse and keyboard USB HID as the [Arduino API](https://www.arduino.cc/en/Reference/MouseKeyboard). 

My train of thought ran thus: if the libraries could be made to work somehow I would have a familiar interface for emulating a keyboard. The first problem was trying to work out what functions of the code in the Pico SDK example were included within the libraries, and what code I would need to keep from the example. The second problem was that the Arduino libraries above all depend on the Arduino core libraries and certain features of this would need to be re-implemented. The third problem was how to debug and fix any problems along the way.

To tackle both the first and second problem, I started with a blank project, included `"TinyUSB_Mouse_and_Keyboard/TinyUSB_Mouse_and_Keyboard.h"`, and worked my way down the include tree until every missing reference was resolved. This required a `USBDevice` instance from Adafruit TinyUSB. In order to compile, a global define `add_definitions(-DARDUINO_ARCH_RP2040)` was needed in `CMakeLists.txt`. This is the define for the Arduino RP2040 core, and triggers all of the hardware specific functions in Adafruit TinyUSB to make USB functionality work. Most noticeably it sets up a looping task to handle USB packets.

The `USBDevice` handles most of the underlying bits, but annoyingly it also forces an instance of a serial device, which I did not want. In order to not have the serial device, two things were needed. First, I had to use `add_definitions(-DCFG_TUSB_CONFIG_FILE="includes/tusb_config.h")` in `CMakeLists.txt` to specify my own `tusb_config.h` that enabled only the HID USB device class. Second, an `Adafruit_USBD_CDC` class was needed that would create empty objects and methods for those needed by `USBDevice.begin()`.

The bulk of the missing elements from the Arduino core libraries were the `Print`, `Stream`, and `String` classes. The first I could not find a C standard library equivalent for and so needed to be copied directly from the library. The second inherits from `Print`. No new member functions were used and so `Stream` was replaced with `Print`. The last can be replaced with `std::string`. There were additionally some special functions for accessing flash memory that needed to have standard C replacements and `delay()`, which was replaced `sleep_ms()`. I'm not entirely sure about the special function replacements, but testing suggests they work. The substitutions were made entirely using C preprocessor `#define`s in a set of stub `.h` files in `includes/`.

Also, note `USE_TINYUSB` needed to be defined via `add_definitions(-DUSE_TINYUSB)` in the CMakesLists.txt file to enable USB support in the Pico SDK. To define what the USB device name is, `USB_PRODUCT` should similarly be defined, e.g., using `add_definitions(-DUSB_PRODUCT="Pico Keyboard")`. `Adafruit_USBD_Device.cpp` lists additional defines.

Debugging was a slightly annoying task. What was not immediately apparent was that a good chunk of the code in the SDK composite HID example was there to control the Pico's built-in LED based on the state of the USB device and did nothing for actually being the USB device. Particularly, the `board_init()` function for the example just initialises the built-in LED. The `led_blinking_task()` is responsible for turning the LED on and off at the interval specified in `blink_interval_ms`. That variable is altered by the `tud_*_cb()` callback functions from TinyUSB. Adding this code back in allowed me to have a rough idea of what was going on without the ability to use SWD (software debug) or serial print statements over USB. You may be aware of the [Hello World/USB](https://github.com/raspberrypi/pico-examples/tree/3617ade198cfdfca24c047f02a0d6948c1c8fdbf/hello_world/usb) example, which creates a USB serial device that responds to `printf()`. However, examining [stdio_usb.c](https://github.com/raspberrypi/pico-sdk/blob/2d5789eca89658a7f0a01e2d6010c0f254605d72/src/rp2_common/pico_stdio_usb/stdio_usb.c) where this functionality is coded up shows that if we explicitly want to link to TinyUSB, say for creating our own Keyboard device, this functionality is disabled. Thus, debugging was limited to the blinking LED and using well-placed `gpio_put()` calls to turn the LED on once a point in the code was reached. 

The biggest issue I had once the code "should have" worked was that it looked like the Pico was locking up. It would get partway through registering as USB device on my desktop and then time out.
```
Feb 27 19:18:11 localhost kernel: usb 3-6: new full-speed USB device number 20 using xhci_hcd
Feb 27 19:18:11 localhost kernel: usb 3-6: config index 0 descriptor too short (expected 9, got 0)
Feb 27 19:18:11 localhost kernel: usb 3-6: can't read configurations, error -22
```
This turned out to be some issue with the usb initialisation.

Further work... I don't see why the rest of the Adafruit TinyUSB library wouldn't work on the Pico. You should be able to use this (maybe with some Arduino core library tweaks) to have a CDC serial device along with your keyboard device. Or mass storage. Or midi. For whatever devices you're going to have though, remember that `tusb_config.h` will need to be updated.

### Usage

0. Have a working Raspberry Pi Pico C SDK setup.
1. Clone/download the repository. Don't forget to `submodule init` if needed.
2. Ensure that there is a symbolic link to [`pico_sdk_import.cmake`](https://github.com/raspberrypi/pico-examples/blob/13f89f628258b398ed07cf715ee3432e16e4e76a/pico_sdk_import.cmake) from the Pico C SDK. (If a symbolic link isn't an option, just copy the file.)
3. Create a build directory, `cd` to it, and run `cmake ../`. (Or your build system equivalents.)
4. Build by running `make`. (Or your build system equivalent.)
5. Reset (or plug in) the Pico holding down the `BOOTSEL` and copy `pico-superkey-board.uf2` to the Pico's USB mass storage device.
6. You should have a keyboard device now. Shorting the `GP20` pin (defined by `SUPERKEY_PIN` in `pico-superkey-board.cpp`) to ground will trigger a keypress of the super (Windows) key.

Life during development was made a lot easier by being able to reset the Pico by shorting the `RUN` pin to ground. My clever way to do this is using an [unfolded paperclip](https://www.youtube.com/watch?v=fqMhhFJ3tiQ).
