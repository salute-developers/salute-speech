"""
Transitive dependencies for buildtools
"""

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

def load_rules_foreign_cc_deps():
    rules_foreign_cc_dependencies(register_built_tools = False,)
