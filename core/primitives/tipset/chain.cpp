/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/chain.hpp"

#include <boost/endian/conversion.hpp>

#include "common/outcome2.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/version.hpp"

namespace fc::primitives::tipset::chain {
  using common::Hash256;

  auto decodeHeight(BytesIn key) {
    assert(key.size() == sizeof(Height));
    return boost::endian::load_big_u64(key.data());
  }

  TipsetKey decodeTsk(BytesIn input) {
    std::vector<CID> cids;
    while (!input.empty()) {
      cids.push_back(asCborBlakeCid(
          Hash256::fromSpan(input.subspan(0, Hash256::size())).value()));
      input = input.subspan(Hash256::size());
    }
    return cids;
  }

  auto encodeHeight(Height height) {
    return Buffer{}.putUint64(height);
  }

  auto encodeTsk(const TipsetKey &tsk) {
    Buffer out;
    for (auto &cid : tsk.cids()) {
      out.put(*asBlake(cid));
    }
    return out;
  }

  void attach(TsBranchPtr parent, TsBranchPtr child) {
    auto bottom{child->chain.begin()};
    assert(*parent->chain.find(bottom->first) == *bottom);
    ++bottom;
    [[maybe_unused]] auto _parent{parent->chain.find(bottom->first)};
    assert(_parent == parent->chain.end() || *_parent != *bottom);
    child->parent = parent;
    --bottom;
    parent->children.emplace(bottom->first, child);
    child->parent_key = boost::none;
  }

  void detach(TsBranchPtr parent, TsBranchPtr child) {
    assert(child->parent == parent);
    child->parent = nullptr;
    parent->children.erase(std::find_if(
        parent->children.begin(), parent->children.end(), [&](auto &p) {
          return ownerEq(parent, p.second);
        }));
  }

  TsBranchPtr TsBranch::make(TsChain chain, TsBranchPtr parent) {
    if (parent && chain.size() == 1) {
      return parent;
    }
    auto branch{std::make_shared<TsBranch>()};
    branch->chain = std::move(chain);
    if (parent) {
      attach(parent, branch);
    }
    return branch;
  }

