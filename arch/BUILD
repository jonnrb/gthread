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
    srcs = ["bit_twiddle_test.cc"],
    deps = [":bit_twiddle"],
)

cc_library(
    name = "switch_to",
    srcs = ["switch_to.S"],
    hdrs = ["switch_to.h"],
)

cc_test(
    name = "switch_to_test",
    srcs = ["switch_to_test.cc"],
    copts = [
        "-Wno-deprecated-declarations",  # uses ucontext.h which is deprecated
    ],
    deps = [
        ":switch_to",
        "//platform",
    ],
)
