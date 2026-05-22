cmake_minimum_required(VERSION 4.2)
project(NGOS C)

set(CMAKE_C_STANDARD 11)

add_executable(NGOS
        src/include/io.h
        src/include/serial.h
        src/serial.c
        src/kernel.c
        src/include/limine.h)
