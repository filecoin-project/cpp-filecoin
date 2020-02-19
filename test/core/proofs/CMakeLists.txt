#
#Copyright Soramitsu Co., Ltd.All Rights Reserved.
#SPDX - License - Identifier : Apache - 2.0
#

addtest(proofs_test proofs_test.cpp)

target_link_libraries( proofs_test
        proofs
        proof_param_provider
        base_fs_test)
