#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(piece_storage_test
    piece_storage_test.cpp
    )
target_link_libraries(piece_storage_test
    piece_storage
    in_memory_storage
   )
