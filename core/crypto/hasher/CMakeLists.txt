#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_hasher
    hasher.cpp
    )
target_link_libraries(filecoin_hasher
    blake2
    filecoin_sha
    )
