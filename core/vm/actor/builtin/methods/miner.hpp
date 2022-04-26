/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

#include <libp2p/multi/multiaddress.hpp>
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "common/smoothing/alpha_beta_filter.hpp"
#include "vm/actor/builtin/types/miner/expiration.hpp"
#include "vm/actor/builtin/types/miner/post_partition.hpp"
#include "vm/actor/builtin/types/miner/replica_update.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/types/storage_power/miner_params.hpp"

namespace fc::vm::actor::builtin {
  using common::smoothing::FilterEstimate;
  using crypto::randomness::Randomness;
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using primitives::sector::PoStProof;
  using primitives::sector::Proof;
  using types::miner::ExpirationExtension;
  using types::miner::FaultDeclaration;
  using types::miner::PoStPartition;
  using types::miner::RecoveryDeclaration;
  using types::miner::ReplicaUpdate;
  using types::miner::SectorDeclaration;
  using types::miner::SectorPreCommitInfo;
  using types::storage_power::CreateMinerParams;

  // These methods must be actual with the last version of actors

  struct MinerActor {
    enum class Method : MethodNumber {
      kConstruct = 1,
      kControlAddresses,
      kChangeWorkerAddress,
      kChangePeerId,
      kSubmitWindowedPoSt,
      kPreCommitSector,
      kProveCommitSector,
      kExtendSectorExpiration,
      kTerminateSectors,
      kDeclareFaults,
      kDeclareFaultsRecovered,
      kOnDeferredCronEvent,
      kCheckSectorProven,
      kApplyRewards,  // since v2, AddLockedFund for v0
      kReportConsensusFault,
      kWithdrawBalance,
      kConfirmSectorProofsValid,
      kChangeMultiaddresses,
      kCompactPartitions,
      kCompactSectorNumbers,
      kConfirmUpdateWorkerKey,  // since v2
      kRepayDebt,               // since v2
      kChangeOwnerAddress,      // since v2
      kDisputeWindowedPoSt,     // since v3
      kPreCommitSectorBatch,    // since v5
      kProveCommitAggregate,    // since v5
      kProveReplicaUpdates,     // since v7
    }

    struct Construct : ActorMethodBase<Method::kConstruct> {
      using Params = types::storage_power::CreateMinerParams;
    };

    struct ControlAddresses : ActorMethodBase<Method::kControlAddresses> {
      struct Result {
        Address owner;
        Address worker;
        std::vector<Address> control;

        inline bool operator==(const Result &other) const {
          return owner == other.owner && worker == other.worker
                 && control == other.control;
        }

        inline bool operator!=(const Result &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ControlAddresses::Result, owner, worker, control)

    struct ChangeWorkerAddress : ActorMethodBase<Method::kChangeWorkerAddress> {
      struct Params {
        Address new_worker;
        std::vector<Address> new_control_addresses;

        inline bool operator==(const Params &other) const {
          return new_worker == other.new_worker
                 && new_control_addresses == other.new_control_addresses;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ChangeWorkerAddress::Params, new_worker, new_control_addresses)

    struct ChangePeerId : ActorMethodBase<Method::kChangePeerId> {
      struct Params {
        Bytes new_id;

        inline bool operator==(const Params &other) const {
          return new_id == other.new_id;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ChangePeerId::Params, new_id)

    struct SubmitWindowedPoSt : ActorMethodBase<Method::kSubmitWindowedPoSt> {
      struct Params {
        uint64_t deadline{};
        std::vector<PoStPartition> partitions;
        std::vector<PoStProof> proofs;
        ChainEpoch chain_commit_epoch{};
        Randomness chain_commit_rand;

