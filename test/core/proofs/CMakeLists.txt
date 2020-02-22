#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

addtest(filecoin_proofs_test
    proofs_test.cpp
    )

target_link_libraries(filecoin_proofs_test
    filecoin_proofs
    filecoin_proof_param_provider
    filecoin_base_fs_test
    )
