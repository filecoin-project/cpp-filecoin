#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(fsm_test
        fsm_test.cpp
       )
target_link_libraries(fsm_test
    fuhon_fsm
    )
