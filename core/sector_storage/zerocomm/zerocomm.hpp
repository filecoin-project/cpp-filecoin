/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/cid/cid.hpp"
#include "primitives/piece/piece.hpp"

namespace fc::sector_storage::zerocomm {
  using primitives::piece::UnpaddedPieceSize;

  outcome::result<CID> getZeroPieceCommitment(const UnpaddedPieceSize &size);
}  // namespace fc::sector_storage::zerocomm
