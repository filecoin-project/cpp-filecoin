/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/metrics/instance_count.hpp>
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
      metric("ts_branches", o.ts_branches->size());
      auto height_head{o.ts_main->chain.rbegin()->first};
      ts_lock.unlock();

      metric("height_head", height_head);
      metric("height_attached",
             std::max(height_head, o.sync_job->metricAttachedHeight()));
      metric("height_known", std::max(height_head, height_known.load()));
      metric("height_expected",
             o.chain_epoch_clock->epochAtTime(o.utc_clock->nowUTC()).value());

      auto car{[&](auto _size, auto _count, auto _tmp, auto &ipld) {
        uint64_t size{}, count{}, tmp{};
        if (ipld) {
          std::shared_lock index_lock{ipld->index_mutex};
          std::shared_lock written_lock{ipld->written_mutex};
          size = ipld->car_offset;
          count = ipld->index->size();
          tmp = ipld->written.size();
        }
        metric(_size, size);
        metric(_count, count);
        metric(_tmp, tmp);
      }};
      {
        std::unique_lock ipld_lock{o.compacter->ipld_mutex};
        car("car_size", "car_count", "car_tmp", o.compacter->old_ipld);
        car("car2_size", "car2_count", "car2_tmp", o.compacter->new_ipld);
      }

      auto &instances{libp2p::metrics::instance::State::get()};
      std::unique_lock instances_lock{instances.mutex};
      for (auto &[type, count] : instances.counts) {
        std::stringstream name;
        name << "instances{type=\"" << type << "\"}";
        metric(name.str(), count);
      }
      instances_lock.unlock();

      return ss.str();
    }

    const NodeObjects &o;

    std::atomic_uint64_t height_known{};
    sync::events::Connection _possible_head;
  };
}  // namespace fc::node
