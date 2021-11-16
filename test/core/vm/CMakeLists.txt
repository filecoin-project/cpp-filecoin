#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

if (TESTING_ACTORS)
    add_subdirectory(actor)
endif()

add_subdirectory(exit_code)
add_subdirectory(interpreter)
add_subdirectory(message)
add_subdirectory(state)
