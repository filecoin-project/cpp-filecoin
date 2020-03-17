#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(signature
    signature.cpp
    )
target_link_libraries(signature
    outcome
    )
