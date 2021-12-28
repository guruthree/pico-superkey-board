/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 guruthree
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/binary_info.h"

#include "includes/usb.h"
#include "includes/Adafruit_USBD_CDC-stub.h"
#include "Adafruit_TinyUSB_Arduino/src/Adafruit_TinyUSB.h"
#include "TinyUSB_Mouse_and_Keyboard/TinyUSB_Mouse_and_Keyboard.h"

// GPIO pin the keyswitch is on
#define SUPERKEY_PIN 20

// Debounce delay (ms)
#define DEBOUNCE_DELAY 5

// This function is called by a timer to change the on-board LED to flash
// differently depending on USB state
static bool loopTask(repeating_timer_t *rt){
    led_blinking_task();
    return true;
}

// Adafruit TinyUSB instance
extern Adafruit_USBD_Device TinyUSBDevice;

int main() {
    bi_decl(bi_program_description("A single superkey keyboard"));
    bi_decl(bi_program_feature("USB HID Device"));

    board_init(); // Sets up the onboard LED as an output
    TinyUSBDevice.begin(); // Initialise Adafruit TinyUSB

    // Timer for regularly processing USB events
    struct repeating_timer timer;
    add_repeating_timer_ms(10, loopTask, NULL, &timer);

    // Initialise a keyboard (code will wait here to be plugged in)
    Keyboard.begin();

    // Initise GPIO pin as input with pull-up
    gpio_init(SUPERKEY_PIN);
    gpio_set_dir(SUPERKEY_PIN, GPIO_IN);
    gpio_pull_up(SUPERKEY_PIN);

    // Variables for detecting key press
    bool lastState = true; // pulled up by default
    uint32_t lastTime = to_ms_since_boot(get_absolute_time());

    // Main loop
    while (1) {

        // Check GPIO pin, and if more than DEBOUNCE_DELAY ms have passed since 
        // the key changed press release key depending on value (delay is for
        // debounce, ie to avoid rapid changes to switch value)
        bool state = gpio_get(SUPERKEY_PIN);
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - lastTime > DEBOUNCE_DELAY) && state != lastState) {
            if (state) // The pin is pulled up by default, so the logic is backwards 
                Keyboard.release(KEY_LEFT_GUI); // and true is released
            else
                Keyboard.press(KEY_LEFT_GUI);
            lastTime = now;
            lastState = state;
        }

    }
}
