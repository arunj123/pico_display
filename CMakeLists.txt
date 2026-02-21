cmake_minimum_required(VERSION 3.13)

# Find Python3 Interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)

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

# Fallback for WSL or pure Linux builds
if(NOT DEFINED ENV{PICO_SDK_PATH})
    if(EXISTS "${USERHOME}/.pico-sdk/sdk/2.2.0")
        set(ENV{PICO_SDK_PATH} "${USERHOME}/.pico-sdk/sdk/2.2.0")
    endif()
endif()

set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(src/pico/pico_sdk_import.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(GattUtils)

project(pico_hid_devices C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(COMMON_SOURCES
    src/hid/HidDevice.cpp
    src/bt/BtStackManager.cpp
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/device_information_service_server.c
)
set(COMMON_LIBS
    pico_stdlib
    pico_btstack_cyw43
    pico_btstack_ble
    hardware_adc
)

# --- Media App ---
set(MEDIA_TARGET_NAME media_app)

# A. Create a library for the Setup Profile
add_library(setup_profile_lib src/media/SetupProfile.c)
pico_btstack_make_gatt_header(setup_profile_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/media/setup_mode.gatt)

# B. Main Media Executable
add_executable(${MEDIA_TARGET_NAME}
    src/media/media_main.cpp
    src/media/MediaApplication.cpp
    src/media/MediaControllerDevice.cpp
    src/pico/RotaryEncoder.cpp
    src/display/Drawing.cpp
    src/display/Display.cpp
    src/net/TcpServer.cpp
    src/net/DiscoveryServer.cpp
    src/StatusScreen.cpp
    src/SetupDrawing.cpp
    lib/qrcodegen/qrcodegen.c
    ${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/hids_device.c
    ${COMMON_SOURCES}
)

# Link the Setup Profile library
target_link_libraries(${MEDIA_TARGET_NAME} PRIVATE 
    ${COMMON_LIBS}
    hardware_pio
    pico_cyw43_arch_lwip_threadsafe_background
    setup_profile_lib
)

pico_generate_pio_header(${MEDIA_TARGET_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/display/st7789_lcd.pio)

# Generate the Normal Profile header
pico_btstack_make_gatt_header(${MEDIA_TARGET_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/media/media_controller.gatt
)

target_include_directories(${MEDIA_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/config
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/ble
    ${CMAKE_CURRENT_LIST_DIR}/include/media
    ${CMAKE_CURRENT_LIST_DIR}/include/net
    ${CMAKE_CURRENT_LIST_DIR}/lib/qrcodegen
    ${CMAKE_CURRENT_LIST_DIR}/src/pico
)

target_compile_definitions(${MEDIA_TARGET_NAME} PRIVATE 
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
    CYW43_LWIP=1
)

target_link_options(${MEDIA_TARGET_NAME} PRIVATE "-Wl,--defsym=__STACK_SIZE=16384")
pico_enable_stdio_uart(${MEDIA_TARGET_NAME} 1)
pico_add_extra_outputs(${MEDIA_TARGET_NAME})

# SD Image
add_custom_target(pi_image
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_pi_image.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Raspberry Pi Zero Image with Buildroot..."
    USES_TERMINAL
)
