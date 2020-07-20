cmake_minimum_required(VERSION 2.8)
project(DYRS) # Dynamic Rank/Select

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

MESSAGE(STATUS "CMAKE_BUILD_TYPE: " ${CMAKE_BUILD_TYPE})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (DISABLE_AVX)
  MESSAGE(STATUS "SIMD AVX disabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_AVX")
endif()

if (AVX512)
  MESSAGE(STATUS "SIMD AVX512 enabled")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2 -mavx512f -mavx512bw -mavx512vl -mavx512vpopcntdq -DAVX512")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif ()

if (UNIX)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-missing-braces")

  if (USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
  endif()

endif()

MESSAGE(STATUS "Compiling with ${CMAKE_CXX_FLAGS}")

# For succinct
find_package(Boost 1.42.0 COMPONENTS unit_test_framework iostreams system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

# SDSL
include_directories($ENV{HOME}/include)
link_directories($ENV{HOME}/lib)

include_directories(${DYRS_SOURCE_DIR}/include)

add_subdirectory(external)
add_subdirectory(src)

enable_testing()
file(GLOB TEST_SOURCES test/test_*.cpp)
foreach(TEST_SRC ${TEST_SOURCES})
  get_filename_component (TEST_SRC_NAME ${TEST_SRC} NAME_WE) # without extension
  add_executable(${TEST_SRC_NAME} ${TEST_SRC})
  add_test(${TEST_SRC_NAME} ${TEST_SRC_NAME})
endforeach(TEST_SRC)