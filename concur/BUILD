package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "concur",
    deps = [
        ":mutex",
    ],
)

cc_library(
    name = "channel",
    srcs = ["channel_impl.h"],
    hdrs = ["channel.h"],
    deps = [
        "//concur/internal:channel_window",
        "@com_google_absl//absl/types:optional",
    ]
)

cc_test(
  name = "channel_test",
  timeout = "short",
  srcs = ["channel_test.cc"],
  deps = [
      "@com_google_googletest//:gtest_main",
      ":channel",
      "//:gthread",
  ],
)

cc_library(
    name = "mutex",
    srcs = ["mutex.cc"],
    hdrs = ["mutex.h"],
    deps = ["//sched"],
)

cc_test(
    name = "mutex_test",
    timeout = "short",
    srcs = ["mutex_test.cc"],
    deps = [":mutex"],
)
