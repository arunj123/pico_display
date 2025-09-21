cmake_minimum_required(VERSION 3.13)

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

project(pico_hid_devices C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- Common sources shared by all targets ---
set(COMMON_SOURCES
    src/hid/HidDevice.cpp
    src/bt/BtStackManager.cpp
    src/pico/picow_bt_example_common.cpp
    src/pico/picow_bt_example_background.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/device_information_service_server.c
)

# --- Common libraries from the SDK, shared by all targets ---
set(COMMON_LIBS
    pico_stdlib
    pico_cyw43_arch_threadsafe_background
    pico_btstack_cyw43
    pico_btstack_ble
    hardware_adc
)


# --- 1. Define the Keyboard Application ---
set(KEYBOARD_TARGET_NAME keyboard_app)
add_executable(${KEYBOARD_TARGET_NAME}
    # App-specific sources
    src/keyboard/keyboard_main.cpp
    src/hid/HidKeyboard.cpp
    src/keyboard/StdinController.cpp
    src/keyboard/DemoController.cpp
    src/keyboard/KeyboardLayout.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    # Common sources
    ${COMMON_SOURCES}
)

# Add generated header for this target
pico_btstack_make_gatt_header(${KEYBOARD_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboard/hog_keyboard_demo.gatt)

# Link libraries and set options
target_link_libraries(${KEYBOARD_TARGET_NAME} PRIVATE ${COMMON_LIBS})
target_include_directories(${KEYBOARD_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_options(${KEYBOARD_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
target_compile_definitions(${KEYBOARD_TARGET_NAME} PRIVATE PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000 CYW43_LWIP=0)
pico_enable_stdio_uart(${KEYBOARD_TARGET_NAME} 1)
pico_add_extra_outputs(${KEYBOARD_TARGET_NAME})


# --- 2. Define the Media Controller Application ---
set(MEDIA_TARGET_NAME media_app)
add_executable(${MEDIA_TARGET_NAME}
    # App-specific sources
    src/media/media_main.cpp
    src/media/MediaControllerDevice.cpp
    src/media/MediaApplication.cpp
    src/pico/RotaryEncoder.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    # Common sources
    ${COMMON_SOURCES}
)

# Add generated headers for this target
pico_generate_pio_header(${MEDIA_TARGET_NAME}  ${CMAKE_CURRENT_SOURCE_DIR}/src/pico/pio_rotary_encoder.pio)
pico_btstack_make_gatt_header(${MEDIA_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/media/media_controller.gatt)

# Link libraries and set options
target_link_libraries(${MEDIA_TARGET_NAME} PRIVATE 
    ${COMMON_LIBS}
    hardware_pio
)
target_include_directories(${MEDIA_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_options(${MEDIA_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
target_compile_definitions(${MEDIA_TARGET_NAME} PRIVATE PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000 CYW43_LWIP=0)
pico_enable_stdio_uart(${MEDIA_TARGET_NAME} 1)
pico_add_extra_outputs(${MEDIA_TARGET_NAME})

#  *  Executing task: C:\Users\arunj/.pico-sdk/openocd/0.12.0+dev/openocd.exe -s C:\Users\arunj/.pico-sdk/openocd/0.12.0+dev/scripts -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c adapter speed 5000; program "C:/Users/arunj/projects/pico/picow_ble_temp_sensor/build/picow_ble_temp_sensor.elf" verify reset exit 
# C:\Users\arunj/.pico-sdk/openocd/0.12.0+dev/openocd.exe -s C:\Users\arunj/.pico-sdk/openocd/0.12.0+dev/scripts -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "init; reset halt; flash erase_address 0x10000000 0x200000; reset; exit"