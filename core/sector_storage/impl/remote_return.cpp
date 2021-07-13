/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sector_storage/impl/remote_return.hpp"

namespace fc::sector_storage {

  outcome::result<void> RemoteReturn::returnAddPiece(
      CallId call_id,
      boost::optional<PieceInfo> maybe_piece_info,
      boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnAddPiece(call_id, maybe_piece_info, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnSealPreCommit1(
      CallId call_id,
      boost::optional<PreCommit1Output> maybe_precommit1_out,
      boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnSealPreCommit1(
        call_id, maybe_precommit1_out, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnSealPreCommit2(
      CallId call_id,
      boost::optional<SectorCids> maybe_sector_cids,
      boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnSealPreCommit2(
        call_id, maybe_sector_cids, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnSealCommit1(
      CallId call_id,
      boost::optional<Commit1Output> maybe_commit1_out,
      boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnSealCommit1(
        call_id, maybe_commit1_out, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnSealCommit2(
      CallId call_id,
      boost::optional<Proof> maybe_proof,
      boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnSealCommit2(call_id, maybe_proof, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnFinalizeSector(
      CallId call_id, boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnFinalizeSector(call_id, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnMoveStorage(
      CallId call_id, boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnMoveStorage(call_id, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnUnsealPiece(
      CallId call_id, boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnUnsealPiece(call_id, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnReadPiece(
      CallId call_id,
      boost::optional<bool> maybe_status,
      boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnReadPiece(call_id, maybe_status, maybe_error);
  }

  outcome::result<void> RemoteReturn::returnFetch(
      CallId call_id, boost::optional<CallError> maybe_error) {
    return miner_api_->ReturnFetch(call_id, maybe_error);
  }

  RemoteReturn::RemoteReturn(std::shared_ptr<StorageMinerApi> miner_api)
      : miner_api_(std::move(miner_api)) {}
}  // namespace fc::sector_storage
