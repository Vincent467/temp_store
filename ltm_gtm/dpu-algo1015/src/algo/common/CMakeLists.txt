# Minimum required version of CMake
cmake_minimum_required(VERSION 2.8)

# Set the project name
set(PROJ_NAME common)
project(${PROJ_NAME})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# Gather all source files recursively from the src directory
file(GLOB_RECURSE COMMON_SOURCE_FILES ${PROJECT_SOURCE_DIR}/src/*.c)

# Gather all header files recursively from the include directory
file(GLOB_RECURSE COMMON_HEADER_FILES ${PROJECT_SOURCE_DIR}/inc/*.h)

# Create source groups for IDEs
create_source_groups(${CMAKE_CURRENT_SOURCE_DIR} "${COMMON_SOURCE_FILES};${COMMON_HEADER_FILES}")

# Add a static library target with the gathered source and header files
add_library(common_objects OBJECT ${COMMON_SOURCE_FILES} ${COMMON_HEADER_FILES})
set_target_properties(common_objects PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(common_objects PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
#target_include_directories(${PROJ_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

# copy include files to build folder
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/HW/inc")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/SW/inc")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/inc/algo_log.h" DESTINATION "${CMAKE_BINARY_DIR}/HW/inc")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/inc/algo_log.h" DESTINATION "${CMAKE_BINARY_DIR}/SW/inc")