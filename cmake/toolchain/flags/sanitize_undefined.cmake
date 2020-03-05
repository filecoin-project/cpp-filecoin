if (DEFINED POLLY_FLAGS_SANITIZE_UNDEFINED_CMAKE_)
  return()
else ()
  set(POLLY_FLAGS_SANITIZE_UNDEFINED_CMAKE_ 1)
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/../../toolchain-util.cmake)

set(FLAGS
    -fsanitize=undefined
    -fno-omit-frame-pointer
    -ferror-limit=0
    -g
    )
foreach(FLAG IN LISTS FLAGS)
  add_cache_flag(CMAKE_CXX_FLAGS ${FLAG})
  add_cache_flag(CMAKE_C_FLAGS ${FLAG})
endforeach()


set(ENV{UBSAN_OPTIONS} print_stacktrace=1)
