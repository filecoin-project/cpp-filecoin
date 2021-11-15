/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "paych/maker.hpp"

namespace fc::api {
  inline void fillPaychGet(
      const std::shared_ptr<FullNodeApi> &api,
      const std::shared_ptr<paych_maker::PaychMaker> &maker) {
    api->PaychGet = [=](auto &&cb,
                        const Address &from,
                        const Address &to,
                        const TokenAmount &amount) {
      maker->make({from, to}, amount, std::move(cb));
    };
  }
}  // namespace fc::api
