"""
Dependency to protobuf, Google's data interchange format
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_protobuf():
    maybe(
        http_archive,
        name = "rules_proto",
        url = "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
        strip_prefix = "rules_proto-4.0.0",
    )
