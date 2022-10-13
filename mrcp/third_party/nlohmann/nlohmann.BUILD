load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "nlohmann",
    cache_entries = {
        "JSON_BuildTests": "OFF",
    },
    lib_source = ":all_srcs",
    out_headers_only = True,
)
