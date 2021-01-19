/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_impl.hpp"

#include <unistd.h>
#include <boost/filesystem.hpp>
#include "markets/pieceio/pieceio_error.hpp"
#include "proofs/proofs.hpp"
#include "storage/car/car.hpp"

namespace fc::markets::pieceio {
  namespace fs = boost::filesystem;
  using primitives::piece::paddedSize;
  using primitives::piece::PieceData;
  using proofs::Proofs;
  using storage::car::makeSelectiveCar;

  PieceIOImpl::PieceIOImpl(std::shared_ptr<Ipld> ipld, std::string temp_dir)
      : ipld_{std::move(ipld)}, temp_dir_{std::move(temp_dir)} {
    if (!fs::exists(temp_dir_)) {
      fs::create_directories(temp_dir_);
    }
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIOImpl::generatePieceCommitment(
      const RegisteredSealProof &registered_proof,
      const CID &payload_cid,
      const Selector &selector) {
    auto car_file = fs::path(temp_dir_) / fs::unique_path();
    auto _ = gsl::finally([&car_file]() {
      fs::remove_all(car_file);
    });  // or we can store it like cache
    OUTCOME_TRY(
        makeSelectiveCar(*ipld_, {{payload_cid, selector}}, car_file.string()));

    return generatePieceCommitment(registered_proof, car_file.string());
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIOImpl::generatePieceCommitment(
      const RegisteredSealProof &registered_proof, const std::string &path) {
    if (!fs::exists(path)) {
      return PieceIOError::kFileNotExist;
    }

    auto copy_path = fs::path(temp_dir_) / fs::unique_path();

    fs::copy_file(path, copy_path);

    auto _ = gsl::finally([&copy_path]() { fs::remove_all(copy_path); });

    auto padded_size{proofs::Proofs::padPiece(copy_path.string())};

    OUTCOME_TRY(
        commitment,
        Proofs::generatePieceCID(
            registered_proof, PieceData(copy_path.string()), padded_size));

    return {commitment, padded_size};
  }

}  // namespace fc::markets::pieceio
