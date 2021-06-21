/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/chain.hpp"

#include <boost/endian/conversion.hpp>

#include "common/error_text.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/version/version.hpp"

namespace fc::primitives::tipset::chain {
  auto decodeHeight(BytesIn key) {
    assert(key.size() == sizeof(Height));
    return boost::endian::load_big_u64(key.data());
  }

  TipsetKey decodeTsk(BytesIn input) {
    std::vector<CbCid> cids;
    while (!input.empty()) {
      cids.push_back(
          CbCid{CbCid::fromSpan(input.subspan(0, CbCid::size())).value()});
      input = input.subspan(CbCid::size());
    }
    return cids;
  }

  auto encodeHeight(Height height) {
    return Buffer{}.putUint64(height);
  }

  auto encodeTsk(const TipsetKey &tsk) {
    Buffer out;
    for (auto &cid : tsk.cids()) {
      out.put(cid);
    }
    return out;
  }

  void attach(TsBranchPtr parent, TsBranchPtr child) {
    auto bottom{child->chain.begin()};
    parent->lazyLoad(bottom->first);
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
    OUTCOME_TRY(ts, ts_load->loadWithCacheInfo(key));
    parent->lazyLoad(ts.tipset->height());
    auto _parent{parent->chain.lower_bound(ts.tipset->height())};
    if (_parent == parent->chain.end()) {
      --_parent;
    }
    while (true) {
      const auto _bottom{
          chain.emplace(ts.tipset->height(), TsLazy{ts.tipset->key, ts.index})
              .first};
      while (_parent->first > _bottom->first) {
        parent->lazyLoad(_parent->first - 1);
        if (_parent == parent->chain.begin()) {
          return ERROR_TEXT("TsBranch::make: not connected");
        }
        --_parent;
      }
      if (*_parent == *_bottom) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->loadWithCacheInfo(ts.tipset->getParents()));
    }
    return make(std::move(chain), parent);
  }

  TsBranchPtr TsBranch::load(KvPtr kv, boost::optional<size_t> limit) {
    assert(!limit || *limit);
    TsChain chain;
    auto cur{kv->cursor()};
    for (cur->seekToLast(); cur->isValid() && (!limit || (*limit)--);
         cur->prev()) {
      chain.emplace(decodeHeight(cur->key()),
                    TsLazy{decodeTsk(cur->value()), 0});  // we don't know index
    }
    if (chain.empty()) {
      return nullptr;
    }
    auto branch{make(std::move(chain), nullptr)};
    cur->seekToFirst();
    assert(cur->isValid());
    branch->lazy.emplace(Lazy{
        {decodeHeight(cur->key()), TsLazy{decodeTsk(cur->value()), 0}},
        kv,
    });
    return branch;
  }

  outcome::result<TsBranchPtr> TsBranch::create(KvPtr kv,
                                                const TipsetKey &key,
                                                TsLoadPtr ts_load) {
    TsChain chain;
    auto batch{kv->batch()};
    OUTCOME_TRY(ts, ts_load->loadWithCacheInfo(key));
    while (true) {
      OUTCOME_TRY(batch->put(encodeHeight(ts.tipset->height()),
                             encodeTsk(ts.tipset->key)));
      chain.emplace(ts.tipset->height(), TsLazy{ts.tipset->key, ts.index});
      if (ts.tipset->height() == 0) {
        break;
      }
      OUTCOME_TRYA(ts, ts_load->loadWithCacheInfo(ts.tipset->getParents()));
    }
    OUTCOME_TRY(batch->commit());
    return make(std::move(chain), nullptr);
  }

  TsChain::value_type &TsBranch::bottom() {
    return lazy ? lazy->bottom : *chain.begin();
  }

  void TsBranch::lazyLoad(Height height) {
    auto bottom{chain.begin()->first};
    if (lazy && height >= lazy->bottom.first && height < bottom) {
      size_t batch{};
      auto cur{lazy->kv->cursor()};
      for (cur->seek(encodeHeight(bottom)); cur->isValid(); cur->prev()) {
        auto current{decodeHeight(cur->key())};
        chain.emplace(current, TsLazy{decodeTsk(cur->value()), {}});
        ++batch;
        if (current <= height && batch >= lazy->min_load) {
          break;
        }
      }
    }
  }

  outcome::result<Path> findPath(TsBranchPtr from, TsBranchIter to_it) {
    Path path;
    auto &[revert, apply]{path};
    auto &[to, _to]{to_it};
    apply.insert(*_to);
    while (to != from) {
      if (!to->parent) {
        return ERROR_TEXT("findPath: no path");
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
      branch->lazyLoad(ts->height());
      auto it{branch->chain.find(ts->height())};
      if (it != branch->chain.end() && it->second.key == ts->key) {
        while (branch->parent && it == branch->chain.begin()) {
          branch = branch->parent;
          branch->lazyLoad(ts->height());
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
    TsChain::value_type p{ts->height(),
                          TsLazy{ts->key, 0}};  // we don't know index
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
      return ERROR_TEXT("find: too high");
    }
    while (branch->bottom().first > height) {
      if (!branch->parent) {
        return ERROR_TEXT("find: too low");
      }
      branch = branch->parent;
    }
    branch->lazyLoad(height);
    auto it{branch->chain.lower_bound(height)};
    if (it->first > height && allow_less) {
      --it;
    }
    return std::make_pair(branch, it);
  }

  outcome::result<TsBranchIter> stepParent(TsBranchIter it) {
    auto &branch{it.first};
    branch->lazyLoad(it.second->first - 1);
    while (it.second == branch->chain.begin()) {
      if (!branch->parent) {
        return ERROR_TEXT("stepParent: error");
      }
      it.second = branch->parent->chain.find(it.second->first);
      branch = branch->parent;
      branch->lazyLoad(it.second->first - 1);
    }
    --it.second;
    return it;
  }

  outcome::result<BeaconEntry> latestBeacon(TsLoadPtr ts_load,
                                            TsBranchIter it) {
    // magic number from lotus
    for (auto i{0}; i < 20; ++i) {
      OUTCOME_TRY(ts, ts_load->lazyLoad(it.second->second));
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
                   ? vm::actor::builtin::types::miner::kChainFinality
                   : kWinningPoStSectorSetLookback));
    if (lookback < it.second->first) {
      return find(it.first, lookback);
    }
    return it;
  }

}  // namespace fc::primitives::tipset::chain
