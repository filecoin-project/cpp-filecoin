# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(weight_calculator_test
    weight_calculator_test.cpp
    )
target_link_libraries(weight_calculator_test
    ipfs_datastore_in_memory
    weight_calculator
    )
