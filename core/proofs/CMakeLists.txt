#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_proof_param_provider
        proof_param_provider.cpp
        proof_param_provider_error.cpp
        )

target_link_libraries(filecoin_proof_param_provider
        Boost::system
        filecoin_outcome
        filecoin_blake2
        filecoin_logger
        )

add_custom_command(
        TARGET filecoin_proof_param_provider PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        /var/tmp/filecoin-proof-parameters)

add_custom_command(
        TARGET filecoin_proof_param_provider PRE_LINK
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/parameters.json
        /var/tmp/filecoin-proof-parameters/parameters.json)

filecoin_add_library(filecoin_proofs
        proofs.cpp
        proofs_error.cpp
        )

target_link_libraries(filecoin_proofs
        filecoin_ffi
        filecoin_outcome
        filecoin_blob
        filecoin_logger
        filecoin_comm_cid
        filecoin_piece
        )