  outcome::result<TsBranchPtr> TsBranch::make(TsLoadPtr ts_load,
                                              const TipsetKey &key,
                                              TsBranchPtr parent) {
    if (parent->chain.rbegin()->second.key == key) {
      return parent;
    }
    TsChain chain;
    OUTCOME_TRY(ts, ts_load->load(key));
    auto _parent{parent->chain.lower_bound(ts->height())};
    if (_parent == parent->chain.end()) {
      --_parent;
    }
    while (true) {
      auto _bottom{chain.emplace(ts->height(), TsLazy{ts->key, ts}).first};
      while (_parent->first > _bottom->first) {
        if (_parent == parent->chain.begin()) {
          return OutcomeError::kDefault;
        }
        --_parent;
      }
      if (*_parent == *_bottom) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->load(ts->getParents()));
    }
    return make(std::move(chain), parent);
  }

  TsBranchPtr TsBranch::load(KvPtr kv) {
    TsChain chain;
    auto cur{kv->cursor()};
    for (cur->seekToFirst(); cur->isValid(); cur->next()) {
      chain.emplace(decodeHeight(cur->key()), TsLazy{decodeTsk(cur->value())});
    }
    if (chain.empty()) {
      return nullptr;
    }
    return make(std::move(chain), nullptr);
  }

  outcome::result<TsBranchPtr> TsBranch::create(KvPtr kv,
                                                const TipsetKey &key,
                                                TsLoadPtr ts_load) {
    TsChain chain;
    auto batch{kv->batch()};
    OUTCOME_TRY(ts, ts_load->load(key));
    while (true) {
      OUTCOME_TRY(batch->put(encodeHeight(ts->height()), encodeTsk(ts->key)));
      chain.emplace(ts->height(), TsLazy{ts->key, ts});
      if (ts->height() == 0) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->load(ts->getParents()));
    }
    OUTCOME_TRY(batch->commit());
    return make(std::move(chain), nullptr);
  }

  outcome::result<Path> findPath(TsBranchPtr from, TsBranchIter to_it) {
    Path path;
    auto &[revert, apply]{path};
    auto &[to, _to]{to_it};
    apply.insert(*_to);
    while (to != from) {
      if (!to->parent) {
        return OutcomeError::kDefault;
      }
      auto bottom{to->chain.begin()};
      apply.insert(bottom, _to);
      _to = to->parent->chain.find(bottom->first);
      to = to->parent;
    }
    revert.insert(_to, from->chain.end());
    return path;
  }

  auto absorbChildren(TsBranchIter branch_it) {
    auto &[branch, _branch]{branch_it};
    std::vector<TsBranchPtr> removed;
    std::vector<std::pair<TsChain::iterator, TsBranchPtr>> queue;
    auto [begin, end]{branch->children.equal_range(_branch->first)};
    for (auto _child{begin}; _child != end;) {
      if (auto child{_child->second.lock()}) {
        queue.emplace_back(_branch, child);
      }
      _child = branch->children.erase(_child);
    }
    while (!queue.empty()) {
      auto [_branch, parent]{queue.back()};
      queue.pop_back();
      auto _parent{parent->chain.begin()};
      auto _child{parent->children.begin()};
      while (_parent != parent->chain.end() && _branch != branch->chain.end()
             && *_parent == *_branch) {
        while (_child != parent->children.end()
               && _child->first == _branch->first) {
          if (auto child{_child->second.lock()}) {
            queue.emplace_back(_branch, child);
            child->parent = branch;
          }
          _child = parent->children.erase(_child);
        }
        ++_parent;
        ++_branch;
      }
      --_parent;
      --_branch;
      parent->chain.erase(parent->chain.begin(), _parent);
      branch->children.emplace(_parent->first, parent);
      if (parent->chain.size() <= 1) {
        removed.push_back(parent);
      }
    }
    return removed;
  }

  outcome::result<std::vector<TsBranchPtr>> update(TsBranchPtr branch,
                                                   const Path &path,
                                                   KvPtr kv) {
    auto &from{branch->chain};
    auto &[revert, apply]{path};
    auto revert_to{from.find(revert.begin()->first)};

    assert(*revert.begin() == *revert_to);
    assert(*apply.begin() == *revert_to);
    assert(*revert.rbegin() == *from.rbegin());

    if (kv) {
      auto batch{kv->batch()};
      for (auto it{revert.rbegin()}; it != revert.rend(); ++it) {
        OUTCOME_TRY(batch->remove(encodeHeight(it->first)));
      }
      for (auto &[height, ts] : apply) {
        OUTCOME_TRY(batch->put(encodeHeight(height), encodeTsk(ts.key)));
      }
      OUTCOME_TRY(batch->commit());
    }

    for (auto _child{branch->children.lower_bound(revert_to->first + 1)};
         _child != branch->children.end();) {
      if (auto child{_child->second.lock()}) {
        child->chain.insert(revert_to, from.find(_child->first));
        ++_child;
      } else {
        _child = branch->children.erase(_child);
      }
    }

    from.erase(std::next(revert_to), from.end());
    from.insert(std::next(apply.begin()), apply.end());

    return absorbChildren({branch, revert_to});
  }

  outcome::result<std::pair<Path, std::vector<TsBranchPtr>>> update(
      TsBranchPtr branch, TsBranchIter to_it, KvPtr kv) {
    OUTCOME_TRY(path, findPath(branch, to_it));
    OUTCOME_TRY(removed, update(branch, path, kv));
    return std::make_pair(std::move(path), std::move(removed));
  }

  TsBranchIter find(const TsBranches &branches, TipsetCPtr ts) {
    for (auto branch : branches) {
      auto it{branch->chain.find(ts->height())};
      if (it != branch->chain.end() && it->second.key == ts->key) {
        while (branch->parent && it == branch->chain.begin()) {
          branch = branch->parent;
          it = branch->chain.find(ts->height());
        }
        return std::make_pair(branch, it);
      }
    }
    return {};
  }

  TsBranchIter insert(TsBranches &branches,
                      TipsetCPtr ts,
                      std::vector<TsBranchPtr> *children) {
    TsChain::value_type p{ts->height(), TsLazy{ts->key, ts}};
    auto [branch, it]{find(branches, ts)};
    for (auto &child : branches) {
      if (child->parent_key && child->parent_key == ts->key) {
        if (children) {
          children->push_back(child);
        }
        child->chain.emplace(p);
        if (branch) {
          attach(branch, child);
        } else {
          branch = child;
          it = child->chain.begin();
          child->parent_key = ts->getParents();
        }
      }
    }
    if (!branch) {
      branch = TsBranch::make({p}, nullptr);
      branch->parent_key = ts->getParents();
      it = branch->chain.begin();
      branches.insert(branch);
    }
    return std::make_pair(branch, it);
  }

  std::vector<TsBranchIter> children(TsBranchIter ts_it) {
    auto &[branch, it]{ts_it};
    std::vector<TsBranchIter> children;
    if (auto next{std::next(it)}; next != branch->chain.end()) {
      children.emplace_back(branch, next);
    }
    auto [begin, end]{branch->children.equal_range(it->first)};
    for (auto _child{begin}; _child != end;) {
      if (auto child{_child->second.lock()}) {
        if (auto next{std::next(child->chain.begin())};
            next != child->chain.end()) {
          children.emplace_back(child, next);
        }
        ++_child;
      } else {
        _child = branch->children.erase(_child);
      }
    }
    return children;
  }

  outcome::result<TsBranchIter> find(TsBranchPtr branch,
                                     Height height,
                                     bool allow_less) {
    if (height > branch->chain.rbegin()->first) {
      return OutcomeError::kDefault;
    }
    TsBranchPtr parent;
    while (branch->chain.begin()->first > height) {
      if (!branch->parent) {
        return OutcomeError::kDefault;
      }
      parent = branch;
      branch = branch->parent;
    }
    auto it{branch->chain.lower_bound(height)};
    if (it == branch->chain.end()) {
      if (!allow_less) {
        // example: find 3 in [[1, 2], [4, 5]]
        if (!parent) {
          return OutcomeError::kDefault;
        }
        return std::make_pair(parent, parent->chain.begin());
      }
      --it;
    }
    if (it->first > height && allow_less) {
      if (it == branch->chain.begin()) {
        return OutcomeError::kDefault;
      }
      --it;
    }
    return std::make_pair(branch, it);
  }

  outcome::result<TsBranchIter> stepParent(TsBranchIter it) {
    auto &branch{it.first};
    if (it.second == branch->chain.begin()) {
      if (!branch->parent) {
        return OutcomeError::kDefault;
      }
      it.second = branch->parent->chain.find(it.second->first);
      branch = branch->parent;
    }
    --it.second;
    return it;
  }

  outcome::result<BeaconEntry> latestBeacon(TsLoadPtr ts_load,
                                            TsBranchIter it) {
    // magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      OUTCOME_TRY(ts, ts_load->loadw(it.second->second));
      auto &beacons{ts->blks[0].beacon_entries};
      if (!beacons.empty()) {
        return *beacons.rbegin();
      }
      if (it.second->first == 0) {
        break;
      }
      OUTCOME_TRYA(it, stepParent(it));
    }
    return TipsetError::kNoBeacons;
  }

  outcome::result<TsBranchIter> getLookbackTipSetForRound(TsBranchIter it,
                                                          ChainEpoch epoch) {
    static constexpr ChainEpoch kWinningPoStSectorSetLookback{10};
    const Height lookback = std::max<ChainEpoch>(
        0,
        epoch
            - (vm::version::getNetworkVersion(epoch)
                       > vm::version::NetworkVersion::kVersion3
                   ? vm::actor::builtin::types::miner::kChainFinalityish
                   : kWinningPoStSectorSetLookback));
    if (lookback < it.second->first) {
      return find(it.first, lookback);
    }
    return it;
  }

}  // namespace fc::primitives::tipset::chain
