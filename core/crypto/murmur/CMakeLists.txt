#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_murmur
    murmur.cpp
    )
target_link_libraries(filecoin_murmur
    filecoin_buffer
    )
