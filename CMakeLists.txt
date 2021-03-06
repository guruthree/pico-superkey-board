cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico-superkey-board_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(pico-superkey-board
    pico-superkey-board.cpp
    TinyUSB_Mouse_and_Keyboard/TinyUSB_Mouse_and_Keyboard.cpp
    Adafruit_TinyUSB_Arduino/src/Adafruit_USBD_HID.cpp
    Adafruit_TinyUSB_ArduinoCore/Adafruit_USBD_Device.cpp
    Print.cpp
)

target_include_directories(pico-superkey-board PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(pico-superkey-board
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
)
pico_add_extra_outputs(pico-superkey-board)

add_definitions(-DUSE_TINYUSB)
add_definitions(-DUSB_PRODUCT="Pico Keyboard")

include_directories(includes
    TinyUSB_Mouse_and_Keyboard
    Adafruit_TinyUSB_Arduino/src
    Adafruit_TinyUSB_ArduinoCore
)
