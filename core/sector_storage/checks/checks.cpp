/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/checks/checks.hpp"
#include "sector_storage/zerocomm/zerocomm.hpp"

namespace fc::sector_storage::checks {
  using primitives::ChainEpoch;
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

}  // namespace fc::sector_storage::checks

OUTCOME_CPP_DEFINE_CATEGORY(fc::sector_storage::checks, ChecksError, e) {
  using E = fc::sector_storage::checks::ChecksError;
  switch (e) {
    case E::kInvalidDeal:
      return "ChecksError: invalid deal";
    case E::kExpiredDeal:
      return "ChecksError: expired deal";
    default:
      return "ChecksError: unknown error";
  }
}
