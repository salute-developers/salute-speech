"""
Dependency to apr, a unit test framework for C++
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_apr():
    # It is not clear in Bazel what is the best practice for using http_archive.
    # If you call http_archive without any kind of check, you could call it two times
    # with the same name and different parameters and you would not get any warning/error.
    #
    # One option is to check if it is already available in the existing_rules and only
    # call http_archive if it is not present. In the else you could display a message in
    # case that was already present but in reality you would only want a warning/error if was
    # already called with different parameters (different library version for example).
    #
    # Another option is to wrap the http_archive in a maybe call but this will also not display
    # a warning. It behaves like the if check with the advantage that the name has not to be
    # repeated
    maybe(
        http_archive,
        name = "apr",
        url = "https://archive.apache.org/dist/apr/apr-1.5.2.tar.gz",
        strip_prefix = "apr-1.5.2",
        patches = [
          "//third_party/apr:0001-Applied-thread-safety-patch-for-APR-memory-pools.patch",
          # "//third_party/apr:0002-Added-a-new-APR-feature-macro-APR_HAS_SETTHREADNAME-.patch",
          # "//third_party/apr:0003-Added-pre-generated-apr_escape_test_char.h-for-unix-.patch",
          # "//third_party/apr:0004-Added-project-files-for-VS2005-and-VS2010.patch",
          # "//third_party/apr:0005-Modified-libapr.rc-file-to-be-able-to-open-it-in-Res.patch",
          # "//third_party/apr:0006-Added-_WIN64-preprocessor-definition-for-x64-targets.patch",
        ],
        build_file = "//third_party/apr:apr.BUILD",
    )
