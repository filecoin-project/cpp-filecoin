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

add_library(fvm
    fvm.cpp
    )
target_link_libraries(fvm
    circulating
    filecoin_ffi
    outcome
    keystore
    )
