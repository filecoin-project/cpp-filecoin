/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_BLS_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_BLS_HPP

#include "vm/message/message_signer.hpp"

namespace fc::vm::message {

  using fc::crypto::bls::BlsProvider;

  /**
   * Class BlsMessageSigner.
   */
  class BlsMessageSigner : public MessageSigner {
   public:
    BlsMessageSigner();
    explicit BlsMessageSigner(std::shared_ptr<BlsProvider> bls_provider);

    /**
     * @copydoc MessageSigner::sign()
     */
    outcome::result<SignedMessage> sign(
        const UnsignedMessage &msg, const PrivateKey &key) noexcept override;

    /**
     * @copydoc MessageSigner::verify()
     */
    outcome::result<bool> verify(const SignedMessage &msg,
                                 const PublicKey &key) noexcept override;

   private:
    std::shared_ptr<BlsProvider> bls_provider_;
  };

}  // namespace fc::vm::message
#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_BLS_HPP
