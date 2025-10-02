cmake_minimum_required(VERSION 3.13)

# Find Python3 Interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

include(src/pico/pico_sdk_import.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GattUtils)

project(pico_hid_devices C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# --- Initialize the Pico SDK ---
pico_sdk_init()

# --- Common sources and libraries ---
set(COMMON_SOURCES
    src/hid/HidDevice.cpp
    src/bt/BtStackManager.cpp
    src/pico/picow_bt_example_common.cpp
    src/pico/picow_bt_example_background.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/device_information_service_server.c
)
set(COMMON_LIBS
    pico_stdlib
    # pico_cyw43_arch_threadsafe_background has been removed
    pico_btstack_cyw43
    pico_btstack_ble
    hardware_adc
)

# --- 1. Define the Keyboard Application (unchanged) ---
set(KEYBOARD_TARGET_NAME keyboard_app)
add_executable(${KEYBOARD_TARGET_NAME}
    src/keyboard/keyboard_main.cpp
    src/hid/HidKeyboard.cpp
    src/keyboard/StdinController.cpp
    src/keyboard/DemoController.cpp
    src/keyboard/KeyboardLayout.cpp
    src/keyboard/hog_keyboard_demo.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    ${COMMON_SOURCES}
)
pico_btstack_make_gatt_header(${KEYBOARD_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard/hog_keyboard_demo.gatt)
target_link_libraries(${KEYBOARD_TARGET_NAME} PRIVATE ${COMMON_LIBS})
target_include_directories(${KEYBOARD_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/ble
    ${CMAKE_CURRENT_BINARY_DIR}/generated/keyboard_app_gatt_header
)
target_link_options(${KEYBOARD_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
target_compile_definitions(${KEYBOARD_TARGET_NAME} PRIVATE PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000 CYW43_LWIP=0)
pico_enable_stdio_uart(${KEYBOARD_TARGET_NAME} 1)
pico_add_extra_outputs(${KEYBOARD_TARGET_NAME})

# --- 2. Define the Media Controller Application ---
set(MEDIA_TARGET_NAME media_app)
add_executable(${MEDIA_TARGET_NAME}
    src/media/media_main.cpp
    src/media/MediaControllerDevice.cpp
    src/media/MediaApplication.cpp
    src/pico/RotaryEncoder.cpp
    src/display/Display.cpp
    src/display/Drawing.cpp
    src/net/TcpServer.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    ${COMMON_SOURCES}
)

# --- Link all necessary libraries for Wi-Fi + BLE ---
# The lwip_threadsafe_background library provides everything needed for both
# Wi-Fi (lwIP) and Bluetooth (BTstack) to coexist.
target_link_libraries(${MEDIA_TARGET_NAME} PRIVATE 
    ${COMMON_LIBS}
    hardware_pio
    pico_cyw43_arch_lwip_threadsafe_background 
)

# --- Generate headers ---
pico_generate_pio_header(${MEDIA_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/display/st7789_lcd.pio)
pico_btstack_make_gatt_header(${MEDIA_TARGET_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/media/media_controller.gatt
)

# --- Configure Media App ---
target_include_directories(${MEDIA_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/ble
    ${CMAKE_CURRENT_LIST_DIR}/include/media
    ${CMAKE_CURRENT_LIST_DIR}/include/net
    ${CMAKE_CURRENT_BINARY_DIR}/generated/media_app_gatt_header
    
    # --- THE FIX: Add BOTH required lwIP include paths ---
    ${PICO_SDK_PATH}/lib/lwip/src/include 
    ${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include
)

target_compile_definitions(${MEDIA_TARGET_NAME} PRIVATE 
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    CYW43_LWIP=1
)

target_link_options(${MEDIA_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
pico_enable_stdio_uart(${MEDIA_TARGET_NAME} 1)
pico_add_extra_outputs(${MEDIA_TARGET_NAME})