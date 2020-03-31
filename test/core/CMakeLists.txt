#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(adt)
add_subdirectory(api)
add_subdirectory(blockchain)
add_subdirectory(clock)
add_subdirectory(codec)
add_subdirectory(common)
add_subdirectory(crypto)
add_subdirectory(fslock)

if (TESTING_PROOFS)
    add_subdirectory(proofs)
endif ()

add_subdirectory(power)
add_subdirectory(primitives)
add_subdirectory(storage)
add_subdirectory(vm)
