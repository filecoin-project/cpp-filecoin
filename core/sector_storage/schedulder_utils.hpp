/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/crypto/sha/sha256.hpp>
#include "api/rpc/json.hpp"
#include "codec/json/json.hpp"
#include "sector_storage/scheduler.hpp"

namespace fc::sector_storage {
  template <class... Types>
  outcome::result<WorkId> getWorkId(const TaskType &task_type,
                                    const std::tuple<Types...> &params) {
    OUTCOME_TRY(param, codec::json::format(api::encode(params)));
    return WorkId{
        .task_type = task_type,
        .param_hash = libp2p::crypto::sha256(param),
    };
  }
}  // namespace fc::sector_storage
