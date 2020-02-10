/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_store.hpp"

#include "common/outcome.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "primitives/cid/json_codec.hpp"
#include "primitives/tipset/tipset_key.hpp"
#include "storage/ipfs/datastore_key.hpp"

namespace fc::storage::chain {

  namespace {
    static const ipfs::Key chain_head_key = ipfs::makeKeyFromString("head");
  }

  outcome::result<ChainStore> ChainStore::create(
      std::shared_ptr<ipfs::BlockService> bs,
      std::shared_ptr<ipfs::IpfsDatastore> ds,
      std::shared_ptr<::fc::chain::block_validator::BlockValidator>
          block_validator) {
    ChainStore cs{};
    cs.block_service_ = std::move(bs);
    cs.data_store_ = std::move(ds);
    cs.block_validator_ = std::move(block_validator);

    // TODO (yuraz): FIL-151 add notifications

    return cs;
  }

  /*
    v, ok := cs.tsCache.Get(tsk)
        if ok {
                return v.(*types.TipSet), nil
        }

        var blks []*types.BlockHeader
        for _, c := range tsk.Cids() {
                b, err := cs.GetBlock(c)
                if err != nil {
                        return nil, xerrors.Errorf("get block %s: %w", c, err)
                }

                blks = append(blks, b)
        }

        ts, err := types.NewTipSet(blks)
        if err != nil {
                return nil, err
        }

        cs.tsCache.Add(tsk, ts)

        return ts, nil
   */

  outcome::result<primitives::tipset::Tipset> ChainStore::loadTipset(
      const primitives::tipset::TipsetKey &key) {
    primitives::tipset::Tipset tipset{};
    for (auto &cid :key.cids) {
//      OUTCOME_TRY(block, block_service_->getBlockContent())
    }

    return tipset;
  }

  outcome::result<void> ChainStore::load() {
    OUTCOME_TRY(head, primitives::cid::getCidOfCbor(chain_head_key.value));
    OUTCOME_TRY(buffer, data_store_->get(head));
    OUTCOME_TRY(cids, codec::json::decodeCidVector(buffer));
    auto &&ts_key = primitives::tipset::TipsetKey{std::move(cids)};
    OUTCOME_TRY(tipset, loadTipset(ts_key));
    heaviest_tipset_ = std::move(tipset);

    return outcome::success();
  }

  outcome::result<void> ChainStore::writeHead(
      const primitives::tipset::Tipset &tipset) {
    OUTCOME_TRY(data, codec::json::encodeCidVector(tipset.cids));
    OUTCOME_TRY(head, primitives::cid::getCidOfCbor(chain_head_key.value));
    OUTCOME_TRY(data_store_->set(head, data));
    return outcome::success();
  }
}  // namespace fc::storage::chain
