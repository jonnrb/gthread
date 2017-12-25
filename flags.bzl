COPTS = [
    "-D_LIBCPP_ENABLE_THREAD_SAFETY_ANNOTATIONS",  # for llvm
    #"-Werror=thread-safety-analysis",
    #"-flto",
    "-pedantic",
]

LINKOPTS = [
    "-flto",
]
