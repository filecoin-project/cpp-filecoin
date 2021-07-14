#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(dvm
    dvm.cpp
    )
target_link_libraries(dvm
    buffer
    cbor
    exit_code
    logger
    )

add_executable(dvm_main
    main.cpp
    )
target_link_libraries(dvm_main
    car
    cids_index
    in_memory_storage
    interpreter
    ipfs_datastore_in_memory
    )
