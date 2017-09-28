# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//platform/BUILD
# info: platform specific libraries

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "platform",
    deps = [
        ":memory",
        ":timer",
    ],
)

cc_library(
    name = "memory",
    srcs = select({
        "@bazel_tools//src:darwin": ["memory_macos.c"],
    }),
    hdrs = ["memory.h"],
    deps = [
        "//:gthread_include",
        "//arch:bit_twiddle",
    ],
)

cc_test(
    name = "allocate_stack_test",
    srcs = ["allocate_stack_test.c"],
    deps = [":platform"],
)

cc_test(
    name = "free_stack_test",
    srcs = ["free_stack_test.c"],
    deps = [":platform"],
)

cc_library(
    name = "timer",
    srcs = select({
        "@bazel_tools//src:darwin": ["timer_posix.c"],
    }),
    hdrs = ["timer.h"],
)

cc_test(
    name = "timer_test",
    srcs = ["timer_test.c"],
    deps = [":timer"],
)
