# author: JonNRb <jonbetti@gmail.com>
# license: MIT
# file: @gthread//concur/BUILD
# info: concurrency primitives

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "concur",
    deps = [
        ":mutex",
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
