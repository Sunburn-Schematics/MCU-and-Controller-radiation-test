cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME HC_FW_BlackPill)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Source files are listed explicitly instead of using file(GLOB ... CONFIGURE_DEPENDS).
# Keep these lists in sync when adding, removing, or renaming project-owned source
# files; this avoids CMake/Ninja VerifyGlobs regeneration hangs in this workspace.
set(DRIVERS_LOCAL_SOURCES
    Drivers_Local/adc_sense_drv.c
    Drivers_Local/pwm_capture_drv.c
    Drivers_Local/reset_reason_drv.c
    Drivers_Local/rtc_drv.c
    Drivers_Local/sync_drv.c
    Drivers_Local/usb_vcp_drv.c
)

set(COMMAND_HANDLER_SOURCES
    Services/CommandHandler/command_processor.c
    Services/CommandHandler/hc_cmd_types.c
    Services/CommandHandler/hc_comms_tx.c
    Services/CommandHandler/hc_datetime.c
    Services/CommandHandler/hc_jsonl_cmd.c
    Services/CommandHandler/hc_jsonl_dispatch.c
    Services/CommandHandler/hc_jsonl_fields.c
    Services/CommandHandler/hc_jsonl_parse.c
    Services/CommandHandler/hc_jsonl_rsp.c
)

set(JSMN_SOURCES
    Services/jsmn/jsmn.c
)

set(RING_BUFFER_SOURCES
    Services/RingBuffer/ring_buffer.c
)


# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    App/fw_app.c
    App/hc_debug_telemetry.c
    App/hc_app_status.c
    App/ltc3901_manager.c
    App/lt8316_manager.c
    Bsp/bsp_board.c
#    Drivers_Local/sync_drv.c
#    Drivers_Local/usb_vcp_drv.c
    ${DRIVERS_LOCAL_SOURCES}
    ${COMMAND_HANDLER_SOURCES}
    ${JSMN_SOURCES}
    ${RING_BUFFER_SOURCES}
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    App
    Config
    Bsp
    Drivers_Local
    Services
    Services/CommandHandler
    Services/jsmn
    Services/RingBuffer
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    JSMN_STRICT
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
