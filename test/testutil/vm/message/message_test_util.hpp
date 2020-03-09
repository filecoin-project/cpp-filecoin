/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESTUTIL_VM_MESSAGE_MESSAGETESTUTIL_HPP
#define CPP_FILECOIN_TESTUTIL_VM_MESSAGE_MESSAGETESTUTIL_HPP

#include "filecoin/common/outcome.hpp"
#include "filecoin/vm/message/message.hpp"

using fc::vm::message::SignedMessage;
using fc::vm::message::UnsignedMessage;
using PrivateKey = std::array<uint8_t, 32>;

fc::outcome::result<SignedMessage> signMessageBls(
    const UnsignedMessage &unsigned_message, const PrivateKey &private_key);

#endif  // CPP_FILECOIN_TESTUTIL_VM_MESSAGE_MESSAGETESTUTIL_HPP
