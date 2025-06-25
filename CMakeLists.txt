# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008" .
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles" .
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name and language
project(WinHier VERSION 2.6 LANGUAGES C CXX RC)

# check build type
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# make uppercase string of build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

# check build type again
if (NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

# set output directory (build/)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

if (WIN32)
    # enable Win32 resource
    enable_language(RC)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # using Clang
        set(CMAKE_C_FLAGS "-static -s")
        set(CMAKE_CXX_FLAGS "-static -s")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # using GCC
        set(CMAKE_C_FLAGS "-static -static-libgcc -static-libstdc++ -s -pedantic -Wall")
        set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++ -s -pedantic -Wall")
    elseif (MSVC)
        # replace "/MD" with "/MT" (building without runtime DLLs)
        set(CompilerFlags
            CMAKE_C_FLAGS
            CMAKE_C_FLAGS_DEBUG
            CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS
            CMAKE_CXX_FLAGS_DEBUG
            CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        foreach(CompilerFlags ${CompilerFlags})
            string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
        endforeach()
        add_definitions(-DMSVC)
    endif()
endif()

##############################################################################

include_directories(. include)

# Unicode
add_definitions(-DUNICODE -D_UNICODE)

# WinHier.exe
add_executable(WinHier WIN32 WinHier/WinHier.cpp WinHier/WinHier_res.rc)
target_link_libraries(WinHier comctl32 psapi shlwapi advapi32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(WinHier PROPERTIES OUTPUT_NAME "WinHier64")
else()
    set_target_properties(WinHier PROPERTIES OUTPUT_NAME "WinHier32")
endif()

# MsgGet32.dll/MsgGet64.dll
add_library(MsgGetDLL SHARED MsgGetDLL/MsgGetDLL.c MsgGetDLL/MsgGetDLL.def)
set_target_properties(MsgGetDLL PROPERTIES PREFIX "")
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(MsgGetDLL PROPERTIES OUTPUT_NAME "MsgGet64")
else()
    set_target_properties(MsgGetDLL PROPERTIES OUTPUT_NAME "MsgGet32")
endif()
if (MSVC)
    target_link_options(MsgGetDLL PRIVATE /DYNAMICBASE:NO /FIXED)
endif()

# MsgGetter32.exe/MsgGetter64.exe
add_executable(MsgGetter WIN32 MsgGetter/MsgGetter.cpp MsgGetter/MsgGetter_res.rc)
target_link_libraries(MsgGetter shlwapi)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(MsgGetter PROPERTIES OUTPUT_NAME "MsgGetter64")
else()
    set_target_properties(MsgGetter PROPERTIES OUTPUT_NAME "MsgGetter32")
endif()

# copy binary files
add_custom_target(copy_binaries ALL
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:WinHier>" "${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:MsgGetDLL>" "${CMAKE_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:MsgGetter>" "${CMAKE_SOURCE_DIR}"
    DEPENDS WinHier MsgGetDLL MsgGetter)

##############################################################################
