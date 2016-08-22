cmake_minimum_required (VERSION 2.6)
project (ORDERED_TRIE)

if (NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release)
endif ()

message (STATUS "Configured for build-type: ${CMAKE_BUILD_TYPE}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
  set (CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} --std=c++14 -O3 -Wall")
  message (STATUS "compile with ${CMAKE_CXX_FLAGS_RELEASE}")
else ()
  set (CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} --std=c++14 -O0 -g -Wall")
  message (STATUS "compile with ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory (tests)
