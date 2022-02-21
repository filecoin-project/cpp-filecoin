/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cli/node/client.hpp"
#include "cli/node/net.hpp"
#include "cli/node/wallet.hpp"
#include "cli/tree.hpp"

namespace fc::cli::_node {
  const auto _tree{tree<Node>(
      {{"net",
        tree<Group>({
            {"listen", tree<Node_net_listen>()},
        })},
       {"client",
        tree<Group>({
            {"retrieve", tree<clientRetrive>()},
        })},
       {"wallet",
        tree<Group>({
            {"new", tree<walletNew>()},                 // TODO DONE)))
            {"list", tree<walletList>()},               // TODO DONE)))
            {"balance", tree<walletBalance>()},         // TODO DONE)))
            {"default", tree<walletDefault>()},         // TODO DONE)))
            {"set-default", tree<walletSetDefault>()},  // TODO DONE)))
            {"import", tree<walletImport>()},           // TODO 20% StateGetActor
            {"sign", tree<walletSign>()},               // TODO DONE)))
            {"verify", tree<walletVerify>()},           // TODO DONE)))
            {"delete",
             tree<walletDelete>()},  // TODO DONE))) (p.S api._->WalletDelete add)
            {"market",
             tree<Group>({
                 {"withdraw", tree<walletWithdraw>()},  // TODO 99% (p.S api._->MarketGetReserved add, api._->MarketWithdraw)
                 {"add", tree<walletAdd>()},            // TODO DONE))) (p.S api._->MarketAddBalance add)
             })},
        })}})};
}  // namespace fc::cli::_node
