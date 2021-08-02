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

add_library(ipld_verifier
    verifier.cpp
    )
target_link_libraries(ipld_verifier
    filecoin_hasher
    ipld_traverser
    ipfs_datastore_in_memory
    )

add_library(cids_ipld
    cids_ipld.cpp
    )
target_link_libraries(cids_ipld
    cid
    )
