/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/remote_worker.hpp"

namespace fc::sector_storage {

  outcome::result<std::shared_ptr<RemoteWorker>>
  RemoteWorker::connectRemoteWorker(io_context &context,
                                    const std::shared_ptr<CommonApi> &api,
                                    const Multiaddress &address) {
    auto token = "stub";  // get token from common api

    struct make_unique_enabler : public RemoteWorker {
      make_unique_enabler(io_context &context) : RemoteWorker{context} {};
    };

    std::unique_ptr<RemoteWorker> r_worker =
        std::make_unique<make_unique_enabler>(context);

    r_worker->wsc_.setup(r_worker->api_);

    OUTCOME_TRY(r_worker->wsc_.connect(address, "/rpc/v0", token));

    return std::move(r_worker);
  }

  outcome::result<primitives::WorkerInfo> RemoteWorker::getInfo() {
    return api_.Info();
  }

  outcome::result<std::set<primitives::TaskType>>
  RemoteWorker::getSupportedTask() {
    return api_.TaskTypes();
  }

  outcome::result<std::vector<primitives::StoragePath>>
  RemoteWorker::getAccessiblePaths() {
    return api_.Paths();
  }

  RemoteWorker::RemoteWorker(io_context &context) : wsc_(context) {}

  outcome::result<CallId> RemoteWorker::addPiece(
      const SectorRef &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      PieceData piece_data) {
    return WorkerErrors::kUnsupportedCall;  // TODO(ortyomka): [FIL-344] add
                                            // functionality
  }

  outcome::result<CallId> RemoteWorker::sealPreCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    return api_.SealPreCommit1(
        sector, ticket, std::vector<PieceInfo>(pieces.begin(), pieces.end()));
  }

  outcome::result<CallId> RemoteWorker::sealPreCommit2(
      const SectorRef &sector, const PreCommit1Output &pre_commit_1_output) {
    return api_.SealPreCommit2(sector, pre_commit_1_output);
  }

  outcome::result<CallId> RemoteWorker::sealCommit1(
      const SectorRef &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids) {
    return api_.SealCommit1(
        sector,
        ticket,
        seed,
        std::vector<PieceInfo>(pieces.begin(), pieces.end()),
        cids);
  }

  outcome::result<CallId> RemoteWorker::sealCommit2(
      const SectorRef &sector, const Commit1Output &commit_1_output) {
    return api_.SealCommit2(sector, commit_1_output);
  }

  outcome::result<CallId> RemoteWorker::finalizeSector(
      const SectorRef &sector, const gsl::span<const Range> &keep_unsealed) {
    return api_.FinalizeSector(
        sector, std::vector<Range>(keep_unsealed.begin(), keep_unsealed.end()));
  }

  outcome::result<CallId> RemoteWorker::moveStorage(const SectorRef &sector,
                                                    SectorFileType types) {
    return api_.MoveStorage(sector, types);
  }

  outcome::result<CallId> RemoteWorker::unsealPiece(
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size,
      const SealRandomness &randomness,
      const CID &unsealed_cid) {
    return api_.UnsealPiece(sector, offset, size, randomness, unsealed_cid);
  }

  outcome::result<CallId> RemoteWorker::readPiece(
      PieceData output,
      const SectorRef &sector,
      UnpaddedByteIndex offset,
      const UnpaddedPieceSize &size) {
    return WorkerErrors::kUnsupportedCall;
  }

  outcome::result<CallId> RemoteWorker::fetch(const SectorRef &sector,
                                              const SectorFileType &file_type,
                                              PathType path_type,
                                              AcquireMode mode) {
    return api_.Fetch(sector, file_type, path_type, mode);
  }
}  // namespace fc::sector_storage
