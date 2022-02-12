/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/net.hpp"
#include "cli/tree.hpp"
#include "cli/node/client.hpp"

namespace fc::cli::_node {
  const auto _tree{tree<Node>({
      {"net",
       tree<Group>({
           {"listen", tree<Node_net_listen>()},
       })},
      {"client",
       tree<Group>({
                       {"retrieve", tree<clientRetrieve>()},
       })},
  })};
}  // namespace fc::cli::_node
