# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/main.cpp)

idf_component_register(
    SRCS ${app_sources}
    REQUIRES espressif__esp_websocket_client espressif__mdns joltwallet__littlefs
)
