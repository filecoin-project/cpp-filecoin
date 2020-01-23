#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/core
        )

add_subdirectory(actor)
add_subdirectory(exit_code)
add_subdirectory(message)
add_subdirectory(runtime)
add_subdirectory(state)
