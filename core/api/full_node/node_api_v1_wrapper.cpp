/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/full_node/node_api_v1_wrapper.hpp"

#include "const.hpp"

namespace fc::api {

  std::shared_ptr<FullNodeApiV1Wrapper> makeFullNodeApiV1Wrapper() {
    auto api = std::make_shared<FullNodeApiV1Wrapper>();
    api->Version = {[]() {
      return VersionResult{
          "fuhon", makeApiVersion(1, 3, 0), kEpochDurationSeconds};
    }};
    return api;
  }

}  // namespace fc::api
