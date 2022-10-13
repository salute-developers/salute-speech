package(default_visibility = ["//visibility:public"])

load("@rules_cc//cc:defs.bzl", "cc_library")

cc_binary(
  name = "smartspeech-recognition-plugin.so",
  linkshared = True,
  srcs = ["smartspeech-recognition-demo.c"],
  copts = ["-I$(GENDIR)/external/apr/apr/include/apr-1/"],
  deps = [
    "//recognition:recognition",
    "@unimrcp",
  ],
)

cc_binary(
  name = "smartspeech-synthesis-plugin.so",
  linkshared = True,
  srcs = ["smartspeech-synthesis-demo.c"],
  copts = ["-I$(GENDIR)/external/apr/apr/include/apr-1/"],
  deps = [
    "//synthesis:synthesis",
    "@unimrcp",
  ],
)
