"""
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_unimrcp():
    maybe(
        http_archive,
        name = "unimrcp",
        url = "https://github.com/unispeech/unimrcp/archive/refs/tags/unimrcp-1.7.0.tar.gz",
        strip_prefix = "unimrcp-unimrcp-1.7.0",
        build_file = "//third_party/unimrcp:unimrcp.BUILD",
    )
