"""
Dependency to cURL
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_curl():
    maybe(
        http_archive,
        name = "curl",
        build_file = "//third_party/curl:curl.BUILD",
        sha256 = "e56b3921eeb7a2951959c02db0912b5fcd5fdba5aca071da819e1accf338bbd7",
        strip_prefix = "curl-7.74.0",
        url = "https://github.com/curl/curl/releases/download/curl-7_74_0/curl-7.74.0.tar.gz",
    )

