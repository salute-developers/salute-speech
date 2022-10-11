cmake_minimum_required(VERSION 3.17)
project(smartspeech_cpp_synth_sample C CXX)
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
set(synthesis_proto "${CMAKE_CURRENT_SOURCE_DIR}/../synthesis.proto")
get_filename_component(synthesis_proto "${synthesis_proto}" ABSOLUTE)
get_filename_component(synthesis_proto_path "${synthesis_proto}" PATH)

# Generated sources
set(synthesis_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/synthesis.pb.cc")
set(synthesis_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/synthesis.pb.h")
set(synthesis_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/synthesis.grpc.pb.cc")
set(synthesis_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/synthesis.grpc.pb.h")

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

add_custom_command(
        OUTPUT "${synthesis_proto_srcs}" "${synthesis_proto_hdrs}" "${synthesis_grpc_srcs}" "${synthesis_grpc_hdrs}"
        COMMAND protobuf::protoc
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${synthesis_proto_path}"
        --plugin=protoc-gen-grpc="${grpc_cpp_plugin_location}"
        "${synthesis_proto}"
        DEPENDS "${synthesis_proto}" protobuf::protoc)

add_library(smartspeech-proto
        ${synthesis_proto_srcs}
        ${synthesis_proto_hdrs}
        ${synthesis_grpc_srcs}
        ${synthesis_grpc_hdrs})
set_target_properties(smartspeech-proto PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(smartspeech-proto
        PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++)

target_include_directories(smartspeech-proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_executable(synthesis
        synthesis.cpp
        config.cpp
        grpc/client.cpp)

target_include_directories(synthesis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(synthesis PUBLIC smartspeech-proto)

