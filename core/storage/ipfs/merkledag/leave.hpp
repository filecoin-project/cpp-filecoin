/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAVE_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAVE_HPP

#include <functional>
#include <string_view>

#include "common/buffer.hpp"
#include "common/outcome.hpp"

namespace fc::storage::ipfs::merkledag {
  class Leave {
   public:
    /**
     * @brief Destructor
     */
    virtual ~Leave() = default;

    /**
     * @brief Get leave content
     * @return operation result
     */
    virtual const common::Buffer &content() const = 0;

    /**
     * @brief Get count of children leaves
     * @return Count of leaves
     */
    virtual size_t count() const = 0;

    /**
     * @brief Get children leave
     * @param name - leave name
     * @return operation result
     */
    virtual outcome::result<std::reference_wrapper<const Leave>> subLeave(
        std::string_view name) const = 0;

    /**
     * @brief Get names of all sub leaves of the current leave
     * @return operation result
     */
    virtual std::vector<std::string_view> getSubLeaveNames() const = 0;
  };

  /**
   * @enum Possible leave errors
   */
  enum class LeaveError : int { LEAVE_NOT_FOUND = 1, DUPLICATE_LEAVE };
}  // namespace fc::storage::ipfs::merkledag

OUTCOME_HPP_DECLARE_ERROR(fc::storage::ipfs::merkledag, LeaveError)

#endif
