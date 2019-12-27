cmake_minimum_required(VERSION 3.3.0)

set(http_client_sample_files
    http_client_sample.c
)

add_executable(http_client_sample ${http_client_sample_files})

target_link_libraries(http_client_sample patchcords lib-util-c http_client)

if (UNIX)
    target_link_libraries(http_client_sample cord_berkley)
endif (UNIX)