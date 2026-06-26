cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
project(fontnik LANGUAGES CXX C)
cmake_policy(VERSION 3.18)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

# Header-only deps: SOURCE_SUBDIR points at a non-existent path so FetchContent
# downloads and extracts the tarball but skips add_subdirectory (which would
# try to build targets we only need as include trees).
FetchContent_Declare(
    node_addon_api
    URL https://github.com/nodejs/node-addon-api/archive/refs/tags/v8.8.0.tar.gz
    URL_HASH SHA256=5c87d586de7204fbe7ebc87e6ceb2d49fc9a6ead02803a263ebea8ff9b7d2060
    SOURCE_SUBDIR "__fontnik_header_only__"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(
    node_api_headers
    URL https://github.com/nodejs/node-api-headers/archive/refs/tags/v1.9.0.tar.gz
    URL_HASH SHA256=071f773bd10479f865f6828b86fe8232d63e255290d9bbb7a0ec487c57310231
    SOURCE_SUBDIR "__fontnik_header_only__"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_Declare(
    protozero
    URL https://github.com/mapbox/protozero/archive/refs/tags/v1.8.1.tar.gz
    URL_HASH SHA256=6c7a896f1dc08435e8cd4f3780ff688cd0bfce6890599b755f6f3cb36398dc25
    SOURCE_SUBDIR "__fontnik_header_only__"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(node_addon_api node_api_headers protozero)

find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost CONFIG REQUIRED)

add_library(
    fontnik SHARED
    src/glyphs.cpp
    src/node_fontnik.cpp
)

target_include_directories(
    fontnik
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(
    fontnik
    SYSTEM PRIVATE
        ${node_addon_api_SOURCE_DIR}
        ${node_api_headers_SOURCE_DIR}/include
        ${protozero_SOURCE_DIR}/include
)

target_link_libraries(
    fontnik
    PRIVATE
        Freetype::Freetype
        ZLIB::ZLIB
        Boost::headers
)

target_compile_options(
    fontnik
    PRIVATE
        -Wall
        -Wextra
        -Wconversion
        -Wshadow
        -Wfloat-equal
        -Wuninitialized
        -Wunreachable-code
        -Wold-style-cast
        -Wno-deprecated-declarations
        -Wno-c++11-narrowing
)

# npm prebuild layout: prebuilds/<platform>-<arch>/fontnik.node
if(APPLE)
    set(FONTNIK_PLATFORM "darwin")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(FONTNIK_PLATFORM "linux")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _fontnik_proc)
if(_fontnik_proc STREQUAL "x86_64" OR _fontnik_proc STREQUAL "amd64")
    set(FONTNIK_ARCH "x64")
elseif(_fontnik_proc STREQUAL "arm64" OR _fontnik_proc STREQUAL "aarch64")
    set(FONTNIK_ARCH "arm64")
else()
    message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(FONTNIK_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/prebuilds/${FONTNIK_PLATFORM}-${FONTNIK_ARCH}")
    file(MAKE_DIRECTORY "${FONTNIK_OUTPUT_DIR}")
else()
    set(FONTNIK_OUTPUT_DIR "${CMAKE_BINARY_DIR}")
endif()

set_target_properties(
    fontnik
    PROPERTIES
        PREFIX ""
        SUFFIX ".node"
        LIBRARY_OUTPUT_DIRECTORY "${FONTNIK_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${FONTNIK_OUTPUT_DIR}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND FONTNIK_PLATFORM STREQUAL "linux")
    add_custom_command(
        TARGET fontnik
        POST_BUILD
        COMMAND ${CMAKE_STRIP} "$<TARGET_FILE:fontnik>"
        COMMENT "Stripping fontnik.node"
    )
endif()

target_compile_definitions(
    fontnik
    PRIVATE
        NAPI_VERSION=9
        NODE_ADDON_API_DISABLE_DEPRECATED
)

if(APPLE)
    set_target_properties(
        fontnik
        PROPERTIES LINK_FLAGS "-undefined dynamic_lookup -bind_at_load"
    )
else()
    set_target_properties(
        fontnik
        PROPERTIES LINK_FLAGS "-z now"
    )
endif()
