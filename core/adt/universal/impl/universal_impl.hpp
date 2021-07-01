/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "adt/universal/universal.hpp"

#include "vm/runtime/runtime.hpp"

namespace fc::adt::universal {
  using vm::actor::ActorVersion;
  using vm::runtime::Runtime;

  template <typename T,
            typename Tv0,
            typename Tv2,
            typename Tv3,
            typename Tv4 = void>
  class UniversalImpl : public Universal<T> {
   public:
    UniversalImpl(Runtime &r, const CID &content) : runtime(r), cid(content) {}

    outcome::result<void> load() override {
      const auto version = runtime.getActorVersion();

      switch (version) {
        case ActorVersion::kVersion0: {
          OUTCOME_TRYA(ptr, loadFromIpld<Tv0>());
          break;
        }
        case ActorVersion::kVersion2: {
          OUTCOME_TRYA(ptr, loadFromIpld<Tv2>());
          break;
        }
        case ActorVersion::kVersion3: {
          OUTCOME_TRYA(ptr, loadFromIpld<Tv3>());
          break;
        }
        case ActorVersion::kVersion4: {
          if constexpr (std::is_same_v<Tv4, void>) {
            TODO_ACTORS_V4();
          } else {
            OUTCOME_TRYA(ptr, loadFromIpld<Tv4>());
          }
        }
      }

      return outcome::success();
    }

    std::shared_ptr<T> get() const override {
      return ptr;
    }

    outcome::result<CID> set() override {
      const auto version = runtime.getActorVersion();

      switch (version) {
        case ActorVersion::kVersion0: {
          OUTCOME_TRYA(cid, setToIpld<Tv0>());
          break;
        }
        case ActorVersion::kVersion2: {
          OUTCOME_TRYA(cid, setToIpld<Tv2>());
          break;
        }
        case ActorVersion::kVersion3: {
          OUTCOME_TRYA(cid, setToIpld<Tv3>());
          break;
        }
        case ActorVersion::kVersion4: {
          if constexpr (std::is_same_v<Tv4, void>) {
            TODO_ACTORS_V4();
          } else {
            OUTCOME_TRYA(cid, setToIpld<Tv4>());
          }
        }
      }

      return cid;
    }

    const CID &getCid() const override {
      return cid;
    }

   private:
    template <typename R>
    outcome::result<std::shared_ptr<R>> loadFromIpld() const {
      OUTCOME_TRY(object, runtime.getIpfsDatastore()->template getCbor<R>(cid));
      return std::make_shared<R>(object);
    }

    template <typename R>
    outcome::result<CID> setToIpld() const {
      const R &object = static_cast<const R &>(*ptr);
      OUTCOME_TRY(object_cid, runtime.getIpfsDatastore()->setCbor(object));
      return std::move(object_cid);
    }

    Runtime &runtime;
    CID cid;
    std::shared_ptr<T> ptr;
  };
}  // namespace fc::adt::universal
