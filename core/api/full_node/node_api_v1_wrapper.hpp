/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/utils.hpp"
#include "api/version.hpp"

namespace fc::api {

  /**
   * The difference between FullNodeApi V2.0.0 and the latest FullNodeApi
   * V1.2.0
   *
   * Note that changed API methods overrides the previous version with the same
   * name so types in new version may be changed.
   */
  struct FullNodeApiV1Wrapper {
    API_METHOD(Version, VersionResult)
  };

  std::shared_ptr<FullNodeApiV1Wrapper> makeFullNodeApiV1Wrapper();

  template <typename A, typename F>
  void visit(const FullNodeApiV1Wrapper &, A &&a, const F &f) {
    f(a.Version);
  }

}  // namespace fc::api
