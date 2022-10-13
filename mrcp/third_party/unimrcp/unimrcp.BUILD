load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
  name = "all_srcs",
  srcs = glob(["**"]),
)

configure_make(
  name = "unimrcp",
  autogen = True,
  autogen_command = "bootstrap",
  configure_in_place = True,
  configure_options = [
    "--with-apr=$EXT_BUILD_DEPS/apr",
    "--with-apr-util=$EXT_BUILD_DEPS/apr-util",
    "--with-sofia-sip=$EXT_BUILD_DEPS/sofia-sip",
  ],
  lib_source=":all_srcs",
  out_static_libs = ["libunimrcpserver.a"],
  visibility = ["//visibility:public"],
  deps = [
    "@apr",
    "@apr-util",
    "@sofia-sip"
  ],
)
