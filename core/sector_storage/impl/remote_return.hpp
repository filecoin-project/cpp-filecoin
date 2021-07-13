/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sector_storage/worker.hpp"

#include "api/storage_miner/storage_api.hpp"

namespace fc::sector_storage {
  using api::StorageMinerApi;

  class RemoteReturn : public WorkerReturn {
   public:
    RemoteReturn(std::shared_ptr<StorageMinerApi> miner_api);

    outcome::result<void> returnAddPiece(
        CallId call_id,
        boost::optional<PieceInfo> maybe_piece_info,
        boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnSealPreCommit1(
        CallId call_id,
        boost::optional<PreCommit1Output> maybe_precommit1_out,
        boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnSealPreCommit2(
        CallId call_id,
        boost::optional<SectorCids> maybe_sector_cids,
        boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnSealCommit1(
        CallId call_id,
        boost::optional<Commit1Output> maybe_commit1_out,
        boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnSealCommit2(
        CallId call_id,
        boost::optional<Proof> maybe_proof,
        boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnFinalizeSector(
        CallId call_id, boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnMoveStorage(
        CallId call_id, boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnUnsealPiece(
        CallId call_id, boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnReadPiece(
        CallId call_id,
        boost::optional<bool> maybe_status,
        boost::optional<CallError> maybe_error) override;

    outcome::result<void> returnFetch(
        CallId call_id, boost::optional<CallError> maybe_error) override;

   private:
    std::shared_ptr<StorageMinerApi> miner_api_;
  };
}  // namespace fc::sector_storage
