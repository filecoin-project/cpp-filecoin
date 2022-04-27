/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/checks.hpp"
#include <primitives/sector/sector.hpp>

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/sector/sector.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore_error.hpp"
#include "vm/actor/builtin/methods/market.hpp"
#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::mining::checks {
  using crypto::randomness::DomainSeparationTag;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::sector::SealVerifyInfo;
  using primitives::sector::SectorId;
  using sector_storage::zerocomm::getZeroPieceCommitment;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::VMExitCode;
  using vm::actor::ActorVersion;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::MethodParams;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::actor::builtin::types::miner::kMaxPreCommitRandomnessLookback;
  using vm::actor::builtin::types::miner::kMaxProveCommitDuration;
  using vm::actor::builtin::types::miner::kPreCommitChallengeDelay;
  using vm::actor::builtin::types::miner::maxSealDuration;
  using vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo;
  using vm::message::UnsignedMessage;
  using vm::toolchain::Toolchain;
  namespace market = vm::actor::builtin::market;

  outcome::result<EpochDuration> getMaxProveCommitDuration(
      NetworkVersion network, const std::shared_ptr<SectorInfo> &sector_info) {
    const auto version{actorVersion(network)};
    switch (version) {
      case ActorVersion::kVersion0:
        return maxSealDuration(sector_info->sector_type);
      case ActorVersion::kVersion2:
      case ActorVersion::kVersion3:
      case ActorVersion::kVersion4:
        return kMaxProveCommitDuration;
      case ActorVersion::kVersion5:
      case ActorVersion::kVersion6:
      case ActorVersion::kVersion7:
        return sector_info->sector_type
                       >= api::RegisteredSealProof::kStackedDrg2KiBV1_1
                   ? 30 * kEpochsInDay + kPreCommitChallengeDelay
                   : kMaxProveCommitDuration;
    }
  }

  outcome::result<size_t> checkPieces(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const std::shared_ptr<FullNodeApi> &api) {
    size_t deal_count{0};
    OUTCOME_TRY(chain_head, api->ChainHead());

    for (const auto &piece : sector_info->pieces) {
      if (!piece.deal_info.has_value()) {
        OUTCOME_TRY(expected_cid,
                    getZeroPieceCommitment(piece.piece.size.unpadded()));
        if (piece.piece.cid != expected_cid) {
          return ChecksError::kInvalidDeal;
        }
        continue;
      }

      const auto maybe_proposal = api->StateMarketStorageDeal(
          piece.deal_info->deal_id, chain_head->key);
      if (maybe_proposal.has_error()) {
        return ChecksError::kInvalidDeal;
      }

      const auto &proposal{maybe_proposal.value()};
      if (miner_address != proposal.proposal.provider) {
        return ChecksError::kInvalidDeal;
      }

      if (piece.piece.cid != proposal.proposal.piece_cid) {
        return ChecksError::kInvalidDeal;
      }

      if (piece.piece.size != proposal.proposal.piece_size) {
        return ChecksError::kInvalidDeal;
      }

      if (chain_head->epoch() >= proposal.proposal.start_epoch) {
        return ChecksError::kExpiredDeal;
      }
      ++deal_count;
    }

    return deal_count;
  }

  outcome::result<CID> getDataCommitment(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const TipsetKey &tipset_key,
      const std::shared_ptr<FullNodeApi> &api) {
    std::vector<DealId> deal_ids;
    for (const auto &piece : sector_info->pieces) {
      if (piece.deal_info.has_value()) {
        deal_ids.push_back(piece.deal_info->deal_id);
      }
    }

    OUTCOME_TRY(params,
                codec::cbor::encode(market::ComputeDataCommitment::Params{
                    .inputs =
                        {
                            {
                                .deals = deal_ids,
                                .sector_type = sector_info->sector_type,
                            },
                        },
                }));

    UnsignedMessage message{kStorageMarketAddress,
                            miner_address,
                            {},
                            {},
                            {},
                            {},
                            market::ComputeDataCommitment::Number,
                            params};
    OUTCOME_TRY(invocation_result, api->StateCall(message, tipset_key));
    if (invocation_result.receipt.exit_code != VMExitCode::kOk) {
      return ChecksError::kInvocationErrored;
    }

    OUTCOME_TRY(res,
                codec::cbor::decode<market::ComputeDataCommitment::Result>(
                    invocation_result.receipt.return_value));

    if (res.commds.size() != 1) {
      return ERROR_TEXT("CommD output must have 1 entry");
    }

    return res.commds[0];
  }

  outcome::result<boost::optional<SectorPreCommitOnChainInfo>>
  getStateSectorPreCommitInfo(const Address &miner_address,
                              const std::shared_ptr<SectorInfo> &sector_info,
                              const TipsetKey &tipset_key,
                              const std::shared_ptr<FullNodeApi> &api) {
    boost::optional<SectorPreCommitOnChainInfo> result;

    OUTCOME_TRY(actor, api->StateGetActor(miner_address, tipset_key));
    auto ipfs = std::make_shared<ApiIpfsDatastore>(api);
    OUTCOME_TRY(network, api->StateNetworkVersion(tipset_key));
    ipfs->actor_version = actorVersion(network);

    OUTCOME_TRY(state, getCbor<MinerActorStatePtr>(ipfs, actor.head));

    OUTCOME_TRY(has,
                state->precommitted_sectors.has(sector_info->sector_number));
    if (has) {
      OUTCOME_TRYA(result,
                   state->precommitted_sectors.get(sector_info->sector_number));
    } else {
      OUTCOME_TRY(allocated_bitset, state->allocated_sectors.get());
      if (allocated_bitset.has(sector_info->sector_number)) {
        return ChecksError::kSectorAllocated;
      }
    }
    return result;
  }

  outcome::result<void> checkPrecommit(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const TipsetKey &tipset_key,
      const ChainEpoch &height,
      const std::shared_ptr<FullNodeApi> &api) {
    OUTCOME_TRY(checkPieces(miner_address, sector_info, api));

    OUTCOME_TRY(commD,
                getDataCommitment(miner_address, sector_info, tipset_key, api));
    if (commD != sector_info->comm_d) {
      return ChecksError::kBadCommD;
    }

    OUTCOME_TRY(state_sector_precommit_info,
                getStateSectorPreCommitInfo(
                    miner_address, sector_info, tipset_key, api));

    if (state_sector_precommit_info.has_value()) {
      if (state_sector_precommit_info->info.seal_epoch
          != sector_info->ticket_epoch) {
        return ChecksError::kBadTicketEpoch;
      }
      return ChecksError::kPrecommitOnChain;
    }

    const ChainEpoch ticket_earliest = height - kMaxPreCommitRandomnessLookback;

    if (sector_info->ticket_epoch < ticket_earliest) {
      return ChecksError::kExpiredTicket;
    }

    return outcome::success();
  }

  outcome::result<void> checkCommit(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const Proof &proof,
      const TipsetKey &tipset_key,
      const std::shared_ptr<FullNodeApi> &api,
      const std::shared_ptr<proofs::ProofEngine> &proofs) {
    if (sector_info->seed_epoch == 0) {
      return ChecksError::kBadSeed;
    }

    auto maybe_precomit_info = getStateSectorPreCommitInfo(
        miner_address, sector_info, tipset_key, api);

    if (maybe_precomit_info.has_error()) {
      if (maybe_precomit_info == outcome::failure(ChecksError::kSectorAllocated)
          && sector_info->message.has_value()) {
        return ChecksError::kCommitWaitFail;
      }

      return maybe_precomit_info.error();
    }

    auto state_sector_precommit_info = maybe_precomit_info.value();
    if (!state_sector_precommit_info.has_value()) {
      return ChecksError::kPrecommitNotFound;
    }

    if (state_sector_precommit_info->precommit_epoch + kPreCommitChallengeDelay
        != sector_info->seed_epoch) {
      return ChecksError::kBadSeed;
    }

    OUTCOME_TRY(miner_address_encoded, codec::cbor::encode(miner_address));
    OUTCOME_TRY(seed,
                api->ChainGetRandomnessFromBeacon(
                    tipset_key,
                    DomainSeparationTag::InteractiveSealChallengeSeed,
                    sector_info->seed_epoch,
                    miner_address_encoded));
    if (seed != sector_info->seed) {
      return ChecksError::kBadSeed;
    }

    if (sector_info->comm_r != state_sector_precommit_info->info.sealed_cid) {
      return ChecksError::kBadSealedCid;
    }

    OUTCOME_TRY(verified,
                proofs->verifySeal(SealVerifyInfo{
                    .seal_proof = sector_info->sector_type,
                    .sector = SectorId{.miner = miner_address.getId(),
                                       .sector = sector_info->sector_number},
                    .deals = {},
                    .randomness = sector_info->ticket,
                    .interactive_randomness = sector_info->seed,
                    .proof = proof,
                    .sealed_cid = state_sector_precommit_info->info.sealed_cid,
                    .unsealed_cid = sector_info->comm_d.get()}));
    if (!verified) {
      return ChecksError::kInvalidProof;
    }

    return outcome::success();
  }

  outcome::result<void> checkUpdate(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const TipsetKey &tipset_key,
      const std::shared_ptr<FullNodeApi> &api,
      const std::shared_ptr<proofs::ProofEngine> &proofs) {
    if (!sector_info->comm_r) {
      return ERROR_TEXT("checkUpdate: no comm_r");
    }
    if (!sector_info->update) {
      return ERROR_TEXT("checkUpdate: not marked for update");
    }
    OUTCOME_TRY(deal_count, checkPieces(miner_address, sector_info, api));
    if (deal_count == 0) {
      return ERROR_TEXT("checkUpdate: no deals");
    }

    if (!sector_info->update_unsealed) {
      return ChecksError::kBadUpdateReplica;
    }

    OUTCOME_TRY(comm_d,
                getDataCommitment(miner_address, sector_info, tipset_key, api));
    if (sector_info->update_unsealed != comm_d) {
      return ChecksError::kBadUpdateReplica;
    }
    if (!sector_info->update_sealed) {
      return ChecksError::kBadUpdateReplica;
    }
    if (!sector_info->update_proof) {
      return ChecksError::kBadUpdateProof;
    }
    OUTCOME_TRY(update_type,
                getRegisteredUpdateProof(sector_info->sector_type));
    OUTCOME_TRY(verified,
                proofs->verifyUpdateProof({
                    update_type,
                    *sector_info->comm_r,
                    *sector_info->update_sealed,
                    *sector_info->update_unsealed,
                    *sector_info->update_proof,
                }));
    if (!verified) {
      return ChecksError::kBadUpdateProof;
    }
    return outcome::success();
  }
}  // namespace fc::mining::checks

