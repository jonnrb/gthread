load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "rq",
    srcs = [
        "rq.cc",
        "rq_impl.h",
    ],
    hdrs = ["rq.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":time_ordered_queue",
        "//sched:task",
    ],
)

cc_library(
    name = "task_freelist",
    srcs = ["task_freelist.cc"],
    hdrs = ["task_freelist.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = ["//sched:task"],
)

cc_library(
    name = "time_ordered_queue",
    srcs = ["time_ordered_queue_impl.h"],
    hdrs = ["time_ordered_queue.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
)
