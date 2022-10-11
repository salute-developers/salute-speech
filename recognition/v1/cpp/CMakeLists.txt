cmake_minimum_required(VERSION 3.17)
project(smartspeech_cpp_recognition_sample C CXX)
set(CMAKE_CXX_STANDARD 17)

if (APPLE)
    set(OPENSSL_ROOT_DIR ${OPENSSL_ROOT_DIR} /usr/local/opt/openssl)
    set(OPENSSL_CRYPTO_LIBRARY ${OPENSSL_ROOT_DIR}/lib/libcrypto.dylib CACHE FILEPATH "" FORCE)
    set(OPENSSL_SSL_LIBRARY ${OPENSSL_ROOT_DIR}/lib/libssl.dylib CACHE FILEPATH "" FORCE)
endif()
find_package(OpenSSL REQUIRED)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# generate proto and grpc
set(recognition_proto "${CMAKE_CURRENT_SOURCE_DIR}/../recognition.proto")
get_filename_component(recognition_proto "${recognition_proto}" ABSOLUTE)
get_filename_component(recognition_proto_path "${recognition_proto}" PATH)

set(task_proto "${CMAKE_CURRENT_SOURCE_DIR}/../../../task/v1/task.proto")
get_filename_component(task_proto "${task_proto}" ABSOLUTE)
get_filename_component(task_proto_path "${task_proto}" PATH)

# Generated sources
set(recognition_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/recognition.pb.cc")
set(recognition_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/recognition.pb.h")
set(recognition_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/recognition.grpc.pb.cc")
set(recognition_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/recognition.grpc.pb.h")

set(task_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/task.pb.cc")
set(task_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/task.pb.h")

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

add_custom_command(
        OUTPUT "${recognition_proto_srcs}" "${recognition_proto_hdrs}" "${recognition_grpc_srcs}" "${recognition_grpc_hdrs}" "${task_proto_srcs}" "${task_proto_hdrs}"
        COMMAND protobuf::protoc
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${recognition_proto_path}"
        -I "${task_proto_path}"
        --plugin=protoc-gen-grpc="${grpc_cpp_plugin_location}"
        "${recognition_proto}" "${task_proto}"
        DEPENDS "${recognition_proto}" "${task_proto}" protobuf::protoc)

add_library(smartspeech-proto
        ${recognition_proto_srcs}
        ${recognition_proto_hdrs}
        ${recognition_grpc_srcs}
        ${recognition_grpc_hdrs}
        ${task_proto_srcs}
        ${task_proto_hdrs})
set_target_properties(smartspeech-proto PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(smartspeech-proto
        PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++)

target_include_directories(smartspeech-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_executable(recognize
        recognize.cpp
        config.cpp
        grpc/client.cpp)

target_include_directories(recognize PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(recognize PUBLIC smartspeech-proto)

