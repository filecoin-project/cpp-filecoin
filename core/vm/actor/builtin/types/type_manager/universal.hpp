/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld_cbor.hpp"

namespace fc::vm::actor::builtin::types {
  using vm::actor::ActorVersion;

  /**
   * Universal type is destined for general work with actor's types which are
   * different from version to version.
   *
   * To use this class, the T class must have the following hierarchy:
   * Base class: T
   * Inheritors: Tv0, Tv2, Tv3 ... TvN
   *
   * Where T is the common type with all fields of all versions. And Tv0 - TvN
   * are implementations of this type for each of actor's version with CBOR.
   */
  template <typename T>
  struct Universal : vm::actor::WithActorVersion {
    Universal() = default;
    explicit Universal(ActorVersion v) : WithActorVersion{v}, object{make(v)} {}
    Universal(ActorVersion v, std::shared_ptr<T> obj)
        : WithActorVersion{v}, object{obj} {}

    static std::shared_ptr<T> make(ActorVersion v);
    codec::cbor::CborDecodeStream &decode(codec::cbor::CborDecodeStream &s);
    codec::cbor::CborEncodeStream &encode(
        codec::cbor::CborEncodeStream &s) const;
    void load(const IpldPtr &ipld);
    outcome::result<void> flush();
    std::shared_ptr<T> copy_object() const;

    T *operator->() const {
      return object.get();
    }

    T &operator*() const {
      return *object;
    }

    Universal<T> copy() const {
      return Universal<T>{actor_version, copy_object()};
    }

    std::shared_ptr<T> object;
  };

  template <typename T>
  CBOR2_DECODE(Universal<T>) {
    v.actor_version = s.actor_version;
    v.object = v.make(v.actor_version);
    return v.decode(s);
  }

  template <typename T>
  CBOR2_ENCODE(Universal<T>) {
    return v.encode(s);
  }
}  // namespace fc::vm::actor::builtin::types

namespace fc::cbor_blake {
  template <typename T>
  struct CbLoadT<vm::actor::builtin::types::Universal<T>> {
    static void call(const IpldPtr &ipld,
                     vm::actor::builtin::types::Universal<T> &universal) {
      universal.load(ipld);
    }
  };

  template <typename T>
  struct CbFlushT<vm::actor::builtin::types::Universal<T>> {
    static outcome::result<void> call(
        vm::actor::builtin::types::Universal<T> &universal) {
      return universal.flush();
    }
  };
}  // namespace fc::cbor_blake
