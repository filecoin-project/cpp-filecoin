#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(bls_provider
    impl/bls_provider_impl.cpp
    )

target_link_libraries(bls_provider
    filecoin_ffi
    outcome
    )
