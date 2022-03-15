/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/net.hpp"
#include "cli/node/wallet.hpp"
#include "cli/run.hpp"

#define CMD(NAME, TYPE) \
  { NAME, tree<TYPE>() }
#define CMDS(NAME, TYPE) NAME, tree<TYPE>
#define GROUP(NAME) CMDS(NAME, Group)

namespace fc::cli::cli_node {
  const auto _tree{tree<Node>({
      {GROUP("net")({
          CMD("connect", Node_net_connect),
          CMD("listen", Node_net_listen),
          CMD("peers", Node_net_peers),
      })},
      {GROUP("wallet")({
          CMD("new", Node_wallet_new),
          CMD("list", Node_wallet_list),
          CMD("add-balance", Node_wallet_addBalance),
          CMD("balance", Node_wallet_balance),
          CMD("default", Node_wallet_default),
          CMD("set-default", Node_wallet_setDefault),
          CMD("import", Node_wallet_import),
          CMD("sign", Node_wallet_sign),
          CMD("verify", Node_wallet_verify),
          CMD("delete", Node_wallet_delete),
          {GROUP("market")({
              CMD("add", Node_wallet_market_add),
          })},
      })},
      CMD("version", Node_version),
  })};

}  // namespace fc::cli::cli_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::cli_node::_tree, argc, argv);
}
