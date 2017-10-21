# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//platform/BUILD
# info: platform specific libraries

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "platform",
    deps = [
        ":clock",
        ":memory",
        ":timer",
        ":tls",
    ],
)

cc_library(
    name = "memory",
    srcs = select({
        "@bazel_tools//src:darwin": ["memory_macos.cc"],
    }) + [
        "memory_inline.h",
    ],
    hdrs = ["memory.h"],
    deps = [
        "//:gthread_include",
        "//arch:bit_twiddle",
        "//util:compiler",
    ],
)

cc_test(
    name = "allocate_stack_test",
    srcs = ["allocate_stack_test.cc"],
    deps = [":platform"],
)

cc_test(
    name = "free_stack_test",
    srcs = ["free_stack_test.cc"],
    deps = [":platform"],
)

cc_library(
    name = "clock",
    srcs = ["clock.cc"],
    hdrs = ["clock.h"],
    deps = [
        "//util:compiler",
    ],
)

cc_test(
    name = "clock_test",
    srcs = ["clock_test.cc"],
    deps = [
        ":clock",
    ],
)

cc_library(
    name = "timer",
    srcs = ["timer.cc"],
    hdrs = ["timer.h"],
    deps = [
        ":clock",
    ],
)

cc_test(
    name = "timer_test",
    srcs = ["timer_test.cc"],
    deps = [":timer"],
)

cc_library(
    name = "tls",
    srcs = select({
        "@bazel_tools//src:darwin": [
            "tls_macos.cc",
            "tls_inline_macos.h",
        ],
    }),
    hdrs = ["tls.h"],
    deps = [
        ":clock",
        ":memory",
    ],
)

cc_test(
    name = "tls_test",
    srcs = ["tls_test.cc"],
    deps = [":tls"],
)
