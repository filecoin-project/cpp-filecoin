/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v3/miner/types/partition.hpp"

#include "vm/actor/builtin/types/type_manager/type_manager.hpp"
#include "vm/runtime/runtime.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::RleBitset;
  using types::TypeManager;

  outcome::result<PowerPair> Partition::addSectors(
      Runtime &runtime,
      bool proven,
      const std::vector<SectorOnChainInfo> &sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    OUTCOME_TRY(expirations,
                TypeManager::loadExpirationQueue(
                    runtime, this->expirations_epochs, quant));

    RleBitset snos;
    PowerPair power;
    OUTCOME_TRY(result, expirations->addActiveSectors(sectors, ssize));
    std::tie(snos, power, std::ignore) = result;
    this->expirations_epochs = expirations->queue;

    if (this->sectors.containsAny(snos)) {
      return ERROR_TEXT("not all added sectors are new");
    }

    this->sectors += snos;
    this->live_power += power;

    if (!proven) {
      this->unproven += snos;
      this->unproven_power += power;
    }

    OUTCOME_TRY(validateState());

    return power;
  }

}  // namespace fc::vm::actor::builtin::v3::miner
