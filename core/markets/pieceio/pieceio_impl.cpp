/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/pieceio/pieceio_impl.hpp"

#include <unistd.h>
#include "markets/pieceio/pieceio_error.hpp"
#include "proofs/proofs.hpp"
#include "storage/car/car.hpp"

namespace fc::markets::pieceio {

  using primitives::piece::paddedSize;
  using primitives::piece::PieceData;
  using proofs::Proofs;
  using storage::car::makeSelectiveCar;

  PieceIOImpl::PieceIOImpl(std::shared_ptr<Ipld> ipld,
                           std::shared_ptr<PieceStorage> piece_storage)
      : ipld_{std::move(ipld)},
        piece_storage_{std::move(piece_storage)} {}

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
      return PieceIOError::CANNOT_CREATE_PIPE;
    }
    std::vector<uint8_t> data = piece.toVector();
    data.resize(padded_size, 0);
    if (write(fds[1], data.data(), padded_size) == -1) {
      return PieceIOError::CANNOT_CREATE_PIPE;
    }
    if (close(fds[1]) == -1) {
      return PieceIOError::CANNOT_CLOSE_PIPE;
    }

    OUTCOME_TRY(commitment,
                Proofs::generatePieceCID(
                    registered_proof, PieceData{fds[0]}, padded_size));

    return {commitment, padded_size};
  }

  outcome::result<std::pair<PiecePayloadLocation, CID>>
  PieceIOImpl::locatePiecePayload(const CID &payload_cid) const {
    if (ipld_->contains(payload_cid)) {
      return {PiecePayloadLocation::IPLD_STORE, payload_cid};
    }
    auto payload_location = piece_storage_->getPayloadLocation(payload_cid);
    if (payload_location.has_error()) {
      return PieceIOError::PAYLOAD_NOT_FOUND;
    }
    return {PiecePayloadLocation::SEALED_SECTOR,
            payload_location.value().parent_piece};
  }

  outcome::result<Buffer> PieceIOImpl::getPiecePayload(
      const CID &payload_cid) const {
    if (ipld_->contains(payload_cid)) {
      return ipld_->get(payload_cid);
    }
    OUTCOME_TRY(payload_location,
                piece_storage_->getPayloadLocation(payload_cid));
    OUTCOME_TRY(piece_info,
                piece_storage_->getPieceInfo(payload_location.parent_piece));
    std::ignore = piece_info;  // TODO: FIL-235 Implement sector unseal
    PieceData piece_data(0);
    const size_t payload_size = payload_location.block_location.block_size;
    std::vector<uint8_t> buffer(payload_size);
    size_t read_offset =
        piece_info.offset + payload_location.block_location.relative_offset;
    off_t seek_result = lseek(piece_data.getFd(), read_offset, SEEK_SET);
    if (seek_result < 0) {
      return PieceIOError::PAYLOAD_READ_ERROR;
    }

    size_t total_length{};
    while (total_length < payload_size) {
      ssize_t current_length = read(piece_data.getFd(),
                                    buffer.data() + total_length,
                                    payload_size - total_length);
      if (current_length < 0) {
        return PieceIOError::PAYLOAD_READ_ERROR;
      }
      total_length += current_length;
    }
    return Buffer{std::move(buffer)};
  }

}  // namespace fc::markets::pieceio
