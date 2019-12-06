#!/bin/bash

g++ -o vastspacewar main.cpp `sdl2-config --cflags` `sdl2-config --libs` -lGL
