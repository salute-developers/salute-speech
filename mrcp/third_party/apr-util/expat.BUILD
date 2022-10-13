load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

LIB_NAME = "expat"

configure_make(
    name = "expat",
    configure_options = [
        "--disable-shared",
        "--without-docbook",
        "--without-examples",
        "--without-tests",
    ],
    env = select({
        "@platforms//os:macos": {"AR": ""},
        "//conditions:default": {},
    }),
    copts = ["-fPIC"],
    lib_name = LIB_NAME,
    lib_source = ":all_srcs",
    out_static_libs = ["libexpat.a"],
    visibility = ["//visibility:public"],
)
