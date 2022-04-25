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
  const auto _tree{tree<Node>({
      {GROUP("net", "Manage P2P Network")({
          CMD("connect",
              Node_net_connect,
              "Connect to a peer",
              "peerMultiaddr|minerActorAddress"),
          CMD("listen", Node_net_listen, "List listen addresses"),
          CMD("peers", Node_net_peers, "Print peers"),
      })},
      {GROUP("filplus",
             "Interact with the verified registry actor used by Filplus")({
          CMD("grant-datacap",
              Node_filplus_grantDatacap,
              "Give allowance to the specified verified client address",
              "address",
              "allowance"),
          CMD("list-notaries", Node_filplus_listNotaries, "List all notaries"),
          CMD("list-clients",
              Node_filplus_listClients,
              "List all verified clients"),
          CMD("add-verifier", Node_filplus_addVerifier, "Add verifier address"),
          CMD("check-client-datacap",
              Node_filplus_checkClientDataCap,
              "Check verified client remaining bytes",
              "client address"),
          CMD("check-notary-datacap",
              Node_filplus_checkNotaryDataCap,
              "Check a notary's remaining bytes",
              "notary address"),
      })},
      {GROUP("client", "Make deals, store data, retrieve data")({
          CMD("retrieve",
              Node_client_retrieve,
              "Retrieve data from network",
              "data CID",
              "output path"),
          CMD("import", Node_client_import, "Import data", "input path"),
          CMD("deal",
              Node_client_deal,
              "Initialize storage deal with a miner",
              "dataCid",
              "miner",
              "price",
              "duration"),
          CMD("generate-car",
              Node_client_generateCar,
              "Generate a car file from input",
              "input path",
              "output path"),
          CMD("local", Node_client_local, "List locally imported data"),
          CMD("find", Node_client_find, "Find data in the network", "data CID"),
          CMD("list-retrievals",
              Node_client_listRetrievals,
              "List retrieval market deals"),
          CMD("inspect-deal",
              Node_client_inspectDeal,
              "Inspect detailed information about deal's lifecycle and the "
              "various stages it goes through"),
          CMD("deal-stats",
              Node_client_dealStats,
              "Print statistics about local storage deals"),
          CMD("list-deals", Node_client_listDeals, "List storage market deals"),
          CMD("balances",
              Node_client_balances,
              "Print storage market client balances"),
          CMD("get-deal",
              Node_client_getDeal,
              "Print detailed deal information",
              "proposal CID"),
      })},
      {GROUP("wallet", "Manage wallet")({
          CMD("new",
              Node_wallet_new,
              "Generate a new key of the given type",
              {"type(bls|secp256k1 (default: secp256k1))"}),
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
      })},
      CMD("version", Node_version, "Api version"),
  })};

}  // namespace fc::cli::cli_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::cli_node::_tree, argc, argv);
}
