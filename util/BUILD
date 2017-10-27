package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "util",
    deps = [
        ":compiler",
        ":log",
    ],
)

cc_library(
    name = "compiler",
    hdrs = ["compiler.h"],
)

cc_library(
    name = "log",
    hdrs = [
        "log.h",
        "log_inline.h",
    ],
)
