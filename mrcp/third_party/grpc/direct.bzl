"""
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_grpc():
   maybe(
        http_archive,
        name = "com_github_grpc_grpc",
        url = "https://github.com/grpc/grpc/archive/b0f37a22bbae12a4b225a88be6d18d5e41dce881.tar.gz",
        strip_prefix = "grpc-b0f37a22bbae12a4b225a88be6d18d5e41dce881",
    )
