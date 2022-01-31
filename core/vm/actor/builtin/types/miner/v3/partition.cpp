/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/v3/partition.hpp"

#include "common/error_text.hpp"

namespace fc::vm::actor::builtin::v3::miner {
  using primitives::RleBitset;
  using types::miner::loadExpirationQueue;

  outcome::result<PowerPair> Partition::addSectors(
      bool proven,
      const std::vector<Universal<SectorOnChainInfo>> &sectors,
      SectorSize ssize,
      const QuantSpec &quant) {
    auto expirations = loadExpirationQueue(this->expirations_epochs, quant);

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
