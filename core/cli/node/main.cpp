/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/client.hpp"
#include "cli/node/filplus.hpp"
#include "cli/node/net.hpp"
#include "cli/node/wallet.hpp"
#include "cli/run.hpp"

#define CMD(NAME, TYPE, DESC, ...) \
  { NAME, treeDesc<TYPE>(DESC, {__VA_ARGS__})() }
#define CMDS(NAME, TYPE, DESC) NAME, treeDesc<TYPE>(DESC)
#define GROUP(NAME, DESC) CMDS(NAME, Group, DESC)

namespace fc::cli::cli_node {
  const auto _tree {tree<Node>({
      {GROUP("net", "Manage P2P Network")({
          CMD("connect",
              Node_net_connect,
              "Connect to a peer",
              "peerMultiaddr|minerActorAddress"),
          CMD("listen", Node_net_listen, "List listen addresses"),
          CMD("peers", Node_net_peers, "Print peers"),
      })},
          {GROUP("filplus")({
              CMD("grant-datacap", Node_filplus_grantDatacap),
              CMD("list-notaries", Node_filplus_listNotaries),
              CMD("list-clients", Node_filplus_listClients),
              CMD("add-verifier", Node_filplus_addVerifier),
              CMD("check-client-datacap", Node_filplus_checkClientDataCap),
              CMD("check-notary-datacap", Node_filplus_checkNotaryDataCap),
          })},
          {GROUP("client")({
              CMD("retrieve", Node_client_retrieve),
              CMD("import", Node_client_importData),
              CMD("deal", Node_client_deal),
              CMD("generate-car", Node_client_generateCar),
              CMD("local", Node_client_local),
              CMD("find", Node_client_find),
              CMD("list-retrievals", Node_client_listRetrievals),
              CMD("inspect-deal", Node_client_inspectDeal),
              CMD("deal-stats", Node_client_dealStats),
              CMD("list-deals", Node_client_listDeals),
              CMD("balances", Node_client_balances),
              CMD("get-deal", Node_client_getDeal),
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
              CMD("import",
                  Node_wallet_import,
                  "Import keys",
                  {"path(optional)"}),
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
                  "Soft delete an address from the wallet - hard deletion "
                  "needed "
                  "for permanent removal",
                  "address"),
              {GROUP("market", "Interact with market balances")({
                  CMD("add",
                      Node_wallet_market_add,
                      "Add funds to the Storage Market Actor",
                      "amount"),
              })},
              CMD("version", Node_version, "Api version"),
          })};

}  // namespace fc::cli::cli_node

int main(int argc, const char *argv[]) {
      fc::cli::run("fuhon-node-cli", fc::cli::cli_node::_tree, argc, argv);
}
