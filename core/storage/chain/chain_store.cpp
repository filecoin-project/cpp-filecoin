/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_store.hpp"

#include "common/outcome.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/ipfs/datastore_key.hpp"

namespace fc::storage::chain {

  namespace {
    static ipfs::Key chainHeadKey = ipfs::makeKeyFromString("head");
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
    return tipset;
  }

/*
 * 	head, err := cs.ds.Get(chainHeadKey)
	if err == dstore.ErrNotFound {
		log.Warn("no previous chain state found")
		return nil
	}
	if err != nil {
		return xerrors.Errorf("failed to load chain state from datastore: %w", err)
	}

	var tscids []cid.Cid
	if err := json.Unmarshal(head, &tscids); err != nil {
		return xerrors.Errorf("failed to unmarshal stored chain head: %w", err)
	}

	ts, err := cs.LoadTipSet(types.NewTipSetKey(tscids...))
	if err != nil {
		return xerrors.Errorf("loading tipset: %w", err)
	}

	cs.heaviest = ts

	return nil
 */

  outcome::result<void> ChainStore::load() {
    OUTCOME_TRY(key, primitives::cid::getCidOfCbor(chainHeadKey.value));
    OUTCOME_TRY(res, data_store_->get(key));

    return outcome::success();
  }
}  // namespace fc::storage::chain