        inline bool operator==(const Params &other) const {
          return deadline == other.deadline && partitions == other.partitions
                 && proofs == other.proofs
                 && chain_commit_epoch == other.chain_commit_epoch
                 && chain_commit_rand == other.chain_commit_rand;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(SubmitWindowedPoSt::Params,
               deadline,
               partitions,
               proofs,
               chain_commit_epoch,
               chain_commit_rand)

    struct PreCommitSector : ActorMethodBase<Method::kPreCommitSector> {
      using Params = SectorPreCommitInfo;
    };

    struct ProveCommitSector : ActorMethodBase<Method::kProveCommitSector> {
      struct Params {
        SectorNumber sector{};
        Proof proof;

        inline bool operator==(const Params &other) const {
          return sector == other.sector && proof == other.proof;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ProveCommitSector::Params, sector, proof)

    struct ExtendSectorExpiration
        : ActorMethodBase<Method::kExtendSectorExpiration> {
      struct Params {
        std::vector<ExpirationExtension> extensions;

        inline bool operator==(const Params &other) const {
          return extensions == other.extensions;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ExtendSectorExpiration::Params, extensions)

    struct TerminateSectors : ActorMethodBase<Method::kTerminateSectors> {
      struct Params {
        std::vector<SectorDeclaration> terminations;

        inline bool operator==(const Params &other) const {
          return terminations == other.terminations;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };

      struct Result {
        bool done{false};

        inline bool operator==(const Result &other) const {
          return done == other.done;
        }

        inline bool operator!=(const Result &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(TerminateSectors::Params, terminations)
    CBOR_TUPLE(TerminateSectors::Result, done)

    struct DeclareFaults : ActorMethodBase<Method::kDeclareFaults> {
      struct Params {
        std::vector<FaultDeclaration> faults;

        inline bool operator==(const Params &other) const {
          return faults == other.faults;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(DeclareFaults::Params, faults)

    struct DeclareFaultsRecovered
        : ActorMethodBase<Method::kDeclareFaultsRecovered> {
      struct Params {
        std::vector<RecoveryDeclaration> recoveries;

        inline bool operator==(const Params &other) const {
          return recoveries == other.recoveries;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(DeclareFaultsRecovered::Params, recoveries)

    struct OnDeferredCronEvent : ActorMethodBase<Method::kOnDeferredCronEvent> {
      struct Params {
        Bytes event_payload;
        FilterEstimate reward_smoothed;
        FilterEstimate qa_power_smoothed;

        inline bool operator==(const Params &other) const {
          return event_payload == other.event_payload
                 && reward_smoothed == other.reward_smoothed
                 && qa_power_smoothed == other.qa_power_smoothed;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(OnDeferredCronEvent::Params,
               event_payload,
               reward_smoothed,
               qa_power_smoothed)

    struct CheckSectorProven : ActorMethodBase<Method::kCheckSectorProven> {
      struct Params {
        SectorNumber sector{};

        inline bool operator==(const Params &other) const {
          return sector == other.sector;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(CheckSectorProven::Params, sector)

    struct ApplyRewards : ActorMethodBase<Method::kApplyRewards> {
      struct Params {
        TokenAmount reward;
        TokenAmount penalty;

        inline bool operator==(const Params &other) const {
          return reward == other.reward && penalty == other.penalty;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ApplyRewards::Params, reward, penalty)

    struct ReportConsensusFault
        : ActorMethodBase<Method::kReportConsensusFault> {
      struct Params {
        Bytes block_header_1;
        Bytes block_header_2;
        Bytes block_header_extra;

        inline bool operator==(const Params &other) const {
          return block_header_1 == other.block_header_1
                 && block_header_2 == other.block_header_2
                 && block_header_extra == other.block_header_extra;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ReportConsensusFault::Params,
               block_header_1,
               block_header_2,
               block_header_extra)

    struct WithdrawBalance : ActorMethodBase<Method::kWithdrawBalance> {
      struct Params {
        TokenAmount amount;

        inline bool operator==(const Params &other) const {
          return amount == other.amount;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };

      using Result = TokenAmount;
    };
    CBOR_TUPLE(WithdrawBalance::Params, amount)

    struct ConfirmSectorProofsValid
        : ActorMethodBase<Method::kConfirmSectorProofsValid> {
      struct Params {
        std::vector<SectorNumber> sectors;

        inline bool operator==(const Params &other) const {
          return sectors == other.sectors;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ConfirmSectorProofsValid::Params, sectors)

    struct ChangeMultiaddresses
        : ActorMethodBase<Method::kChangeMultiaddresses> {
      struct Params {
        std::vector<Multiaddress> new_multiaddresses;

        inline bool operator==(const Params &other) const {
          return new_multiaddresses == other.new_multiaddresses;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ChangeMultiaddresses::Params, new_multiaddresses)

    struct CompactPartitions : ActorMethodBase<Method::kCompactPartitions> {
      struct Params {
        uint64_t deadline{};
        RleBitset partitions;

        inline bool operator==(const Params &other) const {
          return deadline == other.deadline && partitions == other.partitions;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(CompactPartitions::Params, deadline, partitions)

    struct CompactSectorNumbers
        : ActorMethodBase<Method::kCompactSectorNumbers> {
      struct Params {
        RleBitset mask_sector_numbers;

        inline bool operator==(const Params &other) const {
          return mask_sector_numbers == other.mask_sector_numbers;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(CompactSectorNumbers::Params, mask_sector_numbers)

    struct ConfirmUpdateWorkerKey
        : ActorMethodBase<Method::kConfirmUpdateWorkerKey> {};

    struct RepayDebt : ActorMethodBase<Method::kRepayDebt> {};

    struct ChangeOwnerAddress : ActorMethodBase<Method::kChangeOwnerAddress> {
      using Params = Address;
    };

    struct DisputeWindowedPoSt : ActorMethodBase<Method::kDisputeWindowedPoSt> {
      struct Params {
        uint64_t deadline{};
        uint64_t post_index{};

        inline bool operator==(const Params &other) const {
          return deadline == other.deadline && post_index == other.post_index;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(DisputeWindowedPoSt::Params, deadline, post_index)

    struct PreCommitSectorBatch
        : ActorMethodBase<Method::kPreCommitSectorBatch> {
      struct Params {
        std::vector<SectorPreCommitInfo> sectors;

        inline bool operator==(const Params &other) const {
          return sectors == other.sectors;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(PreCommitSectorBatch::Params, sectors);

    struct ProveCommitAggregate
        : ActorMethodBase<Method::kProveCommitAggregate> {
      struct Params {
        RleBitset sectors;
        Bytes proof;

        inline bool operator==(const Params &other) const {
          return sectors == other.sectors && proof == other.proof;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(ProveCommitAggregate::Params, sectors, proof);

    struct ProveReplicaUpdates : ActorMethodBase<Method::kProveReplicaUpdates> {
      struct Params {
        std::vector<ReplicaUpdate> updates;

        inline bool operator==(const Params &other) const {
          return updates == other.updates;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };

      using Result = RleBitset;
    };
    CBOR_TUPLE(ProveReplicaUpdates::Params, updates)
  };

}  // namespace fc::vm::actor::builtin
