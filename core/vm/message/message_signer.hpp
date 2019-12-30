/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_HPP

#include <gsl/span>

#include "vm/message/message_codec.hpp"

namespace fc::vm::message {

  using PrivateKey = gsl::span<uint8_t>;
  using PublicKey = gsl::span<uint8_t>;

  /**
   * Interface of an UnisgnedMessage Signer.
   */
  class MessageSigner {
   public:
    virtual ~MessageSigner() = default;

    /**
     * @brief Sign an unsigned message with a private key
     * @param msg - an UnsignedMessage
     * @param key - an algorithm-specific private key
     * @return SignedMessage or error
     */
    virtual outcome::result<SignedMessage> sign(
        const UnsignedMessage &msg, const PrivateKey &key) noexcept = 0;

    /**
     * @brief Verify a signed message against a public key
     * @param msg - a SignedMessage
     * @param key - an algorithm-specific public key
     * @return boolean verification result or error
     */
    virtual outcome::result<bool> verify(const SignedMessage &msg,
                                         const PublicKey &key) noexcept = 0;
  };

}  // namespace fc::vm::message

#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_HPP
