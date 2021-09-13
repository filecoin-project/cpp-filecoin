/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/multisig/transaction.hpp"
#include "codec/cbor/cbor_codec.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::types::multisig {
  using fc::vm::runtime::Runtime;

  outcome::result<Buffer> Transaction::hash(Runtime &runtime) const {
    ProposalHashData hash_data(*this);
    OUTCOME_TRY(to_hash, codec::cbor::encode(hash_data));
    OUTCOME_TRY(hash, runtime.hashBlake2b(to_hash));
    return Buffer{gsl::make_span(hash)};
  }
}  // namespace fc::vm::actor::builtin::types::multisig
