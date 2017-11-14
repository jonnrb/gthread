package(default_visibility = ["//concur:__pkg__"])

cc_library(
    name = "channel_window",
    hdrs = ["channel_window.h"],
    srcs = ["channel_window_impl.h"],
    deps = [
        "//sched:sched",
        "//sched:task",
        "@com_google_absl//absl/types:optional",
    ],
)

cc_test(
    name = "channel_window_test",
    srcs = ["channel_window_test.cc"],
    deps = [
        ":channel_window",
        "//concur:mutex",
        "//:gthread",
        "@com_google_googletest//:gtest_main",
    ],
)
