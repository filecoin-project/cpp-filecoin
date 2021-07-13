/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/checks.hpp"

#include "crypto/randomness/randomness_types.hpp"
#include "primitives/sector/sector.hpp"
#include "proofs/impl/proof_engine_impl.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp"
#include "storage/ipfs/api_ipfs_datastore/api_ipfs_datastore_error.hpp"
#include "vm/actor/builtin/states/state_provider.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/miner/types.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/toolchain/toolchain.hpp"

namespace fc::mining::checks {
  using crypto::randomness::DomainSeparationTag;
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using primitives::sector::SealVerifyInfo;
  using sector_storage::zerocomm::getZeroPieceCommitment;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::VMExitCode;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::MethodParams;
  using vm::actor::builtin::states::StateProvider;
  using vm::actor::builtin::types::miner::kChainFinality;
  using vm::actor::builtin::types::miner::kPreCommitChallengeDelay;
  using vm::actor::builtin::types::miner::maxSealDuration;
  using vm::actor::builtin::types::miner::SectorPreCommitOnChainInfo;
  using vm::actor::builtin::v0::market::ComputeDataCommitment;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::message::UnsignedMessage;
  using vm::toolchain::Toolchain;

  outcome::result<EpochDuration> getMaxProveCommitDuration(
      NetworkVersion network, const std::shared_ptr<SectorInfo> &sector_info) {
    auto version{Toolchain::getActorVersionForNetwork(network)};
    switch (version) {
      case vm::actor::ActorVersion::kVersion0:
        return maxSealDuration(sector_info->sector_type);
      case vm::actor::ActorVersion::kVersion2:
        return vm::actor::builtin::types::miner::kMaxProveCommitDuration;
      // TODO (m.tagirov or a.chernyshov) change to v3
      case vm::actor::ActorVersion::kVersion3:
        return vm::actor::builtin::types::miner::kMaxProveCommitDuration;
      case vm::actor::ActorVersion::kVersion4:
        TODO_ACTORS_V4();
      case vm::actor::ActorVersion::kVersion5:
        TODO_ACTORS_V5();
    }
  }

  outcome::result<void> checkPieces(
      const Address &miner_address,
      const std::shared_ptr<SectorInfo> &sector_info,
      const std::shared_ptr<FullNodeApi> &api) {
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

      OUTCOME_TRY(proposal,
                  api->StateMarketStorageDeal(piece.deal_info->deal_id,
                                              chain_head->key));

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
    }

    return outcome::success();
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
    ComputeDataCommitment::Params params{
        .deals = deal_ids, .sector_type = sector_info->sector_type};
    OUTCOME_TRY(encoded_params, codec::cbor::encode(params));
    UnsignedMessage message{kStorageMarketAddress,
                            miner_address,
                            {},
                            BigInt{0},
                            kDefaultGasPrice,
                            kDefaultGasLimit,
                            ComputeDataCommitment::Number,
                            MethodParams{encoded_params}};
    OUTCOME_TRY(invocation_result, api->StateCall(message, tipset_key));
    if (invocation_result.receipt.exit_code != VMExitCode::kOk) {
      return ChecksError::kInvocationErrored;
    }
    return codec::cbor::decode<ComputeDataCommitment::Result>(
        invocation_result.receipt.return_value);
  }

  outcome::result<boost::optional<SectorPreCommitOnChainInfo>>
  getStateSectorPreCommitInfo(const Address &miner_address,
                              const std::shared_ptr<SectorInfo> &sector_info,
                              const TipsetKey &tipset_key,
                              const std::shared_ptr<FullNodeApi> &api) {
    boost::optional<SectorPreCommitOnChainInfo> result;

    OUTCOME_TRY(actor, api->StateGetActor(miner_address, tipset_key));
    auto ipfs = std::make_shared<ApiIpfsDatastore>(api);

    StateProvider provider(ipfs);
    OUTCOME_TRY(state, provider.getMinerActorState(actor));

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

    OUTCOME_TRY(network, api->StateNetworkVersion(tipset_key));
    OUTCOME_TRY(seal_duration, getMaxProveCommitDuration(network, sector_info));
    if (height - (sector_info->ticket_epoch + kChainFinality) > seal_duration) {
      return ChecksError::kExpiredTicket;
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

    OUTCOME_TRY(minfo, api->StateMinerInfo(miner_address, tipset_key));

    OUTCOME_TRY(verified,
                proofs->verifySeal(SealVerifyInfo{
                    .seal_proof = minfo.seal_proof_type,
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
    default:
      return "ChecksError: unknown error";
  }
}
