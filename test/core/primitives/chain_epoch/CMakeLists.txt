# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(chain_epoch_codec_test
    chain_epoch_codec_test.cpp
    )
target_link_libraries(chain_epoch_codec_test
    uvarint_key
    )
