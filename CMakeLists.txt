cmake_minimum_required(VERSION 3.10)
project(resit_adc C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)


add_compile_options(-Wall -Wextra -pedantic)

add_executable(resit_adc
    main.c
    adc.c
    io.c
    stats.c
)

target_link_libraries(resit_adc m)
