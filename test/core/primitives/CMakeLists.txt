#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(address)
add_subdirectory(block)
add_subdirectory(chain_epoch)
add_subdirectory(cid)
add_subdirectory(rle_bitset)
add_subdirectory(ticket)
add_subdirectory(tipset)

addtest(filecoin_big_int_test
    big_int_test.cpp
    )
target_link_libraries(filecoin_big_int_test
    Boost::boost
    )
