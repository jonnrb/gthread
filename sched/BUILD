load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "task_attr",
    hdrs = ["task_attr.h"],
)

cc_library(
    name = "sched",
    srcs = [
        "sched.cc",
        "sched_impl.h",
    ],
    hdrs = ["sched.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":preempt",
        ":task",
        ":task_attr",
        "//arch",
        "//platform",
        "//sched/internal:task_freelist",
        "//util",
    ],
)

cc_test(
    name = "sched_test",
    timeout = "short",
    srcs = ["sched_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":sched",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "preempt",
    srcs = ["preempt.cc"],
    hdrs = ["preempt.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":sched_node",
        "//platform:alarm",
    ],
)

cc_test(
    name = "preempt_test",
    srcs = ["preempt_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":preempt"],
)

cc_library(
    name = "task",
    srcs = [
        "task.cc",
        "task_impl.h",
    ],
    hdrs = ["task.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":task_attr",
        "//arch",
        "//platform",
        "//util",
    ],
)

cc_binary(
    name = "task_benchmark",
    srcs = ["task_benchmark.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":task",
        "@jonnrb_bazel_googlebenchmark//:benchmark",
    ],
)

cc_test(
    name = "task_test",
    timeout = "short",
    srcs = ["task_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":task",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "sched_node",
    srcs = ["sched_node.cc"],
    hdrs = ["sched_node.h"],
    deps = [
        "//sched/internal:rq",
        "//sched/internal:task_freelist",
    ],
)

cc_test(
    name = "std_thread_test",
    timeout = "short",
    srcs = ["std_thread_test.cc"],
    copts = COPTS,
    linkopts = ["-lpthread"] + LINKOPTS,
)
