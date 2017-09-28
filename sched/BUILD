package(default_visibility = ["//visibility:public"])

cc_library(
    name = "task",
    srcs = ["task.c"],
    hdrs = ["task.h"],
    deps = [
        "//:gthread_include",
        "//arch",
        "//platform",
    ],
)

cc_library(
    name = "sched",
    srcs = ["sched.c"],
    hdrs = ["sched.h"],
    deps = [
    ":task",
        "//:gthread_include",
        "//arch",
        "//platform",
    ],
)
