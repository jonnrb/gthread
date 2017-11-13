package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gthread",
    hdrs = ["gthread.h"],
    srcs = ["gthread_impl.h"],
    deps = [
        "//sched:sched",
        "//sched:task_attr",
        "//util:function_marshall",
    ]
)

cc_test(
    name = "gthread_test",
    srcs = ["gthread_test.cc"],
    deps = [
        ":gthread",
        "@com_google_googletest//:gtest_main",
    ]
)
