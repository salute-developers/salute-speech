"""
Dependency to zlib
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_zlib():
    maybe(
        http_archive,
        name = "zlib",
        build_file = "//third_party/zlib:zlib.BUILD",
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        url = "https://zlib.net/zlib-1.2.11.tar.gz",
    )

