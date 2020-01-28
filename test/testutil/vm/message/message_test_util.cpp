/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/vm/message/message_test_util.hpp"

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "vm/message/message_util.hpp"

using fc::crypto::bls::impl::BlsProviderImpl;
using fc::vm::message::cid;

fc::outcome::result<SignedMessage> signMessageBls(
    const UnsignedMessage &unsigned_message, const PrivateKey &private_key) {
  BlsProviderImpl bls_provider;
  OUTCOME_TRY(cid, cid(unsigned_message));
  OUTCOME_TRY(cid_serialized,
              libp2p::multi::ContentIdentifierCodec::encode(cid));
  OUTCOME_TRY(signature, bls_provider.sign(cid_serialized, private_key));
  return SignedMessage{unsigned_message, signature};
}
