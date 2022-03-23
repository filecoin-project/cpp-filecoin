/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/storage_miner/storage_api.hpp"

#include "sector_storage/scheduler.hpp"

namespace fc::api {
  inline void makeReturnApi(const std::shared_ptr<StorageMinerApi> &api,
                            const std::shared_ptr<Scheduler> &scheduler) {
    api->ReturnAddPiece = [=](const CallId &call_id,
                              const PieceInfo &piece_info,
                              const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {piece_info, call_error});
    };

    api->ReturnSealPreCommit1 = [=](const CallId &call_id,
                                    const PreCommit1Output &precommit1_output,
                                    const boost::optional<CallError>
                                        &call_error) {
      return scheduler->returnResult(call_id, {precommit1_output, call_error});
    };

    api->ReturnSealPreCommit2 =
        [=](const CallId &call_id,
            const SectorCids &cids,
            const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {cids, call_error});
        };

    api->ReturnSealCommit1 = [=](const CallId &call_id,
                                 const Commit1Output &commit1_out,
                                 const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {commit1_out, call_error});
    };

    api->ReturnSealCommit2 = [=](const CallId &call_id,
                                 const Proof &proof,
                                 const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {proof, call_error});
    };

    api->ReturnFinalizeSector =
        [=](const CallId &call_id,
            const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {{}, call_error});
        };

    api->ReturnMoveStorage = [=](const CallId &call_id,
                                 const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {{}, call_error});
    };

    api->ReturnUnsealPiece = [=](const CallId &call_id,
                                 const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {{}, call_error});
    };

    api->ReturnReadPiece = [=](const CallId &call_id,
                               bool status,
                               const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {status, call_error});
    };

    api->ReturnFetch = [=](const CallId &call_id,
                           const boost::optional<CallError> &call_error) {
      return scheduler->returnResult(call_id, {{}, call_error});
    };

    api->ReturnReplicaUpdate =
        [=](const CallId &call_id,
            SectorCids cids,
            const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {cids, call_error});
        };

    api->ReturnProveReplicaUpdate1 = [=](const CallId &call_id,
                                         Update1Output update_1_output,
                                         const boost::optional<CallError>
                                             &call_error) {
      return scheduler->returnResult(call_id, {update_1_output, call_error});
    };

    api->ReturnProveReplicaUpdate2 =
        [=](const CallId &call_id,
            Proof proof,
            const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {proof, call_error});
        };

    api->ReturnFinalizeReplicaUpdate =
        [=](const CallId &call_id,
            const boost::optional<CallError> &call_error) {
          return scheduler->returnResult(call_id, {{}, call_error});
        };
  }
}  // namespace fc::api
