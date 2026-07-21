# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	TX_Board
	VERSION 1.0
	LANGUAGES C CXX ASM
)

# Include the definition of the slc_TX_Board target,
# which contains the content of the SLC project
include(TX_Board.cmake)

target_include_directories(slc PUBLIC
    "../libraries"
    "../radio"
)

add_executable(TX_Board
    # Add additional sources here
)

target_include_directories(TX_Board PUBLIC
    # Add additional include paths here
    "../libraries"
)

target_compile_definitions(TX_Board PUBLIC
    # Add additional macros here
)

target_compile_options(TX_Board PUBLIC
    # Set additional compiler flags here
)

target_link_options(TX_Board PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(TX_Board PRIVATE
    slc
)

# Include managed project content if available
include(TX_Board_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Simplicity Studio regenerates TX_Board_project.cmake when build settings
# change, so keep hand-maintained application libraries here instead.
set(TX_BOARD_LIBRARY_SOURCES
    "../libraries/ADC.c"
    "../libraries/RGB.c"
    "../libraries/VDAC.c"
    "../libraries/adpcm.c"
    "../libraries/button.c"
    "../libraries/console_tx.c"
    "../libraries/counters.c"
    "../libraries/events_print.c"
    "../libraries/generic.c"
    "../libraries/ldma_manager.c"
    "../libraries/microseconds.c"
    "../libraries/print.c"
    "../libraries/scheduler.c"
    "../libraries/wdog.c"
)

get_target_property(TX_BOARD_EXISTING_SOURCES TX_Board SOURCES)
if(NOT TX_BOARD_EXISTING_SOURCES)
    set(TX_BOARD_EXISTING_SOURCES)
endif()

foreach(source_file IN LISTS TX_BOARD_LIBRARY_SOURCES)
    list(FIND TX_BOARD_EXISTING_SOURCES "${source_file}" source_index)
    if(source_index EQUAL -1)
        target_sources(TX_Board PRIVATE "${source_file}")
        list(APPEND TX_BOARD_EXISTING_SOURCES "${source_file}")
    endif()
endforeach()

list(FIND TX_BOARD_EXISTING_SOURCES "../timer_helper.c" root_timer_helper_index)
list(FIND TX_BOARD_EXISTING_SOURCES "../libraries/timer_helper.c" library_timer_helper_index)
if(root_timer_helper_index GREATER -1)
    list(REMOVE_ITEM TX_BOARD_EXISTING_SOURCES "../timer_helper.c")
    get_target_property(TX_BOARD_COMPILE_SOURCES TX_Board SOURCES)
    list(REMOVE_ITEM TX_BOARD_COMPILE_SOURCES "../timer_helper.c")
    set_target_properties(TX_Board PROPERTIES SOURCES "${TX_BOARD_COMPILE_SOURCES}")
endif()

if(library_timer_helper_index EQUAL -1)
    target_sources(TX_Board PRIVATE "../libraries/timer_helper.c")
endif()

# Force the gcc linker command
set_target_properties(TX_Board PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET TX_Board
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:TX_Board>" "$<TARGET_FILE_DIR:TX_Board>/$<TARGET_FILE_BASE_NAME:TX_Board>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:TX_Board>" "$<TARGET_FILE_DIR:TX_Board>/$<TARGET_FILE_BASE_NAME:TX_Board>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:TX_Board>" "$<TARGET_FILE_DIR:TX_Board>/$<TARGET_FILE_BASE_NAME:TX_Board>.bin" 
)

# Flashing after build is opt-in so normal builds stay standalone.
option(TX_BOARD_FLASH_AFTER_BUILD "Flash TX_Board automatically after build" OFF)

# Run post-build pipeline to perform additional post-processing
if(TX_BOARD_FLASH_AFTER_BUILD AND post_build_command)
add_custom_command(TARGET TX_Board
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()
