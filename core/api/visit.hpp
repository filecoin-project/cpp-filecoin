/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::api {

  template <typename A, typename F>
  void visit(A &&a, const F &f) {
    visit(a, a, f);
  }
}  // namespace fc::api
