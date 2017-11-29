package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "util",
    deps = [
        ":compiler",
        ":log",
    ],
)

cc_library(
    name = "close_wrapper",
    hdrs = ["close_wrapper.h"],
)

cc_library(
    name = "compiler",
    hdrs = ["compiler.h"],
)

cc_library(
    name = "function_marshall",
    srcs = ["function_marshall_impl.h"],
    hdrs = ["function_marshall.h"],
    deps = [
        ":compiler",
    ],
)

cc_test(
    name = "function_marshall_test",
    srcs = ["function_marshall_test.cc"],
    deps = [
        ":function_marshall",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "log",
    hdrs = [
        "log.h",
        "log_inline.h",
    ],
)
