/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio.hpp"

#include "primitives/cid/comm_cid.hpp"
#include "storage/car/car.hpp"

namespace fc::markets::pieceio {

  using fc::common::pieceCommitmentV1ToCID;
  using storage::car::makeSelectiveCar;

  PieceIO::PieceIO(std::shared_ptr<Ipld> ipld) : ipld_{std::move(ipld)} {}

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIO::generatePieceCommitment(
      // const RegisteredProof &registered_proof,
      const CID &payload_cid,
      const Selector &selector) {
    OUTCOME_TRY(selective_car,
                makeSelectiveCar(*ipld_, {{payload_cid, selector}}));
    UnpaddedPieceSize piece_size{selective_car.size()};

    CID commitment = pieceCommitmentV1ToCID(selective_car);

    return {commitment, piece_size};
  }

}  // namespace fc::markets::pieceio
