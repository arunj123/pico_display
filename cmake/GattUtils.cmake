# This function correctly generates a C header from a .gatt file,
# allowing for local #import directives. It builds the correct command line
# arguments for the underlying BTstack GATT compiler.
function(custom_btstack_make_gatt_header target scope gatt_file)
    # Parse for the PYTHON_EXECUTABLE argument
    cmake_parse_arguments(ARG "" "PYTHON_EXECUTABLE" "" ${ARGN})
    if (NOT ARG_PYTHON_EXECUTABLE)
        message(FATAL_ERROR "PYTHON_EXECUTABLE not passed to custom_btstack_make_gatt_header")
    endif()

    get_filename_component(GATT_DIR ${gatt_file} DIRECTORY)
    get_filename_component(GATT_NAME ${gatt_file} NAME_WE)
    set(OUTPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/${target}_gatt_header/${GATT_NAME}.h")
    get_filename_component(OUTPUT_DIR ${OUTPUT_HEADER} DIRECTORY)

    # --- THE FIX: Create a list of correctly formatted include arguments ---
    set(GATT_INCLUDE_ARGS)
    # Add the standard BTstack include path
    list(APPEND GATT_INCLUDE_ARGS "-I" "${PICO_SDK_PATH}/lib/btstack/src")
    # Add the local path where the .gatt file lives
    list(APPEND GATT_INCLUDE_ARGS "-I" "${GATT_DIR}")

    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        
        # Now we call the python script with the correctly formatted list of arguments
        COMMAND ${ARG_PYTHON_EXECUTABLE} 
            ${PICO_SDK_PATH}/lib/btstack/tool/compile_gatt.py
            "${gatt_file}"
            "${OUTPUT_HEADER}"
            ${GATT_INCLUDE_ARGS}  # <-- This expands to "-I path1 -I path2"
        
        DEPENDS 
            ${gatt_file}
            ${PICO_SDK_PATH}/lib/btstack/tool/compile_gatt.py
        
        WORKING_DIRECTORY ${GATT_DIR}
        VERBATIM
    )

    target_sources(${target} ${scope} ${OUTPUT_HEADER})
    target_include_directories(${target} ${scope} ${CMAKE_CURRENT_BINARY_DIR}/generated/${target}_gatt_header)
endfunction()