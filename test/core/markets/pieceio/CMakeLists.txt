#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(pieceio_test
    pieceio_test.cpp
    )
target_link_libraries(pieceio_test
    pieceio
    unixfs
    ipfs_datastore_in_memory
    )
