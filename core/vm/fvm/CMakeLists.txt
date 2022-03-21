#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(fvm_stub
    stub.cpp
    )
target_link_libraries(fvm_stub
    blob
    )
