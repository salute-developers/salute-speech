load("//third_party/boringssl:direct.bzl", "load_boringssl")
load("//third_party/protobuf:direct.bzl", "load_protobuf")
load("//third_party/grpc:direct.bzl", "load_grpc")
load("//third_party/rules_foreign_cc:direct.bzl", "load_rules_foreign_cc")
load("//third_party/nlohmann:direct.bzl", "load_nlohmann")
load("//third_party/zlib:direct.bzl", "load_zlib")
load("//third_party/curl:direct.bzl", "load_curl")
load("//third_party/apr:direct.bzl", "load_apr")
load("//third_party/apr-util:direct.bzl", "load_apr_util")
load("//third_party/sofia-sip:direct.bzl", "load_sofia_sip")
load("//third_party/unimrcp:direct.bzl", "load_unimrcp")

load_boringssl()
load_protobuf()
load_grpc()
load_rules_foreign_cc()
load_nlohmann()
#
load_zlib()
load_curl()
#
load_apr()
load_apr_util()
load_sofia_sip()
load_unimrcp()
#
load("//third_party/bazel-compile-commands:direct.bzl", "load_bazel_compile_commands")
load_bazel_compile_commands()

#
#
load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")
load("//third_party/rules_foreign_cc:deps.bzl", "load_rules_foreign_cc_deps")
load("//third_party/protobuf:deps.bzl", "load_protobuf_deps")

rules_foreign_cc_dependencies()
load_rules_foreign_cc_deps()
load_protobuf_deps()

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()
load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps()
