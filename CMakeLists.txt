cmake_minimum_required(VERSION 3.12)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

set(
    CMAKE_TOOLCHAIN_FILE
    "${CMAKE_SOURCE_DIR}/cmake/toolchain/cxx17.cmake"
    CACHE
    FILEPATH
    "Default toolchain"
)


include("cmake/Hunter/init.cmake")
HunterGate(
    URL  "https://github.com/soramitsu/soramitsu-hunter/archive/v0.23.257-soramitsu23.tar.gz"
    SHA1 "f42cec23fced76800b87191525ee1fa6f9c088a5"
    LOCAL
)

project(cpp_filecoin)

if (NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  # https://cgold.readthedocs.io/en/latest/tutorials/toolchain/globals/cxx-standard.html#summary
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# export compile commands for clang-tidy to analyse only changed files
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TESTING "Build tests" ON)
option(TESTING_PROOFS "Build proofs tests" OFF)
option(TESTING_ACTORS "Build actors tests" OFF)
option(BUILD_INTERNAL_DEPS "Build internal dependencies from git submodules" ON)
option(CLANG_FORMAT "Enable clang-format target" ON)
option(CLANG_TIDY "Enable clang-tidy checks during compilation" OFF)
option(COVERAGE "Enable generation of coverage info" OFF)
# sanitizers will be enabled only for Filecoin, and will be disabled for dependencies
option(ASAN "Enable address sanitizer" OFF)
option(LSAN "Enable leak sanitizer" OFF)
option(MSAN "Enable memory sanitizer" OFF)
option(TSAN "Enable thread sanitizer" OFF)
option(UBSAN "Enable UB sanitizer" OFF)

include(CheckCXXCompilerFlag)
include(cmake/toolchain-util.cmake)
include(cmake/dependencies.cmake)
include(cmake/functions.cmake)
include(cmake/san.cmake)

print("C flags: ${CMAKE_C_FLAGS}")
print("CXX flags: ${CMAKE_CXX_FLAGS}")
print("Using CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")

## setup compilation flags
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "^(AppleClang|Clang|GNU)$")
  # enable those flags
  add_flag(-Wall)
  add_flag(-Wextra)
  add_flag(-Woverloaded-virtual)     # warn if you overload (not override) a virtual function
  add_flag(-Wformat=2)               # warn on security issues around functions that format output (ie printf)
  add_flag(-Wmisleading-indentation) # (only in GCC >= 6.0) warn if indentation implies blocks where blocks do not exist
  add_flag(-Wduplicated-cond)        # (only in GCC >= 6.0) warn if if / else chain has duplicated conditions
  add_flag(-Wduplicated-branches)    # (only in GCC >= 7.0) warn if if / else branches have duplicated code
  add_flag(-Wnull-dereference)       # (only in GCC >= 6.0) warn if a null dereference is detected
  add_flag(-Wdouble-promotion)       # (GCC >= 4.6, Clang >= 3.8) warn if float is implicit promoted to double
  add_flag(-Wsign-compare)
  add_flag(-Wtype-limits)            # size_t - size_t >= 0 -> always true

  # disable those flags
  add_flag(-Wno-unused-command-line-argument)    # clang: warning: argument unused during compilation: '--coverage' [-Wunused-command-line-argument]
  add_flag(-Wno-unused-parameter)    # prints too many useless warnings
  add_flag(-Wno-format-nonliteral)   # prints way too many warnings from spdlog
  add_flag(-Wno-gnu-zero-variadic-macro-arguments)   # https://stackoverflow.com/questions/21266380/is-the-gnu-zero-variadic-macro-arguments-safe-to-ignore

  # promote to errors
  add_flag(-Werror-unused-lambda-capture)  # error if lambda capture is unused
  add_flag(-Werror-return-type)      # warning: control reaches end of non-void function [-Wreturn-type]
  add_flag(-Werror-non-virtual-dtor) # warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
  add_flag(-Werror-sign-compare)     # warn the user if they compare a signed and unsigned numbers
  add_flag(-Werror-reorder)          # field '$1' will be initialized after field '$2'
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  # TODO(warchant): add flags https://github.com/lefticus/cppbestpractices/blob/master/02-Use_the_Tools_Available.md#msvc
endif ()

if (COVERAGE)
  include(cmake/coverage.cmake)
endif ()
if (CLANG_TIDY)
  include(cmake/clang-tidy.cmake)
endif ()
if (CLANG_FORMAT)
  include(cmake/clang-format.cmake)
endif ()

if (BUILD_INTERNAL_DEPS)
  add_subdirectory(deps)
endif()

include_directories(
    # project includes
    ${PROJECT_SOURCE_DIR}/core
    ${PROJECT_SOURCE_DIR}/libs
)

if (BUILD_INTERNAL_DEPS)
  include_directories(
      SYSTEM
      # system includes
      deps/indicators/include
      deps/libsecp256k1/include
  )
endif()

add_subdirectory(libs)
add_subdirectory(core)


if (TESTING)
  enable_testing()
  add_subdirectory(test)
endif ()

install(TARGETS fuhon-node fuhon-miner)
