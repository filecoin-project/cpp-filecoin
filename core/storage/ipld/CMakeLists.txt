#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

add_library(ipld_traverser
    traverser.cpp
    )
target_link_libraries(ipld_traverser
    cbor
    )

add_library(cids_ipld
    cids_ipld.cpp
    )
target_link_libraries(cids_ipld
    cid
    )

add_library(memory_indexed_car
    memory_indexed_car.cpp
    )
target_link_libraries(memory_indexed_car
    cbor
    )
