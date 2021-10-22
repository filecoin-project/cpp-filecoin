/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "cached_interpreter.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::vm::interpreter {

  InterpreterCache::InterpreterCache(std::shared_ptr<PersistentBufferMap> kv,
                                     std::shared_ptr<CbIpld> ipld)
      : kv{std::move(kv)}, ipld_{std::move(ipld)} {}

  boost::optional<outcome::result<Result>> InterpreterCache::tryGet(
      const TipsetKey &key) const {
    boost::optional<outcome::result<Result>> result;
    const Bytes hash{copy(key.hash())};
    if (kv->contains(hash)) {
      const auto raw{kv->get(hash).value()};
      if (auto cached{
              codec::cbor::decode<boost::optional<Result>>(raw).value()}) {
        // check if interpreted state root is still valid (can be deleted during
        // compaction)
        if (ipld_->has(*asBlake(cached->state_root))) {
          result.emplace(std::move(*cached));
        }
      } else {
        result.emplace(InterpreterError::kTipsetMarkedBad);
      }
    }
    return result;
  }

  outcome::result<Result> InterpreterCache::get(const TipsetKey &key) const {
    if (auto cached{tryGet(key)}) {
      return *cached;
    }
    return InterpreterError::kNotCached;
  }

  void InterpreterCache::set(const TipsetKey &key, const Result &result) {
    kv->put(copy(key.hash()), codec::cbor::encode(result).value()).value();
  }

  void InterpreterCache::markBad(const TipsetKey &key) {
    kv->put(copy(key.hash()), BytesIn{codec::cbor::kNull}).value();
  }

  void InterpreterCache::remove(const TipsetKey &key) {
    kv->remove(copy(key.hash())).value();
  }

  CachedInterpreter::CachedInterpreter(std::shared_ptr<Interpreter> interpreter,
                                       std::shared_ptr<InterpreterCache> cache)
      : interpreter{std::move(interpreter)}, cache{std::move(cache)} {}

  outcome::result<Result> CachedInterpreter::interpret(
      TsBranchPtr ts_branch, const TipsetCPtr &tipset) const {
    const auto key{tipset->key};
    if (auto cached{cache->tryGet(key)}) {
      return *cached;
    }
    auto result = interpreter->interpret(ts_branch, tipset);
    if (!result) {
      cache->markBad(key);
    } else {
      cache->set(key, result.value());
    }
    return result;
  }
}  // namespace fc::vm::interpreter
