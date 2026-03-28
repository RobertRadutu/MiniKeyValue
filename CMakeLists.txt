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

# Unit tests — InMemoryStorage / FailingStorage, no filesystem IO
add_executable(unit_tests
    ${SOURCES}
    tests/MiniKeyValueUnitTests.cpp
)
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(unit_tests PRIVATE GTest::gtest_main)

# Integration tests — real FileStorage on disk
add_executable(integration_tests
    ${SOURCES}
    tests/MiniKeyValueIntegrationTests.cpp
)
target_include_directories(integration_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(integration_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)
