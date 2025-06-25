# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name and language
project(WinHier VERSION 2.6 LANGUAGES C CXX RC)

# Display CMAKE_BUILD_TYPE
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

# Don't use extra DLLs
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # using Clang
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # using GCC
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif(MSVC)
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
endif()

# Unicode
add_definitions(-DUNICODE -D_UNICODE)

##############################################################################

include_directories(. include)

# WinHier32.exe / WinHier64.exe
add_executable(WinHier WIN32 WinHier/WinHier.cpp WinHier/WinHier_res.rc)
target_link_libraries(WinHier comctl32 psapi shlwapi advapi32)
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(WinHier PROPERTIES OUTPUT_NAME "WinHier64")
else()
    set_target_properties(WinHier PROPERTIES OUTPUT_NAME "WinHier32")
endif()

# MsgGet32.dll / MsgGet64.dll
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

# MsgGetter32.exe / MsgGetter64.exe
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
