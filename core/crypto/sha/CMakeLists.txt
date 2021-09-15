#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_sha
    sha256.cpp
    )
target_link_libraries(filecoin_sha
    p2p::p2p_sha
    )
