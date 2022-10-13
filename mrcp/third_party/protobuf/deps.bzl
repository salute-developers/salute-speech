"""
Transitive dependencies for protobuf
"""

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

def load_protobuf_deps():
  rules_proto_dependencies()
  rules_proto_toolchains()
