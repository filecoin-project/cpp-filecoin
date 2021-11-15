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
add_subdirectory(config)
add_subdirectory(crypto)
add_subdirectory(data_transfer)
add_subdirectory(drand)
add_subdirectory(fslock)
add_subdirectory(markets)
add_subdirectory(miner)
add_subdirectory(node)
add_subdirectory(paych)
add_subdirectory(power)
add_subdirectory(primitives)
add_subdirectory(proofs)
add_subdirectory(remote_worker)
add_subdirectory(sector_storage)
add_subdirectory(sectorblocks)
add_subdirectory(storage)
add_subdirectory(vm)

add_library(const
    const.cpp
    )
target_link_libraries(const
    Boost::boost
    )
