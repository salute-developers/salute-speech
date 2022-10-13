
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_bazel_compile_commands():
  maybe(
    http_archive,
    name = "bazel_compile_commands",
    sha256 = "810b9c00cb278fe8e2f8331cd70a4f87605ce825e10166fd2d36ee171e7b8521",
    strip_prefix = "bazel_compile_commands-0.1.2",
    urls = [
      "https://github.com/cdump/bazel_compile_commands/archive/0.1.2.zip",
    ],
    repo_mapping = {
      "@com_github_tencent_rapidjson": "@rapidjson",
    },
  )

  maybe(
    http_archive,
    name = "rapidjson",
    build_file = "@//third_party/bazel-compile-commands:rapidjson.BUILD",
    sha256 = "926cb949eb0fdb6154c1104985aee0bf0a6eac38f0b90da05eaf5f079f320672",
    strip_prefix = "rapidjson-ce81bc9edfe773667a7a4454ba81dac72ed4364c",
    urls = [
      "https://github.com/Tencent/rapidjson/archive/ce81bc9edfe773667a7a4454ba81dac72ed4364c.tar.gz",
    ],
  )
