/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_IMPL_HPP
#define CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_IMPL_HPP

#include "common/logger.hpp"
#include "storage/keystore/keystore.hpp"
#include "vm/message/message_signer.hpp"

namespace fc::vm::message {

  using storage::keystore::KeyStore;

  /**
   * Class MessageSignerImpl.
   */
  class MessageSignerImpl : public MessageSigner {
   public:
    explicit MessageSignerImpl(std::shared_ptr<KeyStore> ks);

    /**
     * @copydoc MessageSigner::sign()
     */
    outcome::result<SignedMessage> sign(
        const Address &address, const UnsignedMessage &msg) noexcept override;

    /**
     * @copydoc MessageSigner::verify()
     */
    outcome::result<UnsignedMessage> verify(const Address &address,
                                            const SignedMessage &msg) const
        noexcept override;

   private:
    std::shared_ptr<KeyStore> keystore_;
    common::Logger logger_;
  };

}  // namespace fc::vm::message
#endif  // CPP_FILECOIN_CORE_VM_MESSAGE_SIGNER_IMPL_HPP
