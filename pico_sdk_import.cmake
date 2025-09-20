# pico_sdk_import.cmake

# This file is required to be in your project root to allow cmake to find the PICO_SDK
# It can be found in the SDK at external/pico_sdk_import.cmake


set(PICO_SDK_PATH "C:\\Users\\arunj\\.pico-sdk\\sdk\\2.2.0" CACHE PATH "Path to the Pico SDK")
set(PICO_PLATFORM rp2040 CACHE STRING "Target Pico platform")
set(PICO_BOARD pico_w CACHE STRING "Target Pico board")
set(PICO_TOOLCHAIN_PATH "C:\\Users\\arunj\\.pico-sdk\\toolchain\\14_2_Rel1" CACHE PATH "Path to the Pico toolchain")
set(PICO_TOOLS_DIR "C:\\Users\\arunj\\.pico-sdk\\tools" CACHE PATH "Path to the Pico tools")

if (NOT EXISTS ${PICO_SDK_PATH}/pico_sdk_init.cmake)
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' does not appear to be a Pico SDK")
endif()

include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
