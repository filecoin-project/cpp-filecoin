/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sstream>

#include "clock/chain_epoch_clock.hpp"
#include "clock/utc_clock.hpp"
#include "common/memory_usage.hpp"
#include "node/events.hpp"
#include "node/main/builder.hpp"
#include "node/sync_job.hpp"

namespace fc::node {
  struct Metrics {
    Metrics(const NodeObjects &o) : o{o} {
      _possible_head = o.events->subscribePossibleHead(
          [this](auto &e) { height_known = e.height; });
    }

    std::string prometheus() const {
      std::stringstream ss;
      auto metric{[&](auto &&name, auto &&value) {
        ss << name << ' ' << value << std::endl;
      }};

      auto [vm_size, vm_rss]{memoryUsage()};
      metric("vm_size", vm_size);
      metric("vm_rss", vm_rss);

      std::shared_lock ts_lock{*o.env_context.ts_branches_mutex};
      auto height_head{o.ts_main->chain.rbegin()->first};
      ts_lock.unlock();

      metric("height_head", height_head);
      metric("height_attached",
             std::max(height_head, o.sync_job->metricAttachedHeight()));
      metric("height_known", std::max(height_head, height_known.load()));
      metric("height_expected",
             o.chain_epoch_clock->epochAtTime(o.utc_clock->nowUTC()).value());

      metric("car_size", o.ipld_cids_write->car_offset);
      std::shared_lock index_lock{o.ipld_cids_write->index_mutex};
      metric("car_count", o.ipld_cids_write->index->size());
      index_lock.unlock();
      std::shared_lock written_lock{o.ipld_cids_write->written_mutex};
      metric("car_tmp", o.ipld_cids_write->written.size());
      written_lock.unlock();

      return ss.str();
    }

    const NodeObjects &o;

    std::atomic_uint64_t height_known{};
    sync::events::Connection _possible_head;
  };
}  // namespace fc::node
