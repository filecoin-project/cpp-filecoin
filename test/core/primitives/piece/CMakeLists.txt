#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(piece_test
    piece_test.cpp
    )
target_link_libraries(piece_test
    piece
    )
