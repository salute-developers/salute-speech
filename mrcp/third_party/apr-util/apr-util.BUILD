load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "apr-util",
    configure_in_place = True,
    configure_options = [
        "--with-apr=$EXT_BUILD_DEPS/apr",
        "--with-expat=$EXT_BUILD_DEPS/expat",
    ],
    env = select({
        "@platforms//os:macos": {"AR": ""},
        "//conditions:default": {},
    }),
    copts = ["-fPIC"],
    lib_source = ":all_srcs",
    out_static_libs = ["libaprutil-1.a"],
    visibility = ["//visibility:public"],
    deps = [
        "@apr",
        "@expat",
    ],
)
