/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace fc::common::git_commit_version {

  /**
   * Returns string containing git most recent tag and commit hash.
   */
  const std::string &getGitPrettyVersion();
}  // namespace fc::common::git_commit_version
