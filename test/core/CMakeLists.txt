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
add_subdirectory(fsm)
add_subdirectory(markets)
add_subdirectory(miner)
add_subdirectory(node)
add_subdirectory(paych)
add_subdirectory(power)
add_subdirectory(primitives)

if (TESTING_PROOFS)
    add_subdirectory(proofs)
endif ()

add_subdirectory(sector_storage)
add_subdirectory(sectorblocks)
add_subdirectory(serialization)
add_subdirectory(storage)

if (TESTING_ACTORS)
    add_subdirectory(test_vectors)
endif()

add_subdirectory(vm)
