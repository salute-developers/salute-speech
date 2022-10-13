load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all_srcs",
    srcs = glob(
        include = ["**"],
    ),
)

cmake(
    name = "zlib",
    copts = ["-fPIC"],
    lib_source = ":all_srcs",
    out_static_libs = ["libz.a"],
)
