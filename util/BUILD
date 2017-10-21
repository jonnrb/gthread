# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//util/BUILD
# info: utilities and data structures

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "util",
    deps = [
        ":compiler",
        ":list",
        ":log",
        ":rb",
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

cc_library(
    name = "rb",
    srcs = ["rb.cc"],
    hdrs = ["rb.h"],
)

cc_test(
    name = "rb_test",
    srcs = ["rb_test.cc"],
    deps = [
        ":rb",
        "//platform:clock",
    ],
)

cc_library(
    name = "list",
    srcs = ["list.cc"],
    hdrs = ["list.h"],
    deps = [
        ":compiler",
    ],
)

cc_test(
    name = "list_test",
    srcs = ["list_test.cc"],
    deps = [
        ":list",
    ],
)
