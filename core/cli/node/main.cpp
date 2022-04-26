/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cli/node/client.hpp"
#include "cli/node/developer.hpp"
#include "cli/node/filplus.hpp"
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
          CMD("grant-datacap", Node_client_grantDatacap),
          CMD("check-client-datacap", Node_client_checkClientDataCap),
          CMD("list-notaries", Node_client_listNotaries),
          CMD("list-clients", Node_client_listClients),
          CMD("check-notary-datacap", Node_client_checkNotaryDataCap),
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
      {GROUP("mpool")({
          CMD("pending", Node_developer_pending),
          CMD("subscribe", Node_developer_sub),
          CMD("find", Node_developer_find),
      })},
      {GROUP("auth")({
          CMD("create-token", Node_developer_createToken),
          CMD("api-info", Node_developer_apiInfo),
      })},
      {GROUP("chain")({
          CMD("head", Node_developer_head),
          CMD("get-block", Node_developer_getBlock),
          CMD("read-object", Node_developer_readObject),
          CMD("get-message", Node_developer_getMessage),
          CMD("get", Node_developer_get),
          CMD("slash-consensus", Node_developer_slashConsensus),
          CMD("gas-price", Node_developer_estimateGasPrices),
      })},
      {GROUP("state")({
          CMD("miner-info", Node_developer_stateMinerInfo),
          CMD("network-version", Node_developer_networkVersion),
          {GROUP("market")({
              CMD("balance", Node_developer_balance),
          })},
          CMD("sector", Node_developer_sector),
          CMD("call", Node_developer_call),
          CMD("search-msg", Node_developer_searchMsg),
          CMD("wait-msg", Node_developer_waitMsg),
          CMD("sector-size", Node_developer_sectorSize),
          CMD("lookup", Node_developer_lookup),
          CMD("get-actor", Node_developer_getActor),
          CMD("list-actors", Node_developer_listActors),
          CMD("list-miners", Node_developer_listMiners),
          CMD("get-deal", Node_developer_getDeal),
          CMD("active-sectors", Node_developer_activeSectors),
          CMD("sectors", Node_developer_sectors),
          CMD("power", Node_developer_power),
      })},
      CMD("version", Node_version),
  })};

}  // namespace fc::cli::cli_node

int main(int argc, const char *argv[]) {
  fc::cli::run("fuhon-node-cli", fc::cli::cli_node::_tree, argc, argv);
}
