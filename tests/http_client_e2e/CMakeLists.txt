#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.2)

compileAsC11()

set(theseTestsName http_client_e2e)
include_directories(${PROJECT_SOURCE_DIR}/inc)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
)

set(${theseTestsName}_h_files
)

build_test_project(${theseTestsName} "tests/lib_utils_tests")

target_link_libraries(${theseTestsName}_exe patchcords http_client lib-util-c)
if (UNIX)
    target_link_libraries(${theseTestsName}_exe cord_berkley)
elseif(WIN32)
    target_link_libraries(${theseTestsName}_exe cord_winsock)
endif (UNIX)
