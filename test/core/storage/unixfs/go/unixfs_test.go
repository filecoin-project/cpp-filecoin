package unixfs_test

import (
	"bytes"
	"context"
	"github.com/ipfs/go-blockservice"
	"github.com/ipfs/go-datastore"
	"github.com/ipfs/go-ipfs-blockstore"
	"github.com/ipfs/go-ipfs-chunker"
	"github.com/ipfs/go-ipfs-exchange-offline"
	"github.com/ipfs/go-merkledag"
	"github.com/ipfs/go-unixfs/importer/balanced"
	"github.com/ipfs/go-unixfs/importer/helpers"
	"github.com/stretchr/testify/require"
	"testing"
)

var ctx = context.Background()
var bs = blockstore.NewBlockstore(datastore.NewMapDatastore())
var dag = merkledag.NewDAGService(blockservice.New(bs, offline.Exchange(bs)))

func caseUnixfs(t *testing.T, data string, chunkSize int64, maxLinks int, cidStr string) {
	sp := chunk.NewSizeSplitter(bytes.NewBuffer([]byte(data)), chunkSize)
	dbp := helpers.DagBuilderParams{
		Maxlinks:  maxLinks,
		RawLeaves: true,
		Dagserv:   dag,
	}
	db, e := dbp.New(sp); require.NoError(t, e)
	root, e := balanced.Layout(db); require.NoError(t, e)
	require.Equal(t, root.Cid().String(), cidStr)
}

func TestUnixfs(t *testing.T) {
	caseUnixfs(t, "[0     9)", 10, 2,"bafkreieqjdswwovkokzitxxsn4ppswzzsy2qs623duz5ivjhplglbygbou")
	caseUnixfs(t, "[0     10)", 10, 2,"bafkreiaymswyg525nktwb75n53rcwdqvjbfjgi5nu4glriglylthbnuxnu")
	caseUnixfs(t, "[0     10)[10   19)", 10, 2,"QmUyaoLFxpUpZra4qs65dMbrBMEERJt91JL3kdyKEXqLxN")
	caseUnixfs(t, "[0     10)[10     21)", 10, 2,"QmZ9KupPPphsds2Cbu7Ntb73tUHbyrZEgJisHMmw9AUfQP")
	caseUnixfs(t, "[0     10)[10    20)[10    30)", 10, 2,"QmSYvtzqeJY6zCRk31SdRweDD6z1bwP9GJY5bmKQ55XwWp")
	caseUnixfs(t, "[0     10)[10    20)[10     31)", 10, 2,"QmZ2Be9hJQhifpgASmQTGkL7j9KUaJC5W9siL6MnFSkLhd")
	caseUnixfs(t, "[0     10)[10    20)[10    30)", 5, 3,"QmYb4gZGCAkRdNZbc5npDzLJA34ZzPtdVDmESb9Xk76Ws2")
}
