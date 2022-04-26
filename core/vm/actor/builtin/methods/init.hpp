/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "vm/actor/actor_method.hpp"

namespace fc::vm::actor::builtin {

  // These methods must be actual with the last version of actors

  struct InitActor {
    enum class Method : MethodNumber {
      kConstruct = 1,
      kExec,
    }

    struct Construct : ActorMethodBase<Method::kConstruct> {
      struct Params {
        std::string network_name;

        inline bool operator==(const Params &other) const {
          return network_name == other.network_name;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(Construct::Params, network_name)

    struct Exec : ActorMethodBase<Method::kExec> {
      struct Params {
        CodeId code;
        MethodParams params;

        inline bool operator==(const Params &other) const {
          return code == other.code && params == other.params;
        }

        inline bool operator!=(const Params &other) const {
          return !(*this == other);
        }
      };

      struct Result {
        Address id_address;
        Address robust_address;

        inline bool operator==(const Result &other) const {
          return id_address == other.id_address
                 && robust_address == other.robust_address;
        }

        inline bool operator!=(const Result &other) const {
          return !(*this == other);
        }
      };
    };
    CBOR_TUPLE(Exec::Params, code, params)
    CBOR_TUPLE(Exec::Result, id_address, robust_address)
  };

}  // namespace fc::vm::actor::builtin
