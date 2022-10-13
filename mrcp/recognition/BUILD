package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "recognition",
  srcs = glob(["*.cpp"]),
  hdrs = glob(["*.hpp", "*.h"]),
  copts = ["-I$(GENDIR)/external/apr/apr/include/apr-1/"],
  deps = [
    "//grpc_client:grpc_client",
    "@apr",
    "@apr-util",
    "@unimrcp",
  ],
)
