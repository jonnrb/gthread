package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gthread",
    srcs = ["gthread_impl.h"],
    hdrs = ["gthread.h"],
    deps = [
        "//sched",
        "//util:function_marshall",
    ],
)

cc_test(
    name = "gthread_test",
    srcs = ["gthread_test.cc"],
    deps = [
        ":gthread",
        "@com_google_googletest//:gtest_main",
    ],
)
