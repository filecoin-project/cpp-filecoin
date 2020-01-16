/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_HPP

#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::message {

  using primitives::address::Address;

  /**
   * Interface of an UnisgnedMessage Signer.
   */
  class MessageSigner {
   public:
    virtual ~MessageSigner() = default;

    /**
     * @brief Sign an unsigned message with a private key
     * @param address - the signer's address
     * @param msg - an UnsignedMessage
     * @return SignedMessage or error
     */
    virtual outcome::result<SignedMessage> sign(
        const Address &address, const UnsignedMessage &msg) noexcept = 0;

    /**
     * @brief Verify a signed message against a public key
     * @param address - the address on whose behalf the message was signed
     * @param msg - a SignedMessage
     * @return original UnsignedMessage or error
     */
    virtual outcome::result<UnsignedMessage> verify(
        const Address &address, const SignedMessage &msg) noexcept = 0;
  };

}  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_HPP
