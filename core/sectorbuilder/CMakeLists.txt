#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(proof_verifier
        impl/proof_verifier_impl.cpp
        )

target_link_libraries(proof_verifier
        outcome
        sector
        proofs
        )
