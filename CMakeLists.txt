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

set(CORE_SOURCES
    src/MiniKeyValue.cpp
    src/FileStorage.cpp
)

add_library(minikvalue_core STATIC ${CORE_SOURCES})
target_include_directories(minikvalue_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(unit_tests tests/MiniKeyValueUnitTests.cpp)
target_link_libraries(unit_tests PRIVATE minikvalue_core GTest::gtest_main)

add_executable(integration_tests tests/MiniKeyValueIntegrationTests.cpp)
target_link_libraries(integration_tests PRIVATE minikvalue_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(integration_tests)

option(MINIKVALUE_WITH_GRPC "Build gRPC shard server, distributed client, and gRPC tests" ON)

if(MINIKVALUE_WITH_GRPC)
    if(DEFINED ENV{CONDA_PREFIX})
        list(APPEND CMAKE_PREFIX_PATH "$ENV{CONDA_PREFIX}")
    endif()

    find_package(Protobuf CONFIG REQUIRED)
    find_package(gRPC CONFIG REQUIRED)

    set(KV_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/proto/keyvalue.proto")
    set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY "${PROTO_GEN_DIR}")

    set(KV_PROTO_SRCS "${PROTO_GEN_DIR}/keyvalue.pb.cc" "${PROTO_GEN_DIR}/keyvalue.grpc.pb.cc")
    set(KV_PROTO_HDRS "${PROTO_GEN_DIR}/keyvalue.pb.h" "${PROTO_GEN_DIR}/keyvalue.grpc.pb.h")

    add_custom_command(
        OUTPUT ${KV_PROTO_SRCS} ${KV_PROTO_HDRS}
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS --cpp_out=${PROTO_GEN_DIR}
            --grpc_out=${PROTO_GEN_DIR}
            -I ${CMAKE_CURRENT_SOURCE_DIR}/proto
            --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
            ${KV_PROTO}
        DEPENDS ${KV_PROTO}
        COMMENT "Generating protobuf and gRPC sources from keyvalue.proto"
        VERBATIM
    )

    add_library(minikvalue_proto STATIC ${KV_PROTO_SRCS})
    target_include_directories(minikvalue_proto PUBLIC "${PROTO_GEN_DIR}")
    target_link_libraries(minikvalue_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)

    add_library(minikvalue_grpc STATIC
        src/GrpcKeyValueServiceImpl.cpp
        src/DistributedMiniKeyValue.cpp
    )
    target_include_directories(minikvalue_grpc PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        "${PROTO_GEN_DIR}"
    )
    target_link_libraries(minikvalue_grpc PUBLIC minikvalue_core minikvalue_proto)

    add_executable(minikvalue-grpc-server src/minikvalue_grpc_server_main.cpp)
    target_link_libraries(minikvalue-grpc-server PRIVATE minikvalue_grpc)

    add_executable(grpc_sharded_tests tests/GrpcShardedTests.cpp)
    target_link_libraries(grpc_sharded_tests PRIVATE minikvalue_grpc GTest::gtest_main)
    gtest_discover_tests(grpc_sharded_tests DISCOVERY_MODE PRE_TEST)
endif()
