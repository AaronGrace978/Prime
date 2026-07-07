cmake_minimum_required(VERSION 3.16)

project(Prime
  VERSION 0.1.0
  DESCRIPTION "Resurrection of the Farbrausch Werkkzeug procedural content pipeline"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(PRIME_BUILD_TOOLS "Build the Prime command line tools" ON)
option(PRIME_BUILD_TESTS "Build the Prime test suite" ON)

add_subdirectory(engine/texture)
add_subdirectory(engine/synth)

if(PRIME_BUILD_TOOLS)
  add_subdirectory(tools/primetex)
  add_subdirectory(tools/primeforge)
  add_subdirectory(tools/primeplay)
endif()

if(PRIME_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
