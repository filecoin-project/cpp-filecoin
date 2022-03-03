/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/net.hpp"
#include "cli/run.hpp"

#define CMD(NAME, TYPE) \
  { NAME, tree<TYPE>() }
#define CMDS(NAME, TYPE) NAME, tree<TYPE>
#define GROUP(NAME) CMDS(NAME, Group)

namespace fc::cli::_node {
  const auto _tree{tree<Node>({
      {GROUP("net")({
          CMD("connect", Node_net_connect),
          CMD("listen", Node_net_listen),
          CMD("peers", Node_net_peers),
      })},
      CMD("version", Node_version),
  })};
}  // namespace fc::cli::_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::_node::_tree, argc, argv);
}
