#Copyright (c) Microsoft. All rights reserved.
#Licensed under the MIT license. See LICENSE file in the project root for full license information.

cmake_minimum_required(VERSION 3.2)

compileAsC11()

set(theseTestsName http_headers_ut)
include_directories(${PROJECT_SOURCE_DIR}/inc)

set(${theseTestsName}_test_files
    ${theseTestsName}.c
)

set(${theseTestsName}_c_files
    ../../src/http_headers.c
)

set(${theseTestsName}_h_files
)

build_test_project(${theseTestsName} "tests/lib_utils_tests")
