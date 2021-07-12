/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/multi/multiaddress.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "common/buffer.hpp"
#include "common/libp2p/multi/cbor_multiaddress.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "vm/actor/actor_method.hpp"
#include "vm/actor/builtin/states/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using common::Buffer;
  using crypto::randomness::Randomness;
  using fc::primitives::sector::SectorInfo;
  using libp2p::multi::Multiaddress;
  using primitives::ChainEpoch;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::sector::PoStProof;
  using primitives::sector::Proof;
  using primitives::sector::RegisteredSealProof;
  using types::miner::CronEventPayload;
  using types::miner::SectorDeclaration;
  using types::miner::SectorPreCommitInfo;

  struct Construct : ActorMethodBase<1> {
    struct Params {
      Address owner;
      Address worker;
      std::vector<Address> control_addresses;
      RegisteredSealProof seal_proof_type;
      Buffer peer_id;
      std::vector<Multiaddress> multiaddresses;
    };
    ACTOR_METHOD_DECL();

    static outcome::result<void> makeEmptyState(
        const Runtime &runtime, states::MinerActorStatePtr state);
  };
  CBOR_TUPLE(Construct::Params,
             owner,
             worker,
             control_addresses,
             seal_proof_type,
             peer_id,
             multiaddresses)

  struct ControlAddresses : ActorMethodBase<2> {
    struct Result {
      Address owner;
      Address worker;
      std::vector<Address> control;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ControlAddresses::Result, owner, worker, control)

  /**
   * ChangeWorkerAddress will ALWAYS overwrite the existing control addresses
   * with the control addresses passed in the params. A worker change will be
   * scheduled if the worker passed in the params is different from the
   * existing worker.
   */
  struct ChangeWorkerAddress : ActorMethodBase<3> {
    struct Params {
      Address new_worker;
      std::vector<Address> new_control_addresses;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangeWorkerAddress::Params, new_worker, new_control_addresses)

  struct ChangePeerId : ActorMethodBase<4> {
    struct Params {
      Buffer new_id;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangePeerId::Params, new_id)

  /**
   * Invoked by miner's worker address to submit their fallback post
   */
  struct SubmitWindowedPoSt : ActorMethodBase<5> {
    /**
     * Information submitted by a miner to provide a Window PoSt
     */
    struct Params {
      struct PoStPartition {
        /// Partitions are numbered per-deadline, from zero
        uint64_t index{0};
        // Sectors skipped while proving that weren't already declared faulty
        RleBitset skipped;
      };

      /** The deadline index which the submission targets */
      uint64_t deadline{0};

      /** The partitions being proven */
      std::vector<PoStPartition> partitions;

      /**
       * Array of proofs, one per distinct registered proof type present in
       * the sectors being proven. In the usual case of a single proof type,
       * this array will always have a single element (independent of number
       * of partitions).
       */
      std::vector<PoStProof> proofs;

      /**
       * The epoch at which these proofs is being committed to a particular
       * chain.
       */
      ChainEpoch chain_commit_epoch;

      /**
       * The ticket randomness on the chain at the ChainCommitEpoch on the
       * chain this post is committed to
       */
      Randomness chain_commit_rand;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(SubmitWindowedPoSt::Params::PoStPartition, index, skipped)
  CBOR_TUPLE(SubmitWindowedPoSt::Params,
             deadline,
             partitions,
             proofs,
             chain_commit_epoch,
             chain_commit_rand)

  struct PreCommitSector : ActorMethodBase<6> {
    using Params = SectorPreCommitInfo;
    ACTOR_METHOD_DECL();
  };

  /**
   * Checks state of the corresponding sector pre-commitment, then schedules
   * the proof to be verified in bulk by the power actor. If valid, the power
   * actor will call ConfirmSectorProofsValid at the end of the same epoch as
   * this message
   */
  struct ProveCommitSector : ActorMethodBase<7> {
    struct Params {
      SectorNumber sector;
      Proof proof;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ProveCommitSector::Params, sector, proof)

  /**
   * Changes the expiration epoch for a sector to a new, later one. The sector
   * must not be terminated or faulty. The sector's power is recomputed for
   * the new expiration.
   */
  struct ExtendSectorExpiration : ActorMethodBase<8> {
    struct Params {
      struct ExpirationExtension {
        uint64_t deadline{0};
        uint64_t partition{0};
        RleBitset sectors;
        ChainEpoch new_expiration;
      };

      std::vector<ExpirationExtension> extensions;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ExtendSectorExpiration::Params::ExpirationExtension,
             deadline,
             partition,
             sectors,
             new_expiration)
  CBOR_TUPLE(ExtendSectorExpiration::Params, extensions)

  /**
   * Marks some sectors as terminated at the present epoch, earlier than their
   * scheduled termination, and adds these sectors to the early termination
   * queue. This method then processes up to AddressedSectorsMax sectors and
   * AddressedPartitionsMax partitions from the early termination queue,
   * terminating deals, paying fines, and returning pledge collateral. While
   * sectors remain in this queue:
   *
   *  1. The miner will be unable to withdraw funds.
   *  2. The chain will process up to AddressedSectorsMax sectors and
   *  AddressedPartitionsMax per epoch until the queue is empty.
   *
   * The sectors are immediately ignored for Window PoSt proofs, and should be
   * masked in the same way as faulty sectors. A miner terminating sectors in
   * the current deadline must be careful to compute an appropriate Window
   * PoSt proof for the sectors that will be active at the time the PoSt is
   * submitted.
   *
   * This function may be invoked with no new sectors to explicitly process
   * the next batch of sectors.
   */
  struct TerminateSectors : ActorMethodBase<9> {
    struct Params {
      std::vector<SectorDeclaration> terminations;
    };

    /**
     * Set to true if all early termination work has been completed. When
     * false, the miner may choose to repeatedly invoke TerminateSectors
     * with no new sectors to process the remainder of the pending
     * terminations. While pending terminations are outstanding, the miner
     * will not be able to withdraw funds.
     */
    struct Result {
      bool done;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(TerminateSectors::Params, terminations)
  CBOR_TUPLE(TerminateSectors::Result, done)

  struct DeclareFaults : ActorMethodBase<10> {
    struct Params {
      std::vector<SectorDeclaration> faults;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeclareFaults::Params, faults)

  struct DeclareFaultsRecovered : ActorMethodBase<11> {
    struct Params {
      std::vector<SectorDeclaration> recoveries;
    };

    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(DeclareFaultsRecovered::Params, recoveries)

  struct OnDeferredCronEvent : ActorMethodBase<12> {
    using Params = CronEventPayload;
    ACTOR_METHOD_DECL();
  };

  struct CheckSectorProven : ActorMethodBase<13> {
    struct Params {
      SectorNumber sector;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CheckSectorProven::Params, sector)

  struct AddLockedFund : ActorMethodBase<14> {
    using Params = TokenAmount;
    ACTOR_METHOD_DECL();
  };

  struct ReportConsensusFault : ActorMethodBase<15> {
    struct Params {
      Buffer block_header_1;
      Buffer block_header_2;
      Buffer block_header_extra;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ReportConsensusFault::Params,
             block_header_1,
             block_header_2,
             block_header_extra)

  struct WithdrawBalance : ActorMethodBase<16> {
    struct Params {
      TokenAmount amount;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(WithdrawBalance::Params, amount)

  struct ConfirmSectorProofsValid : ActorMethodBase<17> {
    struct Params {
      std::vector<SectorNumber> sectors;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ConfirmSectorProofsValid::Params, sectors)

  struct ChangeMultiaddresses : ActorMethodBase<18> {
    struct Params {
      std::vector<Multiaddress> new_multiaddresses;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(ChangeMultiaddresses::Params, new_multiaddresses)

  /**
   * Compacts a number of partitions at one deadline by removing terminated
   * sectors, re-ordering the remaining sectors, and assigning them to new
   * partitions so as to completely fill all but one partition with live
   * sectors. The addressed partitions are removed from the deadline, and new
   * ones appended. The final partition in the deadline is always included in
   * the compaction, whether or not explicitly requested. Removed sectors are
   * removed from state entirely. May not be invoked if the deadline has any
   * un-processed early terminations.
   */
  struct CompactPartitions : ActorMethodBase<19> {
    struct Params {
      uint64_t deadline{0};
      RleBitset partitions;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CompactPartitions::Params, deadline, partitions)

  /**
   * Compacts sector number allocations to reduce the size of the allocated
   * sector number bitfield.
   *
   * When allocating sector numbers sequentially, or in sequential groups,
   * this bitfield should remain fairly small. However, if the bitfield grows
   * large enough such that PreCommitSector fails (or becomes expensive), this
   * method can be called to mask out (throw away) entire ranges of unused
   * sector IDs. For example, if sectors 1-99 and 101-200 have been allocated,
   * sector number 99 can be masked out to collapse these two ranges into one.
   */
  struct CompactSectorNumbers : ActorMethodBase<20> {
    struct Params {
      RleBitset mask_sector_numbers;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(CompactSectorNumbers::Params, mask_sector_numbers)

  extern const ActorExports exports;

  /**
   * Collects and stores precommit messages to make
   * a packaged sending of a several messages within one transaction
   * which reduces the general amount of transactions in the network
   * with reduction of a gas fee for transactions.
   */

  struct PreCommitBatch : ActorMethodBase<25> {
    struct Params {
      std::vector<SectorPreCommitInfo> sectors;
    };
    ACTOR_METHOD_DECL();
  };
  CBOR_TUPLE(PreCommitBatch::Params, sectors);

}  // namespace fc::vm::actor::builtin::v0::miner
