/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/net.hpp"
#include "cli/node/wallet.hpp"
#include "cli/run.hpp"

#define CMD(NAME, TYPE, DESC, ...) \
  { NAME, treeDesc<TYPE>(DESC, {__VA_ARGS__})() }
#define CMDS(NAME, TYPE, DESC) NAME, treeDesc<TYPE>(DESC)
#define GROUP(NAME, DESC) CMDS(NAME, Group, DESC)

namespace fc::cli::cli_node {
  const auto _tree{tree<Node>({
      {GROUP("net", "Manage P2P Network")({
          CMD("connect",
              Node_net_connect,
              "Connect to a peer",
              "peerMultiaddr|minerActorAddress"),
          CMD("listen", Node_net_listen, "List listen addresses"),
          CMD("peers", Node_net_peers, "Print peers"),
      })},
      {GROUP("wallet", "Manage wallet")({
          CMD("new",
              Node_wallet_new,
              "Generate a new key of the given type",
              {"type(bls|secp256k1 (default secp256k1))"}),
          CMD("list", Node_wallet_list, "List wallet address"),
          CMD("add-balance", Node_wallet_addBalance, ""),
          CMD("balance",
              Node_wallet_balance,
              "Get account balance",
              {"address"}),
          CMD("default", Node_wallet_default, "Get default wallet address"),
          CMD("set-default",
              Node_wallet_setDefault,
              "Set default wallet address",
              {"address"}),
          CMD("import", Node_wallet_import, "Import keys", {"path(optional)"}),
          CMD("sign",
              Node_wallet_sign,
              "Sign a message",
              "signing address",
              "hex message"),
          CMD("verify",
              Node_wallet_verify,
              "Verify the signature of a message",
              "signing address",
              "hexMessage",
              "signature"),
          CMD("delete",
              Node_wallet_delete,
              "Soft delete an address from the wallet - hard deletion needed "
              "for permanent removal",
              "address"),
          {GROUP("market", "Interact with market balances")({
              CMD("add",
                  Node_wallet_market_add,
                  "Add funds to the Storage Market Actor",
                  "amount"),
          })},
      })},
      CMD("version", Node_version, "Api version"),
  })};

}  // namespace fc::cli::cli_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::cli_node::_tree, argc, argv);
}
