load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//concur:__pkg__"])

cc_library(
    name = "channel_window",
    srcs = ["channel_window_impl.h"],
    hdrs = ["channel_window.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":waiter",
        "//:gthread",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_test(
    name = "channel_window_test",
    timeout = "short",
    srcs = ["channel_window_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":channel_window",
        "//:gthread",
        "//concur:mutex",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "ring_buffer",
    srcs = ["ring_buffer_impl.h"],
    hdrs = ["ring_buffer.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":waiter",
        "//:gthread",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_test(
    name = "ring_buffer_test",
    timeout = "short",
    srcs = ["ring_buffer_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":ring_buffer",
        "//:gthread",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "spin_lock",
    hdrs = ["spin_lock.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = ["//sched"],
)

cc_library(
    name = "waiter",
    srcs = [
        "waiter.cc",
        "waiter_impl.h",
    ],
    hdrs = ["waiter.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = ["//sched"],
)

cc_binary(
    name = "waiter_benchmark",
    srcs = ["waiter_benchmark.cc"],
    copts = COPTS,
    linkopts = LINKOPTS + ["-lpthread"],
    deps = [
        ":waiter",
        "//:gthread",
        "@jonnrb_bazel_googlebenchmark//:benchmark",
    ],
)

cc_test(
    name = "waiter_test",
    timeout = "short",
    srcs = ["waiter_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":waiter",
        "//:gthread",
        "@com_google_googletest//:gtest_main",
    ],
)
