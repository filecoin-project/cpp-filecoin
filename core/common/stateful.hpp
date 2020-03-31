/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_STATEFUL_HPP
#define CPP_FILECOIN_STATEFUL_HPP

namespace fc::common {
  template <class T>
  class Stateful {
   public:
    const T &getState() const {
      return state_;
    }

    void setState(const T &state) {
      state_ = state;
    }

   protected:
    T state_{};
  };
}  // namespace fc::common

#endif  // CPP_FILECOIN_STATEFUL_HPP
