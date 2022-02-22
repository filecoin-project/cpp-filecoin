/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

namespace fc::common {

  /**
   * Class to put move-only lambdas in std::function interface
   */
  template <typename T>
  class PutInFunction {
   public:
    explicit PutInFunction(T &&function)
        : move_function_{std::make_shared<T>(std::move(function))} {};

    template <typename... Args>
    auto operator()(Args &&...args) const {
      return move_function_->operator()(std::forward<Args>(args)...);
    }

   private:
    std::shared_ptr<T> move_function_;
  };
}  // namespace fc::common
