function(print)
  message(STATUS "[${CMAKE_PROJECT_NAME}] ${ARGV}")
endfunction()

function(fatal_error)
  message(FATAL_ERROOR "[${CMAKE_PROJECT_NAME}] ${ARGV}")
endfunction()


function(add_cache_flag var_name flag)
  set(spaced_string " ${${var_name}} ")
  string(FIND "${spaced_string}" " ${flag} " flag_index)
  if(NOT flag_index EQUAL -1)
    return()
  endif()
  string(COMPARE EQUAL "" "${${var_name}}" is_empty)
  if(is_empty)
    # beautify: avoid extra space at the end if var_name is empty
    set("${var_name}" "${flag}" CACHE STRING "" FORCE)
  else()
    set("${var_name}" "${flag} ${${var_name}}" CACHE STRING "" FORCE)
  endif()
endfunction()


function(require_compiler CC CXX)
  if(XCODE_VERSION)
    set(_err "This toolchain is not available for Xcode")
    set(_err "${_err} because Xcode ignores CMAKE_C(XX)_COMPILER variable.")
    set(_err "${_err} Use xcode.cmake toolchain instead.")
    fatal_error(${_err})
  endif()

  find_program(CMAKE_C_COMPILER ${CC})
  find_program(CMAKE_CXX_COMPILER ${CXX})

  if(NOT CMAKE_C_COMPILER)
    fatal_error("${CC} not found")
  endif()

  if(NOT CMAKE_CXX_COMPILER)
    fatal_error("${CXX} not found")
  endif()

  set(
      CMAKE_C_COMPILER
      "${CMAKE_C_COMPILER}"
      CACHE
      STRING
      "C compiler"
      FORCE
  )

  set(
      CMAKE_CXX_COMPILER
      "${CMAKE_CXX_COMPILER}"
      CACHE
      STRING
      "C++ compiler"
      FORCE
  )
endfunction()
