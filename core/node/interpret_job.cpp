/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "interpret_job.hpp"

#include "common/io_thread.hpp"
#include "common/logger.hpp"
#include "events.hpp"
#include "primitives/tipset/chain.hpp"
#include "vm/interpreter/impl/interpreter_impl.hpp"

namespace fc::sync {

  namespace {
    auto log() {
      static common::Logger logger = common::createLogger("interpret_job");
      return logger.get();
    }
  }  // namespace

  class InterpretJobImpl : public InterpretJob {
   public:
    InterpretJobImpl(std::shared_ptr<CachedInterpreter> interpreter,
                     const TsBranches &ts_branches,
                     IpldPtr ipld,
                     std::shared_ptr<events::Events> events)
        : interpreter_(std::move(interpreter)),
          ts_branches_(ts_branches),
          ipld_(std::move(ipld)),
          events_(std::move(events)) {}

    void add(TipsetCPtr head) override {
      thread.io->post([this, head] {
        log()->info("interpret {} {}",
                    head->height(),
                    fmt::join(head->key.cids(), " "));
        auto branch{find(ts_branches_, head)};
        auto result{interpreter_->interpret(branch.first, head)};
        if (!result) {
          log()->warn("interpret error {} {}",
                      result.error(),
                      result.error().message());
        }
        events_->signalHeadInterpreted({std::move(head), std::move(result)});
      });
    }

    std::shared_ptr<CachedInterpreter> interpreter_;
    const TsBranches &ts_branches_;
    IpldPtr ipld_;
    std::shared_ptr<events::Events> events_;
    IoThread thread;
  };

  std::shared_ptr<InterpretJob> InterpretJob::create(
      std::shared_ptr<CachedInterpreter> interpreter,
      const TsBranches &ts_branches,
      IpldPtr ipld,
      std::shared_ptr<events::Events> events) {
    return std::make_shared<InterpretJobImpl>(std::move(interpreter),
                                              ts_branches,
                                              std::move(ipld),
                                              std::move(events));
  }

}  // namespace fc::sync
