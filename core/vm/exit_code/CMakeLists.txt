#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

filecoin_add_library(filecoin_exit_code
    impl/exit_code.cpp
    )
target_link_libraries(filecoin_exit_code
    filecoin_outcome
    )
