/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/multisig/multisig_actor_state.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::vm::actor::builtin::v3::multisig {
  ACTOR_STATE_TO_CBOR_THIS(MultisigActorState)

  std::shared_ptr<states::MultisigActorState> MultisigActorState::copy() const {
    auto copy = std::make_shared<MultisigActorState>(*this);
    return copy;
  }
}  // namespace fc::vm::actor::builtin::v3::multisig
