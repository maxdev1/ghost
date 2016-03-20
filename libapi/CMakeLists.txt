cmake_minimum_required(VERSION 2.8.4)

project(ghostapi)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(ghostapi STATIC ${SOURCES})
set_target_properties(ghostapi PROPERTIES COMPILE_FLAGS "-std=c++1y")
target_include_directories(ghostapi PUBLIC "inc")
