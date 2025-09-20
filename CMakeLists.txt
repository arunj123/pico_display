# CMakeLists.txt - FINAL WORKING VERSION

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_hid_devices C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- Common sources shared by all targets ---
set(COMMON_SOURCES
    HidDevice.cpp
    BtStackManager.cpp
    picow_bt_example_common.cpp
    picow_bt_example_background.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/device_information_service_server.c
)

# --- Common libraries from the SDK, shared by all targets ---
set(COMMON_LIBS
    pico_stdlib
    pico_cyw43_arch_threadsafe_background
    pico_btstack_cyw43
    pico_btstack_ble
    # pico_btstack_stdin  # <-- This is the correct way. The SDK handles the rest.
    hardware_adc
)


# --- 1. Define the Keyboard Application ---
set(KEYBOARD_TARGET_NAME keyboard_app)
add_executable(${KEYBOARD_TARGET_NAME}
    # App-specific sources
    keyboard_main.cpp
    HidKeyboard.cpp
    StdinController.cpp
    DemoController.cpp
    KeyboardLayout.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    # Common sources
    ${COMMON_SOURCES}
)

target_link_libraries(${KEYBOARD_TARGET_NAME} PRIVATE ${COMMON_LIBS})

target_include_directories(${KEYBOARD_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
)
pico_btstack_make_gatt_header(${KEYBOARD_TARGET_NAME} PRIVATE hog_keyboard_demo.gatt)
target_link_options(${KEYBOARD_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
target_compile_definitions(${KEYBOARD_TARGET_NAME} PRIVATE PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000 CYW43_LWIP=0)
pico_enable_stdio_uart(${KEYBOARD_TARGET_NAME} 1)
pico_add_extra_outputs(${KEYBOARD_TARGET_NAME})


# --- 2. Define the Media Controller Application ---
set(MEDIA_TARGET_NAME media_app)
add_executable(${MEDIA_TARGET_NAME}
    # App-specific sources
    media_main.cpp
    MediaControllerDevice.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    # Common sources
    ${COMMON_SOURCES}
)

target_link_libraries(${MEDIA_TARGET_NAME} PRIVATE ${COMMON_LIBS})

target_include_directories(${MEDIA_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
)
pico_btstack_make_gatt_header(${MEDIA_TARGET_NAME} PRIVATE hog_keyboard_demo.gatt)
target_link_options(${MEDIA_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
target_compile_definitions(${MEDIA_TARGET_NAME} PRIVATE PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000 CYW43_LWIP=0)
pico_enable_stdio_uart(${MEDIA_TARGET_NAME} 1)
pico_add_extra_outputs(${MEDIA_TARGET_NAME})