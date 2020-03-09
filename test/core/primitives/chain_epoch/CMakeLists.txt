# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(filecoin_chain_epoch_codec_test
    chain_epoch_codec_test.cpp
    )
target_link_libraries(filecoin_chain_epoch_codec_test
    filecoin_chain_epoch_codec
    )
