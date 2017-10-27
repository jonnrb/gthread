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
        "stats.h",
        "stats_impl.h",
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
    srcs = ["sched_test.cc"],
    deps = [":sched"],
)
