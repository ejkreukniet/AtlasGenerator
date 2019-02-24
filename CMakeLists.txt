cmake_minimum_required(VERSION 3.13)
project(AtlasGenerator)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(AtlasGenerator
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ui_mainwindow.h)
