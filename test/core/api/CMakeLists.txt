#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(api_version_test
    version_test.cpp
    )

addtest(full_node_api_test
    full_node_api_test.cpp
    )
target_link_libraries(full_node_api_test
    api
    rpc
    )
