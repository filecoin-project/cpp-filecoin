/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/logger.hpp"
#include "fwd.hpp"
#include "primitives/types.hpp"
#include "vm/runtime/runtime_types.hpp"

// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CAT1(a, b) _CAT2(a, b)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CAT2(a, b) _CAT3(~, a##b)
// NOLINTNEXTLINE(bugprone-reserved-identifier)
#define _CAT3(a, b) b

#define DVM_INDENT fc::dvm::Indent _CAT1(dvmd, __COUNTER__)
#define DVM_LOG(...)                         \
  if (fc::dvm::logger && fc::dvm::logging) { \
    fc::dvm::logger->info(__VA_ARGS__);      \
  }

namespace fc::dvm {
  using primitives::GasAmount;
  using primitives::address::Address;
  using vm::actor::Actor;
  using vm::message::UnsignedMessage;
  using vm::runtime::InvocationOutput;
  using vm::runtime::MessageReceipt;
  using vm::state::StateTree;

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern common::Logger logger;
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
  extern bool logging;

  struct Indent {
    inline Indent() {
      ++indent_;
    }
    Indent(const Indent &) = delete;
    Indent(Indent &&) = delete;

    inline ~Indent() {
      --indent_;
    }

    Indent &operator=(const Indent &) = delete;
    Indent &operator=(Indent &&) = delete;

    inline static size_t indent() {
      return indent_;
    }

   private:
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    static size_t indent_;
  };

  void onIpldGet(const CID &cid, const BytesIn &data);
  void onIpldSet(const CID &cid, const BytesIn &data);
  void onCharge(GasAmount gas);
  void onSend(const UnsignedMessage &msg);
  void onSendTo(const CID &code);
  void onReceipt(const outcome::result<InvocationOutput> &invocation_output,
                 const GasAmount &gas_used);
  void onReceipt(const MessageReceipt &receipt);
  void onActor(StateTree &tree, const Address &address, const Actor &actor);
}  // namespace fc::dvm
