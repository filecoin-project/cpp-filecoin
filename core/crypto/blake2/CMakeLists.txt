#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_blake2
    blake2s.c
    blake2b.c
    blake2b160.cpp
    )
target_link_libraries(filecoin_blake2
    filecoin_blob
    filecoin_outcome
    )
