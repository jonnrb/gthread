package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "arch",
    deps = [
        ":bit_twiddle",
        ":switch_to",
    ],
)

cc_inc_library(
    name = "bit_twiddle",
    hdrs = ["bit_twiddle.h"],
)

cc_test(
    name = "bit_twiddle_test",
    timeout = "short",
    srcs = ["bit_twiddle_test.cc"],
    deps = [":bit_twiddle"],
)

cc_library(
    name = "switch_to",
    srcs = ["switch_to.s"],
    hdrs = ["switch_to.h"],
)

cc_test(
    name = "switch_to_test",
    timeout = "short",
    srcs = ["switch_to_test.cc"],
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
    ],
    linkopts = [
        "-pthread",
    ],
    deps = [
        ":switch_to",
        "//platform:memory",
        "@jonnrb_bazel_googlebenchmark//:benchmark",
    ],
)
