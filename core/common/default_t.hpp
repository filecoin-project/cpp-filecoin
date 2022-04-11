/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

namespace fc::common {
  /**
   * Default value for codec value instantiation
   * In case of non default constructible type instantiate default value with
   * any constant
   * @tparam T - constructible type
   */
  template <typename T>
  inline T kDefaultT() {
    return {};
  }
}
