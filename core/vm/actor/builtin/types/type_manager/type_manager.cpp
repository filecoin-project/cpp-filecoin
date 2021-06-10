/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/type_manager/type_manager.hpp"

#include "vm/actor/builtin/v0/miner/types/expiration.hpp"
#include "vm/actor/builtin/v2/miner/types/expiration.hpp"
#include "vm/actor/builtin/v3/miner/types/expiration.hpp"

#include "vm/actor/builtin/v4/todo.hpp"

namespace fc::vm::actor::builtin::types {

  outcome::result<ExpirationQueuePtr> TypeManager::loadExpirationQueue(
      Runtime &runtime,
      const adt::Array<ExpirationSet> &expirations_epochs,
      const QuantSpec &quant) {
    const auto version = runtime.getActorVersion();

    ExpirationQueuePtr expiration_queue;

    switch (version) {
      case ActorVersion::kVersion0:
        expiration_queue = std::make_shared<v0::miner::ExpirationQueue>();
      case ActorVersion::kVersion2:
        expiration_queue = std::make_shared<v2::miner::ExpirationQueue>();
      case ActorVersion::kVersion3:
        expiration_queue = std::make_shared<v3::miner::ExpirationQueue>();
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
    }

    expiration_queue->queue = expirations_epochs;
    expiration_queue->quant = quant;
    OUTCOME_TRY(expiration_queue->queue.amt.loadRoot());

    return expiration_queue;
  }

}  // namespace fc::vm::actor::builtin::types
