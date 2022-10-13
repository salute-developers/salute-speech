load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

configure_make(
    name = "sofia-sip",
    autogen = True,
    configure_in_place = True,
    configure_options = [
      "--with-glib=no",
      "--without-doxygen",
    ],
    env = select({
        "@platforms//os:macos": {"AR": ""},
        "//conditions:default": {},
    }),
    copts = ["-fPIC"],
    lib_source = ":all_srcs",
    out_static_libs = ["libsofia-sip-ua.a"],
    visibility = ["//visibility:public"],
)
