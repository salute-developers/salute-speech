"""
Dependency to apr-util, a unit test framework for C++
"""

load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_apr_util():
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
        name = "apr-util",
        build_file = "//third_party/apr-util:apr-util.BUILD",
        strip_prefix = "apr-util-1.5.4",
        url = "https://archive.apache.org/dist/apr/apr-util-1.5.4.tar.gz",
    )

    maybe(
        http_archive,
        name = "expat",
        build_file = "//third_party/apr-util:expat.BUILD",
        sha256 = "a00ae8a6b96b63a3910ddc1100b1a7ef50dc26dceb65ced18ded31ab392f132b",
        strip_prefix = "expat-2.4.1",
        url = "https://github.com/libexpat/libexpat/releases/download/R_2_4_1/expat-2.4.1.tar.gz",
    )
