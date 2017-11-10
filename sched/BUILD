package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "task",
    srcs = [
        "task.cc",
        "task_inline.h",
    ],
    hdrs = ["task.h"],
    deps = [
        "//:gthread_include",
        "//arch",
        "//platform",
        "//util",
    ],
)

cc_library(
    name = "sched",
    srcs = [
        "sched.cc",
        "sched_impl.h",
        "task_freelist.h",
        "task_freelist.cc",
    ],
    hdrs = ["sched.h"],
    deps = [
        ":task",
        "//:gthread_include",
        "//arch",
        "//platform",
        "//util",
    ],
)

cc_test(
    name = "sched_test",
    timeout = "short",
    srcs = ["sched_test.cc"],
    deps = [
        ":sched",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "std_thread_test",
    srcs = ["std_thread_test.cc"],
)
