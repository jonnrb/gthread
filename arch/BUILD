load("//:flags.bzl", "COPTS", "LINKOPTS")

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "arch",
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":bit_twiddle",
        ":switch_to",
    ],
)

cc_library(
    name = "bit_twiddle",
    hdrs = ["bit_twiddle.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
)

cc_test(
    name = "bit_twiddle_test",
    timeout = "short",
    srcs = ["bit_twiddle_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [":bit_twiddle"],
)

cc_library(
    name = "switch_to",
    srcs = ["switch_to.s"],
    hdrs = ["switch_to.h"],
    copts = COPTS,
    linkopts = LINKOPTS,
)

cc_test(
    name = "switch_to_test",
    timeout = "short",
    srcs = ["switch_to_test.cc"],
    copts = COPTS,
    linkopts = LINKOPTS,
    deps = [
        ":switch_to",
        "//platform",
    ],
)

cc_binary(
    name = "switch_to_benchmark",
    srcs = ["switch_to_benchmark.cc"],
    copts = [
        "-Wno-deprecated-declarations",  # uses ucontext.h which is deprecated
        "-D_XOPEN_SOURCE",  # for ucontext.h on macOS
    ] + COPTS,
    linkopts = [
        "-pthread",
    ] + LINKOPTS,
    deps = [
        ":switch_to",
        "//platform:memory",
        "@jonnrb_bazel_googlebenchmark//:benchmark",
    ],
)
