load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gthread",
    srcs = ["gthread_impl.h"],
    hdrs = ["gthread.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        "//sched",
        "//util:function_marshall",
    ],
)

cc_test(
    name = "gthread_test",
    srcs = ["gthread_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":gthread",
        "@com_google_googletest//:gtest_main",
    ],
)
