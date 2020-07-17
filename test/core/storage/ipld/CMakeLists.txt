# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(ipld_verifier_test
    ipld_verifier_test.cpp
    )
target_link_libraries(ipld_verifier_test
    ipld_verifier
    )

