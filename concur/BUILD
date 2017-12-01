load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "concur",
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":channel",
        ":mutex",
    ],
)

cc_library(
    name = "channel",
    srcs = ["channel_impl.h"],
    hdrs = ["channel.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//concur/internal:channel_window",
        "//concur/internal:ring_buffer",
        "//util:close_wrapper",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_binary(
    name = "channel_benchmark",
    srcs = ["channel_benchmark.cc"],
    copts = COPTS,
    linkopts = [
        "-pthread",
    ] + LINKOPTS,
    deps = [
        ":channel",
        "@jonnrb_bazel_googlebenchmark//:benchmark",
    ],
)

cc_test(
    name = "channel_test",
    timeout = "short",
    srcs = ["channel_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":channel",
        "//:gthread",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mutex",
    srcs = ["mutex.cc"],
    hdrs = ["mutex.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:gthread",
        "//concur/internal:spin_lock",
        "//concur/internal:waiter",
        "//sched",
    ],
)

cc_binary(
    name = "mutex_benchmark",
    srcs = ["mutex_benchmark.cc"],
    copts = COPTS,
    linkopts = ["-pthread"] + LINKOPTS,
    deps = [
        ":mutex",
        "@jonnrb_bazel_googlebenchmark//:benchmark",
    ],
)

cc_test(
    name = "mutex_test",
    timeout = "short",
    srcs = ["mutex_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":mutex",
        "//:gthread",
        "@com_google_googletest//:gtest_main",
    ],
)
