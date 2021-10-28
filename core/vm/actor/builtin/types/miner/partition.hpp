/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/streams_annotation.hpp"
#include "common/outcome.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/quantize.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/types/miner/sectors.hpp"
#include "vm/actor/builtin/types/miner/termination.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using primitives::RleBitset;

  constexpr size_t kEearlyTerminatedBitWidth = 3;

  struct Partition {
    RleBitset sectors;
    RleBitset unproven;
    RleBitset faults;
    RleBitset recoveries;
    RleBitset terminated;
    PartitionExpirationsArray expirations_epochs;  // quanted
    adt::Array<RleBitset, kEearlyTerminatedBitWidth> early_terminated;
    PowerPair live_power;
    PowerPair unproven_power;
    PowerPair faulty_power;
    PowerPair recovering_power;

    virtual ~Partition() = default;

    RleBitset liveSectors() const;

    virtual RleBitset activeSectors() const = 0;

    virtual PowerPair activePower() const = 0;

    virtual outcome::result<PowerPair> addSectors(
        bool proven,
        const std::vector<SectorOnChainInfo> &sectors,
        SectorSize ssize,
        const QuantSpec &quant) = 0;

    virtual outcome::result<std::tuple<PowerPair, PowerPair>> addFaults(
        const RleBitset &sector_nos,
        const std::vector<SectorOnChainInfo> &sectors,
        ChainEpoch fault_expiration,
        SectorSize ssize,
        const QuantSpec &quant) = 0;

    outcome::result<std::tuple<RleBitset, PowerPair, PowerPair>> recordFaults(
        const Sectors &sectors,
        const RleBitset &sector_nos,
        ChainEpoch fault_expiration_epoch,
        SectorSize ssize,
        const QuantSpec &quant);

    outcome::result<PowerPair> recoverFaults(const Sectors &sectors,
                                             SectorSize ssize,
                                             const QuantSpec &quant);

    PowerPair activateUnproven();

    outcome::result<void> declareFaultsRecovered(const Sectors &sectors,
                                                 SectorSize ssize,
                                                 const RleBitset &sector_nos);

    outcome::result<void> removeRecoveries(const RleBitset &sector_nos,
                                           const PowerPair &power);

    outcome::result<RleBitset> rescheduleExpirationsV0(
        const Sectors &sectors,
        ChainEpoch new_expiration,
        const RleBitset &sector_nos,
        SectorSize ssize,
        const QuantSpec &quant);

    outcome::result<std::vector<SectorOnChainInfo>> rescheduleExpirationsV2(
        const Sectors &sectors,
        ChainEpoch new_expiration,
        const RleBitset &sector_nos,
        SectorSize ssize,
        const QuantSpec &quant);

    outcome::result<std::tuple<PowerPair, TokenAmount>> replaceSectors(
        const std::vector<SectorOnChainInfo> &old_sectors,
        const std::vector<SectorOnChainInfo> &new_sectors,
        SectorSize ssize,
        const QuantSpec &quant);

    outcome::result<void> recordEarlyTermination(ChainEpoch epoch,
                                                 const RleBitset &sectors);

    virtual outcome::result<ExpirationSet> terminateSectors(
        const Sectors &sectors,
        ChainEpoch epoch,
        const RleBitset &sector_nos,
        SectorSize ssize,
        const QuantSpec &quant) = 0;

    virtual outcome::result<ExpirationSet> popExpiredSectors(
        ChainEpoch until, const QuantSpec &quant) = 0;

    outcome::result<std::tuple<PowerPair, PowerPair>> recordMissedPostV0(
        ChainEpoch fault_expiration, const QuantSpec &quant);

    outcome::result<std::tuple<PowerPair, PowerPair, PowerPair>>
    recordMissedPostV2(ChainEpoch fault_expiration, const QuantSpec &quant);

    outcome::result<std::tuple<TerminationResult, bool>> popEarlyTerminations(
        uint64_t max_sectors);

    outcome::result<std::tuple<PowerPair, PowerPair, PowerPair, bool>>
    recordSkippedFaults(const Sectors &sectors,
                        SectorSize ssize,
                        const QuantSpec &quant,
                        ChainEpoch fault_expiration,
                        const RleBitset &skipped);

    outcome::result<void> validatePowerState() const;

    outcome::result<void> validateBFState() const;

    virtual outcome::result<void> validateState() const = 0;
  };

}  // namespace fc::vm::actor::builtin::types::miner
