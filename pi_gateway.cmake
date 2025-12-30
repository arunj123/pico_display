cmake_minimum_required(VERSION 3.13)

add_custom_target(pi_image
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_pi_image.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building Raspberry Pi Zero Image with Buildroot..."
    USES_TERMINAL
)

