# CMakeLists.txt - C++ Refactoring Step 5 (Corrected)

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(pico_hid_keyboard C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(TARGET_NAME hog_keyboard_demo)
add_executable(${TARGET_NAME}
    hog_keyboard_demo.cpp
    HidKeyboard.cpp
    BtStackManager.cpp
    KeyboardLayout.cpp
    picow_bt_example_common.cpp
    picow_bt_example_background.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/device_information_service_server.c
)

pico_btstack_make_gatt_header(${TARGET_NAME} PRIVATE hog_keyboard_demo.gatt)

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    # --- FIX: Corrected the typo from CYW4L3 to CYW43 ---
    CYW43_LWIP=0
)

target_link_options(${TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")

target_link_libraries(${TARGET_NAME} PRIVATE
    pico_stdlib
    pico_cyw43_arch_threadsafe_background
    pico_btstack_cyw43
    pico_btstack_ble
    hardware_adc
)

pico_enable_stdio_uart(${TARGET_NAME} 1)
pico_add_extra_outputs(${TARGET_NAME})