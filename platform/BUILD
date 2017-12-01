load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "platform",
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":alarm",
        ":clock",
        ":memory",
        ":timer",
        ":tls",
    ],
)

cc_library(
    name = "alarm",
    srcs = ["alarm.cc"],
    hdrs = ["alarm.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":clock",
    ],
)

cc_test(
    name = "alarm_test",
    timeout = "short",
    srcs = ["alarm_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":alarm"],
)

cc_library(
    name = "clock",
    srcs = ["clock.cc"],
    hdrs = ["clock.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        "//util:compiler",
    ],
)

cc_test(
    name = "clock_test",
    timeout = "short",
    srcs = ["clock_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":clock",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "memory",
    srcs = select({
        "@bazel_tools//src:darwin": ["memory_macos.cc"],
        "//conditions:default": ["memory_linux.cc"],
    }) + [
        "memory_inline.h",
    ],
    hdrs = ["memory.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        "//arch:bit_twiddle",
        "//sched:task_attr",
        "//util:compiler",
    ],
)

cc_test(
    name = "allocate_stack_test",
    timeout = "short",
    srcs = ["allocate_stack_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":platform"],
)

cc_test(
    name = "free_stack_test",
    timeout = "short",
    srcs = ["free_stack_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":platform"],
)

cc_library(
    name = "timer",
    hdrs = ["timer.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
)

cc_test(
    name = "timer_test",
    timeout = "short",
    srcs = ["timer_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":timer"],
)

cc_library(
    name = "tls",
    srcs = select({
        "@bazel_tools//src:darwin": [
            "tls_macos.cc",
            "tls_impl_macos.h",
        ],
        "//conditions:default": [
            "tls_impl_linux.h",
            "tls_linux.cc",
        ],
    }),
    hdrs = ["tls.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":clock",
        ":memory",
        "//util",
    ],
)

cc_test(
    name = "tls_test",
    timeout = "short",
    srcs = ["tls_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":tls"],
)
