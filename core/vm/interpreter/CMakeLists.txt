#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(interpreter
    interpreter.cpp
    )
target_link_libraries(interpreter
    tipset
    amt
    )
