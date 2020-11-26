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

  PieceIOImpl::PieceIOImpl(std::shared_ptr<Ipld> ipld)
      : ipld_{std::move(ipld)} {}

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIOImpl::generatePieceCommitment(const RegisteredProof &registered_proof,
                                       const CID &payload_cid,
                                       const Selector &selector) {
    OUTCOME_TRY(selective_car,
                makeSelectiveCar(*ipld_, {{payload_cid, selector}}));
    return generatePieceCommitment(registered_proof, selective_car);
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIOImpl::generatePieceCommitment(const RegisteredProof &registered_proof,
                                       const Buffer &piece) {
    UnpaddedPieceSize padded_size = paddedSize(piece.size());

    int fds[2];
    if (pipe(fds) < 0) {
      return PieceIOError::kCannotCreatePipe;
    }
    std::vector<uint8_t> data = piece.toVector();
    data.resize(padded_size, 0);
    if (write(fds[1], data.data(), padded_size) == -1) {
      return PieceIOError::kCannotCreatePipe;
    }
    if (close(fds[1]) == -1) {
      return PieceIOError::kCannotClosePipe;
    }

    OUTCOME_TRY(commitment,
                Proofs::generatePieceCID(
                    registered_proof, PieceData{fds[0]}, padded_size));

    return {commitment, padded_size};
  }

  outcome::result<std::pair<CID, UnpaddedPieceSize>>
  PieceIOImpl::generatePieceCommitment(const RegisteredProof &registered_proof,
                                       const std::string &path) {
    if (!fs::exists(path)) {
      return PieceIOError::kFileNotExist;
    }
    uint64_t original_size = fs::file_size(path);
    UnpaddedPieceSize padded_size = paddedSize(original_size);

    auto copy_path = fs::temp_directory_path() / fs::unique_path();

    fs::copy_file(path, copy_path);

    auto _ = gsl::finally([&copy_path]() { fs::remove_all(copy_path); });

    fs::resize_file(copy_path, padded_size);

    OUTCOME_TRY(
        commitment,
        Proofs::generatePieceCID(
            registered_proof, PieceData(copy_path.string()), padded_size));

    return {commitment, padded_size};
  }

}  // namespace fc::markets::pieceio
