/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blocksync_common.hpp"

#include "storage/ipfs/datastore.hpp"

namespace fc::sync::blocksync {

  namespace {

    outcome::result<void> storeBlock(
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
        BlockHeader header,
        const std::vector<CID> &secp_cids,
        const std::vector<uint64_t> &secp_includes,
        const std::vector<CID> &bls_cids,
        const std::vector<uint64_t> &bls_includes,
        bool store_messages,
        const OnBlockStored &callback) {
      BlockMsg m{std::move(header), {}, {}};

      if (store_messages) {
        MsgMeta meta;
        ipld->load(meta);

        m.secp_messages.reserve(secp_includes.size());
        for (auto idx : secp_includes) {
          m.secp_messages.push_back(secp_cids[idx]);
          OUTCOME_TRY(meta.secp_messages.append(secp_cids[idx]));
        }

        m.bls_messages.reserve(bls_includes.size());
        for (auto idx : bls_includes) {
          m.bls_messages.push_back(bls_cids[idx]);
          OUTCOME_TRY(meta.bls_messages.append(bls_cids[idx]));
        }

        OUTCOME_TRY(meta_cid, ipld->setCbor<MsgMeta>(meta));

        if (meta_cid != m.header.messages) {
          return Error::SYNC_MSG_LOAD_FAILURE;
        }
      }

      OUTCOME_TRY(block_cid, ipld->setCbor<BlockHeader>(m.header));

      callback(std::move(block_cid), std::move(m));
      return outcome::success();
    }

    outcome::result<void> storeTipsetBundle(
        const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
        TipsetBundle &bundle,
        bool store_messages,
        const OnBlockStored &callback) {
      size_t sz = bundle.blocks.size();

      std::vector<CID> secp_cids;
      std::vector<CID> bls_cids;

      if (store_messages) {
        if (bundle.secp_msg_includes.size() != sz
            || bundle.bls_msg_includes.size() != sz) {
          return Error::SYNC_INCONSISTENT_BLOCKSYNC_RESPONSE;
        }

        secp_cids.reserve(bundle.secp_msgs.size());
        for (const auto &msg : bundle.secp_msgs) {
          OUTCOME_TRY(cid, ipld->setCbor<SignedMessage>(msg));
          secp_cids.push_back(std::move(cid));
        }

        bls_cids.reserve(bundle.bls_msgs.size());
        for (const auto &msg : bundle.bls_msgs) {
          OUTCOME_TRY(cid, ipld->setCbor<UnsignedMessage>(msg));
          bls_cids.push_back(std::move(cid));
        }
      }

      for (size_t i = 0; i < sz; ++i) {
        OUTCOME_TRY(storeBlock(ipld,
                               std::move(bundle.blocks[i]),
                               secp_cids,
                               bundle.secp_msg_includes[i],
                               bls_cids,
                               bundle.bls_msg_includes[i],
                               store_messages,
                               callback));
      }

      return outcome::success();
    }

  }  // namespace

  outcome::result<void> storeResponse(
      const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
      std::vector<TipsetBundle> chain,
      bool store_messages,
      const OnBlockStored &callback) {
    assert(callback);

    for (auto &bundle : chain) {
      OUTCOME_TRY(storeTipsetBundle(ipld, bundle, store_messages, callback));
    }

    return outcome::success();
  }

}  // namespace fc::sync::blocksync
