#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(filecoin_bls_provider
    impl/bls_provider_impl.cpp
    )

target_link_libraries(filecoin_bls_provider
    bls_signatures_ffi
    filecoin_outcome
    )