OUTCOME_CPP_DEFINE_CATEGORY(fc::mining::checks, ChecksError, e) {
  using E = fc::mining::checks::ChecksError;
  switch (e) {
    case E::kInvalidDeal:
      return "ChecksError: invalid deal";
    case E::kExpiredDeal:
      return "ChecksError: expired deal";
    case E::kInvocationErrored:
      return "ChecksError: invocation result has error";
    case E::kBadCommD:
      return "ChecksError: on chain CommD differs from sector";
    case E::kExpiredTicket:
      return "ChecksError: ticket has expired";
    case E::kBadTicketEpoch:
      return "ChecksError: bad ticket epoch";
    case E::kSectorAllocated:
      return "ChecksError: sector is allocated";
    case E::kPrecommitOnChain:
      return "ChecksError: precommit already on chain";
    case E::kBadSeed:
      return "ChecksError: seed epoch does not match";
    case E::kPrecommitNotFound:
      return "ChecksError: precommit info not found on-chain";
    case E::kBadSealedCid:
      return "ChecksError: on-chain sealed CID doesn't match";
    case E::kInvalidProof:
      return "ChecksError: invalid proof";
    case E::kCommitWaitFail:
      return "ChecksError: need to wait commit";
    case E::kMinerVersion:
      return "ChecksError::kMinerVersion";
    case E::kBadUpdateReplica:
      return "ChecksError: bad update replica";
    case E::kBadUpdateProof:
      return "ChecksError: bad update proof";
    default:
      return "ChecksError: unknown error";
  }
}
