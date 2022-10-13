load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

CONFIGURE_OPTIONS = [
    "no-comp",
    "no-idea",
    "no-weak-ssl-ciphers",
    "no-shared",
]

LIB_NAME = "openssl"

MAKE_TARGETS = [
    "build_libs",
    "install_dev",
]

configure_make(
    name = "openssl",
    configure_command = "config",
    configure_options = CONFIGURE_OPTIONS,
    env = 
        select({
            "@platforms//os:macos": {
              "OSX_DEPLOYMENT_TARGET": "10.14",
              "AR": "",
            },
            "//conditions:default": {}},
        ), 
    lib_name = LIB_NAME,
    lib_source = ":all_srcs",
    out_static_libs = [
        "libssl.a",
        "libcrypto.a",
    ],
    visibility = ["//visibility:public"],
    toolchains = ["@rules_perl//:current_toolchain"],
)

alias(
    name = "libssl",
    actual = ":openssl",
)

alias(
    name = "libcrypto",
    actual = ":openssl",
)

filegroup(
    name = "gen_dir",
    srcs = [":openssl"],
    output_group = "gen_dir",
    visibility = ["//visibility:public"],
)
