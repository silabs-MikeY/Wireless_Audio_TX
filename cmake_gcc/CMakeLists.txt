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

add_executable(TX_Board
    # Add additional sources here
)

target_include_directories(TX_Board PUBLIC
    # Add additional include paths here
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

# Force the gcc linker command
set_target_properties(TX_Board PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET TX_Board
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:TX_Board>" "$<TARGET_FILE_DIR:TX_Board>/$<TARGET_FILE_BASE_NAME:TX_Board>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:TX_Board>" "$<TARGET_FILE_DIR:TX_Board>/$<TARGET_FILE_BASE_NAME:TX_Board>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:TX_Board>" "$<TARGET_FILE_DIR:TX_Board>/$<TARGET_FILE_BASE_NAME:TX_Board>.bin" 
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET TX_Board
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()