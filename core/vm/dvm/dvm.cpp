/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/dvm/dvm.hpp"

#include <spdlog/sinks/basic_file_sink.h>

#include "codec/cbor/cbor_dump.hpp"
#include "vm/message/message.hpp"
#include "vm/runtime/runtime_types.hpp"
#include "vm/state/impl/state_tree_impl.hpp"

#define DEFINE(x) decltype(x) x

namespace fc::dvm {
  using primitives::address::Address;
  using vm::isVMExitCode;
  using vm::VMExitCode;
  using vm::actor::Actor;

  struct Formatter : public spdlog::formatter {
    void format(const spdlog::details::log_msg &m,
                spdlog::memory_buf_t &o) override {
      auto s{2 * indent};
      o.reserve(s + m.payload.size() + 1);
      o.resize(o.size() + s);
      memset(o.end() - s, ' ', s);
      if (auto v{m.payload.data()}) {
        o.append(v, v + m.payload.size());
      }
      o.push_back('\n');
    }
    std::unique_ptr<formatter> clone() const override {
      return std::make_unique<Formatter>();
    }
  };

  DEFINE(logger){[] {
    common::Logger logger;
    if (auto path{getenv("DVM_LOG")}) {
      logger = spdlog::basic_logger_mt("dvm", path, true);
      logger->set_formatter(std::make_unique<Formatter>());
    }
    return logger;
  }()};
  DEFINE(logging){false};
  DEFINE(indent){};

  void onCharge(GasAmount gas) {
    if (gas) {
      DVM_LOG("CHARGE {}", gas);
    }
  }

  void onIpldGet(const CID &cid, const BytesIn &data) {
    DVM_LOG("IPLD GET: {} {}", dumpCid(cid), dumpCbor(data));
  }

  void onIpldSet(const CID &cid, const BytesIn &data) {
    DVM_LOG("IPLD PUT: {} {}", dumpCid(cid), dumpCbor(data));
  }

  void onSend(const UnsignedMessage &msg) {
    DVM_LOG("SEND m={} n={} v={} to={} from={} {}",
            msg.method,
            msg.nonce,
            msg.value,
            msg.to,
            msg.from,
            dumpCbor(msg.params));
  }

  void onSendTo(const CID &code) {
    DVM_LOG("TO {}", *asActorCode(code));
  }

  void onReceipt(const outcome::result<InvocationOutput> &invocation_output,
                 const GasAmount &gas_used) {
    if (invocation_output.has_error()) {
      const auto &error{invocation_output.error()};
      if (isVMExitCode(error) && error != VMExitCode::kFatal) {
        dvm::onReceipt({VMExitCode{error.value()}, {}, gas_used});
      }
    } else {
      dvm::onReceipt({VMExitCode::kOk, invocation_output.value(), gas_used});
    }
  }

  void onReceipt(const MessageReceipt &receipt) {
    DVM_LOG("RECEIPT c={} g={} {}",
            (int64_t)receipt.exit_code,
            receipt.gas_used,
            dumpCbor(receipt.return_value));
  }

  void onActor(StateTree &tree, const Address &address, const Actor &actor) {
    if (logger && logging) {
      if (auto _old{tree.get(address)}) {
        auto &old{_old.value()};
        auto _b{old.balance != actor.balance}, _n{old.nonce != actor.nonce},
            _h{old.head != actor.head};
        if (_b || _n || _h) {
          auto code_multihash{old.code.content_address.getHash()};
          DVM_LOG(
              "ACTOR {} {}", address, common::span::bytestr(code_multihash));
          DVM_INDENT;
          if (_b) {
            DVM_LOG("balance {} -> {}", old.balance, actor.balance);
          }
          if (_n) {
            DVM_LOG("nonce {} -> {}", old.nonce, actor.nonce);
          }
          if (_h) {
            DVM_LOG("HEAD {} -> {}", dumpCid(old.head), dumpCid(actor.head));
            DVM_INDENT;
            if (auto _os{tree.getStore()->get(old.head)}) {
              DVM_LOG("{}", dumpCbor(_os.value()));
            } else {
              DVM_LOG("???");
            }
            OUTCOME_EXCEPT(ns, tree.getStore()->get(actor.head));
            DVM_LOG("{}", dumpCbor(ns));
          }
        }
      }
    }
  }
}  // namespace fc::dvm
