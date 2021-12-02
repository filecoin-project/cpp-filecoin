# Devnet

## Your new Filecoin network start

To run a new filecoin network, you should run at least one node, and a miner (miners are a representation of the network power, so the more miners connected and sealed, the more power your network will have). In this guide, we will consider a network of two nodes and two miners(genesis and common ones)

1. To run the first node you will need to generate a new genesis file which will be an initial instruction for all new connected nodes.
   This can be done with lotus-seed tools:
   - Clone  [lotus repository](https://github.com/filecoin-project/lotus "lotus project")

   - build lotus-seed with `$make lotus-seed` or `make debug` - for debug version initiation,
     or `make 2k` for a version with 2 KiB sector size  or `make interopnet` for interopnet version.

   - Generate genesis and preseal. Ensure lotus-seed was built in the previous step and run script node `$genesis.js DIR 1` to generate genesis. `DIR` - directory where genesis and presealed sectors will be generated, 1 - number of sectors for a miner. In case your lotus executables lay in a custom directory or script can not find them, you can link lotus target with `LOTUS=/<path to your lotus executable>/lotus node genesis.js <directory where results will be> <sealing sectors upper bound>`

2. Run your node-1 with new genesis file as it was done in chapter **Fuhon node establishing**

3. Run your node-2 as the 1st one, but be sure to open api port on it with `--api API_PORT`

4. Connect node-2 to node-1 by commands:
   `addr=$(lotus --repo REPO1 net listen | grep 127.0.0.1)`
   `lotus --repo REPO2 net connect $addr`t

5. After generating a new genesis file, you will also receive genesis.json with declared miners list, these are addresses of the initial miners.

6. To start miner that is declared in genesis file (step 5) you can use chapter **Fuhon miner establishing** but with some major changes.
   You will also need the following flags to be provided:   `--genesis-miner`, `--actor ACTOR`, `--pre-sealed-sectors PRESEAL_DIR`, `--pre-sealed-metadata PRESEAL_JSON`.

7. To start a new miner just follow the steps declared in **Fuhon miner establishing**, or you can run lotus miner using [filecoin-docs](https://docs.filecoin.io/get-started/lotus/installation/#minimal-requirements "lotus establishing guide")
8. After two nodes will be connected and your miner will work, a new filecoin network starting to exist, since that moment you can share your peers to invite new participants. 
