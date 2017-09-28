package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "gthread_include",
    hdrs = ["gthread.h"],
)

cc_binary(
    name = "shittythread",
    srcs = ["shittythread.c"],
    deps = [
        "//sched",
    ],
)
