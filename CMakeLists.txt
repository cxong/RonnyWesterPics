cmake_minimum_required(VERSION 2.18.12)
project(ronnypix C)

add_executable(ronnypix bmpfile.c bmpfile.h main.c pic_file.c pic_file.h)
