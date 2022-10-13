"""
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_nlohmann():
    maybe(
        http_archive,
        name = "nlohmann",
        url = "https://github.com/nlohmann/json/archive/refs/tags/v3.10.5.tar.gz",
        strip_prefix = "json-3.10.5",
        build_file = "//third_party/nlohmann:nlohmann.BUILD",
    )
