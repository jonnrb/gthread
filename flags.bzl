COPTS = [
    "-D_LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS",
    "-Werror=thread-safety-analysis",
    "-flto",
]

LINKOPTS = [
    "-flto",
]
