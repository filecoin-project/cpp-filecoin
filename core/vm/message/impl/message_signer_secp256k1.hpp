/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_SEC256k1_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_SEC256k1_HPP

#include "vm/message/message_signer.hpp"

namespace fc::vm::message {

  using fc::crypto::secp256k1::Secp256k1Provider;

  /**
   * Class Secp256k1MessageSigner.
   */
  class Secp256k1MessageSigner : public MessageSigner {
   public:
    Secp256k1MessageSigner();
    Secp256k1MessageSigner(
        std::shared_ptr<Secp256k1Provider> secp256k1_provider);

    /**
     * @copydoc MessageSigner::sign()
     */
    virtual outcome::result<SignedMessage> sign(const UnsignedMessage &msg,
                                                const PrivateKey &key) noexcept;

    /**
     * @copydoc MessageSigner::verify()
     */
    virtual outcome::result<bool> verify(const SignedMessage &msg,
                                         const PublicKey &key) noexcept;

   private:
    std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
  };

}  // namespace fc::vm::message
#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_SEC256k1_HPP
