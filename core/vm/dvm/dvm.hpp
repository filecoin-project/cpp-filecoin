/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "primitives/types.hpp"
#include "primitives/cid/cid.hpp"

#define _CAT1(a, b) _CAT2(a, b)
#define _CAT2(a, b) _CAT3(~, a##b)
#define _CAT3(a, b) b

#define DVM_INDENT fc::dvm::Indent _CAT1(dvmd, __COUNTER__)
#define DVM_LOG(...)                         \
  if (fc::dvm::logger && fc::dvm::logging) { \
    fc::dvm::logger->info(__VA_ARGS__);      \
  }

namespace fc::vm {
  namespace runtime { struct MessageReceipt; }
  namespace state { class StateTree; }
  namespace actor { struct Actor; }
  namespace message { struct UnsignedMessage; }
}

namespace fc::primitives::address {
  struct Address;
}

namespace fc::dvm {
  using common::Buffer;
  using primitives::GasAmount;
  using primitives::address::Address;
  using vm::actor::Actor;
  using vm::message::UnsignedMessage;
  using vm::runtime::MessageReceipt;
  using vm::state::StateTree;

  extern common::Logger logger;
  extern bool logging;
  extern size_t indent;

  struct Indent {
    inline Indent() {
      ++indent;
    }
    inline ~Indent() {
      --indent;
    }
  };

  void onIpldSet(const CID &cid, const Buffer &data);
  void onCharge(GasAmount gas);
  void onSend(const UnsignedMessage &msg);
  void onReceipt(const MessageReceipt &receipt);
  void onActor(StateTree &tree, const Address &address, const Actor &actor);
}  // namespace fc::dvm
