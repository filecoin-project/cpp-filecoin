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

    switch (version) {
      case ActorVersion::kVersion0:
        return createLoadedExpirationQueuePtr<v0::miner::ExpirationQueue>(
            runtime.getIpfsDatastore(), expirations_epochs, quant);
      case ActorVersion::kVersion2:
        return createLoadedExpirationQueuePtr<v2::miner::ExpirationQueue>(
            runtime.getIpfsDatastore(), expirations_epochs, quant);
      case ActorVersion::kVersion3:
        return createLoadedExpirationQueuePtr<v3::miner::ExpirationQueue>(
            runtime.getIpfsDatastore(), expirations_epochs, quant);
      case ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

}  // namespace fc::vm::actor::builtin::types
