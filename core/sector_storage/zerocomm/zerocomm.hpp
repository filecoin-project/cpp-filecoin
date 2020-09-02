/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_SECTOR_STORAGE_ZEROCOMM_ZEROCOMM_HPP
#define CPP_FILECOIN_CORE_SECTOR_STORAGE_ZEROCOMM_ZEROCOMM_HPP

#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"

namespace fc::sector_storage::zerocomm {
  using primitives::piece::UnpaddedPieceSize;

  outcome::result<CID> getZeroPieceCommitment(const UnpaddedPieceSize &size);
}  // namespace fc::sector_storage::zerocomm

#endif  // CPP_FILECOIN_CORE_SECTOR_STORAGE_ZEROCOMM_ZEROCOMM_HPP
