#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(blake2
    blake2s.c
    blake2b.c
    blake2b_error.cpp
    blake2b160.cpp
    )
target_link_libraries(blake2
    blob
    outcome
    )
