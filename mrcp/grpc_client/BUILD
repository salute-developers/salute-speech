package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "recognition_proto",
    srcs = ["recognition.proto"],
    deps = ["@com_google_protobuf//:duration_proto"]
)

cc_proto_library(
    name = "recognition_cc_proto",
    deps = [":recognition_proto"],
)

cc_grpc_library(
    name = "recognition_cc_grpc",
    srcs = [":recognition_proto"],
    grpc_only = True,
    deps = [":recognition_cc_proto"],
)

proto_library(
    name = "synthesis_proto",
    srcs = ["synthesis.proto"],
    deps = ["@com_google_protobuf//:duration_proto"]
)

cc_proto_library(
    name = "synthesis_cc_proto",
    deps = [":synthesis_proto"],
)

cc_grpc_library(
    name = "synthesis_cc_grpc",
    srcs = [":synthesis_proto"],
    grpc_only = True,
    deps = [":synthesis_cc_proto"],
)

cc_library(
    name = "grpc_client",
    srcs = ["client.cpp", "token_resolver.cpp"],
    hdrs = ["client.hpp", "token_resolver.hpp"],
    deps = [
        ":recognition_cc_grpc",
        ":synthesis_cc_grpc",
        "@com_github_grpc_grpc//:grpc++",
        "@nlohmann",
        "@curl",
    ],
)
