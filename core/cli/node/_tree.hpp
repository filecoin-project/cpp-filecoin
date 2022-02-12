/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/net.hpp"
#include "cli/tree.hpp"

namespace fc::cli::_node {
  const auto _tree{tree<Node>({
      {"net",
       tree<Group>({
           {"listen", tree<Node_net_listen>()},
       })},
  })};
}  // namespace fc::cli::_node
