/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MOCKS_BLOCKCHAIN_MESSAGE_POOL_STORAGE
#define CPP_FILECOIN_MOCKS_BLOCKCHAIN_MESSAGE_POOL_STORAGE

#include <gmock/gmock.h>

#include "blockchain/message_pool/message_storage.hpp"

namespace fc::blockchain::message_pool {
  class MessageStorageMock : public MessageStorage {
   public:
    MOCK_METHOD1(put, outcome::result<void>(const SignedMessage &message));

    MOCK_METHOD1(remove, void(const SignedMessage &message));

    MOCK_CONST_METHOD1(getTopScored, std::vector<SignedMessage>(size_t n));
  };
}  // namespace fc::blockchain::message_pool

#endif
