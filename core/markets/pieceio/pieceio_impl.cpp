/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_impl.hpp"

#include <boost/filesystem.hpp>
#include "markets/pieceio/pieceio_error.hpp"
#include "proofs/proofs.hpp"
#include "storage/car/car.hpp"

namespace fc::markets::pieceio {
  namespace fs = boost::filesystem;
  using primitives::piece::PieceData;
  using proofs::Proofs;

  PieceIOImpl::PieceIOImpl(const boost::filesystem::path &temp_dir)
      : temp_dir_{temp_dir} {
    if (!fs::exists(temp_dir_)) {
      fs::create_directories(temp_dir_);
    }
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIOImpl::generatePieceCommitment(
      const RegisteredSealProof &registered_proof,
      const boost::filesystem::path &path) {
    if (!fs::exists(path)) {
      return PieceIOError::kFileNotExist;
    }

    auto copy_path = temp_dir_ / fs::unique_path();

    fs::copy_file(path, copy_path);

    auto _ = gsl::finally([&copy_path]() { fs::remove_all(copy_path); });

    auto padded_size{proofs::Proofs::padPiece(copy_path)};

    OUTCOME_TRY(
        commitment,
        Proofs::generatePieceCID(
            registered_proof, PieceData(copy_path.string()), padded_size));

    return {commitment, padded_size};
  }

}  // namespace fc::markets::pieceio
