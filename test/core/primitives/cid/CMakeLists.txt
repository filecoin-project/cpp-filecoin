# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_cid_json_test
    cid_json_test.cpp
    )
target_link_libraries(filecoin_cid_json_test
    filecoin_cid
    )
