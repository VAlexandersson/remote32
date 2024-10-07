# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(IR_controller)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/certs/cert.pem
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/certs/cert.pem ${CMAKE_BINARY_DIR}/certs/cert.pem
    DEPENDS ${CMAKE_SOURCE_DIR}/certs/cert.pem
)

add_custom_target(copy_certs ALL DEPENDS ${CMAKE_BINARY_DIR}/certs/cert.pem)