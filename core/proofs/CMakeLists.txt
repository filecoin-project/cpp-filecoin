#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(proof_param_provider
        proof_param_provider.cpp
        proof_param_provider_error.cpp
        )

target_link_libraries(proof_param_provider
        Boost::system
        outcome
        blake2
        logger
        )

add_custom_command(
        TARGET proof_param_provider PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        /var/tmp/filecoin-proof-parameters)

add_custom_command(
        TARGET proof_param_provider PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/parameters.json
        /var/tmp/filecoin-proof-parameters/parameters.json)

add_library(proofs
        proofs.cpp
        proofs_error.cpp
        )

target_link_libraries(proofs
        filecoin_ffi
        outcome
        blob
        proof_param_provider
        logger
        )
