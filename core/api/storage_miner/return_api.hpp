/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/storage_miner/storage_api.hpp"

#include "sector_storage/scheduler.hpp"

namespace fc::api {
  inline void makeReturnApi(const std::shared_ptr<StorageMinerApi> &api,
                            const std::shared_ptr<Scheduler> &scheduler) {
    api->ReturnAddPiece =
        wrapCb([=](const CallId &call_id,
                   const PieceInfo &piece_info,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {piece_info, call_error});
        });

    api->ReturnSealPreCommit1 = wrapCb([=](const CallId &call_id,
                                           const PreCommit1Output
                                               &precommit1_output,
                                           const boost::optional<CallError>
                                               &call_error) {
      return scheduler->returnResult(call_id, {precommit1_output, call_error});
    });

    api->ReturnSealPreCommit2 =
        wrapCb([=](const CallId &call_id,
                   const SectorCids &cids,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {cids, call_error});
        });

    api->ReturnSealCommit1 =
        wrapCb([=](const CallId &call_id,
                   const Commit1Output &commit1_out,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {commit1_out, call_error});
        });

    api->ReturnSealCommit2 =
        wrapCb([=](const CallId &call_id,
                   const Proof &proof,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {proof, call_error});
        });

    api->ReturnFinalizeSector =
        wrapCb([=](const CallId &call_id,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {{}, call_error});
        });

    api->ReturnMoveStorage =
        wrapCb([=](const CallId &call_id,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {{}, call_error});
        });

    api->ReturnUnsealPiece =
        wrapCb([=](const CallId &call_id,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {{}, call_error});
        });

    api->ReturnReadPiece =
        wrapCb([=](const CallId &call_id,
                   bool status,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {status, call_error});
        });

    api->ReturnFetch =
        wrapCb([=](const CallId &call_id,
                   const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {{}, call_error});
        });
  }
}  // namespace fc::api
