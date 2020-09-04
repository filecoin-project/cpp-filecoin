/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/checks/checks.hpp"
#include <storage/ipfs/api_ipfs_datastore/api_ipfs_datastore.hpp>
#include <storage/ipfs/api_ipfs_datastore/api_ipfs_datastore_error.hpp>
#include "sector_storage/zerocomm/zerocomm.hpp"

namespace fc::sector_storage::checks {
  using primitives::BigInt;
  using primitives::ChainEpoch;
  using primitives::DealId;
  using storage::ipfs::ApiIpfsDatastore;
  using vm::VMExitCode;
  using vm::actor::kStorageMarketAddress;
  using vm::actor::MethodParams;
  using vm::actor::builtin::market::ComputeDataCommitment;
  using vm::actor::builtin::miner::kChainFinalityish;
  using vm::actor::builtin::miner::maxSealDuration;
  using vm::actor::builtin::miner::MinerActorState;
  using vm::actor::builtin::miner::SectorPreCommitOnChainInfo;
  using vm::message::kDefaultGasLimit;
  using vm::message::kDefaultGasPrice;
  using vm::message::UnsignedMessage;
  using zerocomm::getZeroPieceCommitment;

  outcome::result<void> checkPieces(const SectorInfo &sector_info,
                                    const std::shared_ptr<Api> &api) {
    OUTCOME_TRY(chain_head, api->ChainHead());
    OUTCOME_TRY(tipset_key, chain_head.makeKey());

    for (const auto &piece : sector_info.pieces) {
      if (!piece.deal_info.has_value()) {
        OUTCOME_TRY(expected_cid,
                    getZeroPieceCommitment(piece.piece.size.unpadded()));
        if (piece.piece.cid != expected_cid) {
          return ChecksError::kInvalidDeal;
        }
        continue;
      }

      OUTCOME_TRY(
          proposal,
          api->StateMarketStorageDeal(piece.deal_info->deal_id, tipset_key));

      if (piece.piece.cid != proposal.proposal.piece_cid) {
        return ChecksError::kInvalidDeal;
      }

      if (piece.piece.size != proposal.proposal.piece_size) {
        return ChecksError::kInvalidDeal;
      }

      if (static_cast<ChainEpoch>(chain_head.height)
          >= proposal.proposal.start_epoch) {
        return ChecksError::kExpiredDeal;
      }
    }

    return outcome::success();
  }

  outcome::result<CID> getDataCommitment(const Address &miner_address,
                                         const SectorInfo &sector_info,
                                         const TipsetKey &tipset_key,
                                         const std::shared_ptr<Api> &api) {
    std::vector<DealId> deal_ids;
    for (const auto &piece : sector_info.pieces) {
      if (piece.deal_info.has_value()) {
        deal_ids.push_back(piece.deal_info->deal_id);
      }
    }
    ComputeDataCommitment::Params params{
        .deals = deal_ids, .sector_type = sector_info.sector_type};
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
                              const SectorInfo &sector_info,
                              const TipsetKey &tipset_key,
                              const std::shared_ptr<Api> &api) {
    boost::optional<SectorPreCommitOnChainInfo> result;

    OUTCOME_TRY(actor, api->StateGetActor(miner_address, tipset_key));
    ApiIpfsDatastore ipfs{api};
    OUTCOME_TRY(state, ipfs.getCbor<MinerActorState>(actor.head));
    OUTCOME_TRY(has, state.precommitted_sectors.has(sector_info.sector_number));
    if (has) {
      OUTCOME_TRYA(result,
                   state.precommitted_sectors.get(sector_info.sector_number));
    } else {
      OUTCOME_TRY(allocated_bitset, state.allocated_sectors.get());
      if (allocated_bitset.has(sector_info.sector_number)) {
        return ChecksError::kSectorAllocated;
      }
    }
    return result;
  }

  outcome::result<void> checkPrecommit(const Address &address,
                                       const SectorInfo &sector_info,
                                       const TipsetKey &tipset_key,
                                       const ChainEpoch &height,
                                       const std::shared_ptr<Api> &api) {
    OUTCOME_TRY(commD,
                getDataCommitment(address, sector_info, tipset_key, api));
    if (commD != sector_info.comm_d) {
      return ChecksError::kBadCommD;
    }

    OUTCOME_TRY(seal_duration, maxSealDuration(sector_info.sector_type));
    if (height - (sector_info.ticket_epoch + kChainFinalityish)
        > seal_duration) {
      return ChecksError::kExpiredTicket;
    }

    OUTCOME_TRY(
        state_sector_precommit_info,
        getStateSectorPreCommitInfo(address, sector_info, tipset_key, api));
    if (state_sector_precommit_info.has_value()) {
      if (state_sector_precommit_info.value().info.seal_epoch
          != sector_info.ticket_epoch) {
        return ChecksError::kBadTicketEpoch;
      }
      return ChecksError::kPrecommitOnChain;
    }

    return outcome::success();
  }

}  // namespace fc::sector_storage::checks

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::checks, ChecksError, e) {
  using E = fc::sector_storage::checks::ChecksError;
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
    default:
      return "ChecksError: unknown error";
  }
}
