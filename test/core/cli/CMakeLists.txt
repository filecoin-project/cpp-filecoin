#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(cli_test
    test.cpp
    )
target_link_libraries(cli_test
    cli
    )
