/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/manager_impl.hpp"

namespace fc::sector_storage {

  outcome::result<std::vector<SectorId>> ManagerImpl::checkProvable(
      RegisteredProof seal_proof_type, gsl::span<const SectorId> sectors) {
    return outcome::success();
  }

  SectorSize ManagerImpl::getSectorSize() {
    return 0;
  }

  outcome::result<void> ManagerImpl::ReadPiece(const proofs::PieceData &output,
                                               const SectorId &sector,
                                               UnpaddedByteIndex offset,
                                               const UnpaddedPieceSize &size,
                                               const SealRandomness &randomness,
                                               const CID &cid) {
    return outcome::success();
  }

  outcome::result<std::vector<PoStProof>> ManagerImpl::generateWinningPoSt(
      ActorId miner_id,
      gsl::span<const SectorInfo> sector_info,
      const PoStRandomness &randomness) {
    return outcome::success();
  }

  outcome::result<Prover::WindowPoStResponse> ManagerImpl::generateWindowPoSt(
      ActorId miner_id,
      gsl::span<const SectorInfo> sector_info,
      const PoStRandomness &randomness) {
    return outcome::success();
  }

  outcome::result<PreCommit1Output> ManagerImpl::sealPreCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      gsl::span<const PieceInfo> pieces) {
    return outcome::success();
  }

  outcome::result<SectorCids> ManagerImpl::sealPreCommit2(
      const SectorId &sector, const PreCommit1Output &pre_commit_1_output) {
    return outcome::success();
  }

  outcome::result<Commit1Output> ManagerImpl::sealCommit1(
      const SectorId &sector,
      const SealRandomness &ticket,
      const InteractiveRandomness &seed,
      gsl::span<const PieceInfo> pieces,
      const SectorCids &cids) {
    return outcome::success();
  }

  outcome::result<Proof> ManagerImpl::sealCommit2(
      const SectorId &sector, const Commit1Output &commit_1_output) {
    return outcome::success();
  }

  outcome::result<void> ManagerImpl::finalizeSector(const SectorId &sector) {
    return outcome::success();
  }

  outcome::result<void> ManagerImpl::remove(const SectorId &sector) {
    return outcome::success();
  }

  outcome::result<PieceInfo> ManagerImpl::addPiece(
      const SectorId &sector,
      gsl::span<const UnpaddedPieceSize> piece_sizes,
      const UnpaddedPieceSize &new_piece_size,
      const proofs::PieceData &piece_data) {
    return outcome::success();
  }

  outcome::result<void> ManagerImpl::addLocalStorage(const std::string &path) {
    return outcome::success();
  }

  outcome::result<void> ManagerImpl::addWorker(const Worker &worker) {
    return outcome::success();
  }

  outcome::result<std::unordered_map<StorageID, std::string>>
  ManagerImpl::getStorageLocal() {
    return outcome::success();
  }

  outcome::result<FsStat> ManagerImpl::getFsStat(StorageID storage_id) {
    return outcome::success();
  }
}  // namespace fc::sector_storage
