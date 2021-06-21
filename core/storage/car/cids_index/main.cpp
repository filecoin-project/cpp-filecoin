/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_dump.hpp"
#include "storage/car/cids_index/util.hpp"

int main(int argc, char **argv) {
  using namespace fc;
  if (argc > 1) {
    std::string car_path{argv[1]};
    // TODO(turuslan): max memory
    if (auto _ipld{storage::cids_index::loadOrCreateWithProgress(
            car_path, false, boost::none, nullptr, nullptr)}) {
      auto &ipld{_ipld.value()};
      if (argc > 2) {
        for (auto i{2}; i < argc; ++i) {
          std::string str{argv[i]};
          boost::optional<CID> cid;
          if (auto _key{CbCid::fromHex(str)}) {
            cid = CID{CbCid{_key.value()}};
          } else if (auto _cid{CID::fromString(str)}) {
            cid = _cid.value();
          } else {
            fmt::print("invalid cid \"{}\": {:#}\n", str, _cid.error());
          }
          if (cid) {
            if (auto _value{ipld->get(*cid)}) {
              auto &value{_value.value()};
              fmt::print(
                  "{} ({}): {} bytes\n", *cid, dumpCid(*cid), value.size());
              fmt::print("  hex {}\n", dumpBytes(value));
              fmt::print("  cbor {}\n", dumpCbor(value));
            } else {
              fmt::print("{} error: {:#}\n", *cid, _value.error());
            }
          }
        }
      }
    }
  } else {
    fmt::print("usage: {} CAR [CID]...\n", argv[0]);
  }
}
