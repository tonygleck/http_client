#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.3.0)

#Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(http_client)

option(http_client_ut "Include unittest in build" OFF)
option(http_client_samples "Include samples in build" OFF)

include("${PROJECT_SOURCE_DIR}/cmake_configs/proj_config.cmake")

if ((NOT TARGET azure_macro_utils_c) AND (EXISTS ${PROJECT_SOURCE_DIR}/deps/azure-macro-utils-c/CMakeLists.txt))
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-macro-utils-c)
    include_directories(${MACRO_UTILS_INC_FOLDER})
endif()
if (NOT TARGET umock_c)
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/umock-c)
    include_directories(${PROJECT_SOURCE_DIR}/deps/umock-c/inc)
endif()
if (NOT TARGET lib-util-c)
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/lib-util-c)
    include_directories(${PROJECT_SOURCE_DIR}/deps/lib-util-c/inc)
endif()
if (NOT TARGET patchcords)
    add_subdirectory(${PROJECT_SOURCE_DIR}/deps/libpatchcords)
    include_directories(${PROJECT_SOURCE_DIR}/deps/libpatchcords/inc)
endif()

#these are the C source files
set(source_c_files
    ${PROJECT_SOURCE_DIR}/src/http_client.c
    ${PROJECT_SOURCE_DIR}/src/http_codec.c
    ${PROJECT_SOURCE_DIR}/src/http_headers.c
)

#these are the C headers
set(source_h_files
    ${PROJECT_SOURCE_DIR}/inc/http_client/http_client.h
    ${PROJECT_SOURCE_DIR}/inc/http_client/http_codec.h
    ${PROJECT_SOURCE_DIR}/inc/http_client/http_headers.h
)

#this is the product (a library)
add_library(http_client ${source_c_files} ${source_h_files})
target_include_directories(http_client PUBLIC ${PROJECT_SOURCE_DIR}/inc)

#addCompileSettings(http_client)
compileTargetAsC99(http_client)

if (${http_client_ut})
    enable_testing()
    include (CTest)

    include("${CMAKE_CURRENT_LIST_DIR}/cmake_configs/proj_testing.cmake")

    if ((NOT TARGET testrunnerswitcher) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-c-testrunnerswitcher/CMakeLists.txt))
        add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-c-testrunnerswitcher)
        include_directories(${TESTRUNNERSWITCHER_INC_FOLDER})
    endif()
    if ((NOT TARGET ctest) AND (EXISTS ${CMAKE_CURRENT_LIST_DIR}/deps/azure-ctest/CMakeLists.txt))
        add_subdirectory(${PROJECT_SOURCE_DIR}/deps/azure-ctest)
        include_directories(${CTEST_INC_FOLDER})
    endif()

    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
endif()

if (${http_client_samples})
    add_subdirectory(samples)
endif()
