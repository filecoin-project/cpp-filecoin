/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/git_commit_version/git_commit_version.hpp"

namespace fc::node {
  using common::git_commit_version::getGitPrettyVersion;

  const std::string kNodeVersion = "Fuhon Node " + getGitPrettyVersion();
}  // namespace fc::node
