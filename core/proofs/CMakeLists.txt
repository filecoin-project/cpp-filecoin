#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(proof_param_provider
        impl/proof_param_provider.cpp
        impl/proof_param_provider_error.cpp
        )

target_link_libraries(proof_param_provider
        CURL::libcurl
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
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/parameters.json
        /var/tmp/filecoin-proof-parameters/parameters.json)

add_library(proofs
        impl/proofs.cpp
        impl/proofs_error.cpp
        )

target_link_libraries(proofs
        filecoin_ffi
        outcome
        blob
        logger
        comm_cid
        piece
        address
        sector
        piece_data
        Boost::filesystem
        )
