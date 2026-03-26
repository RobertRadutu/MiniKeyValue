cmake_minimum_required(VERSION 3.14)
project(minikvalue LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(SOURCES
    src/MiniKeyValue.cpp
    src/FileStorage.cpp
)

add_executable(minikeyvalue_tests
    ${SOURCES}
    tests/MiniKeyValueTests.cpp
)

target_include_directories(minikeyvalue_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(minikeyvalue_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(minikeyvalue_tests)
