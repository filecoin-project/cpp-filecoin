#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

if (APPLE)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(libs)
add_subdirectory(testutil)
