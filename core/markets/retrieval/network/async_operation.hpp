/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CPP_FILECOIN_MARKETS_RETRIEVAL_NETWORK_ASYNC_OPERATION_HPP
#define CPP_FILECOIN_MARKETS_RETRIEVAL_NETWORK_ASYNC_OPERATION_HPP

#include <functional>
#include <future>

#include "common/outcome.hpp"

namespace fc::markets::retrieval::network {
  /**
   * @class Execute async callback operations
   * @tparam Error - type of the error, which code can be assigned to the
   * operation promise
   */
  class AsyncOperation {
   public:
    using Operation = std::shared_ptr<std::promise<void>>;
    using Action = std::function<void(Operation)>;

    /**
     * @brief Run async operation and wait it's result
     * @param action - function, which will assign operation promise result
     * value or error
     * @return operation result
     */
    static outcome::result<void> run(Action action) {
      auto operation = std::make_shared<std::promise<void>>();
      action(operation);
      auto future = operation->get_future();
      try {
        future.get();
      } catch (std::system_error &error) {
        return outcome::failure(error.code());
      }
      return outcome::success();
    }

    /**
     * @brief Cancel async operation and return error
     * @param operation - async action
     * @param error - code of the operation error
     */
    template <typename Error,
              typename = std::enable_if_t<std::is_enum_v<Error>>>
    static void failure(Operation operation, Error error) {
      operation->set_exception(
          std::make_exception_ptr(std::system_error{make_error_code(error)}));
    }
  };
}  // namespace fc::markets::retrieval::network

#endif
