cmake_minimum_required(VERSION 3.20)
project(InfiniteHair LANGUAGES CXX)

# Path to the extracted ScriptHookRDR2 SDK (contains inc/ and lib/).
set(SHRDR2_SDK_DIR "${CMAKE_SOURCE_DIR}/sdk" CACHE PATH "ScriptHookRDR2 SDK directory")

if(NOT EXISTS "${SHRDR2_SDK_DIR}/inc/main.h")
    message(FATAL_ERROR "ScriptHookRDR2 SDK not found in ${SHRDR2_SDK_DIR}. "
                        "Extract the SDK so that ${SHRDR2_SDK_DIR}/inc/main.h exists, "
                        "or pass -DSHRDR2_SDK_DIR=<path>.")
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "RDR2 is 64-bit only - configure with -A x64.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(InfiniteHair SHARED
    src/main.cpp
    src/script.cpp
    src/config.cpp
    src/groom.cpp
    src/log.cpp
)

target_include_directories(InfiniteHair PRIVATE "${SHRDR2_SDK_DIR}/inc" src)
target_link_libraries(InfiniteHair PRIVATE "${SHRDR2_SDK_DIR}/lib/ScriptHookRDR2.lib")
target_compile_definitions(InfiniteHair PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(InfiniteHair PRIVATE /W4 /permissive-)
    # Static CRT so players don't need a matching VC++ redistributable.
    set_property(TARGET InfiniteHair PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ScriptHookRDR2 loads plugins with the .asi extension.
set_target_properties(InfiniteHair PROPERTIES SUFFIX ".asi" PREFIX "")
