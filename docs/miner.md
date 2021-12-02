# Miner

## Miner key and funds
1. For proper miner work you will need a bls wallet, which can be made with lotus-cli by using the following command: `$lotus --repo REPO wallet new bls`, after the execution of this command you will revive the public address of your new account, and a private key in `<repo of your working node>/keystore`.
2. Also, you may need to put some Filecoins on your account from other addresses you may have, and you can do it  with `lotus --repo REPO send --from SENDER_ADDRESS DISTANATION_ADDRESS`

3. In case you want to get some Filecoins, you can buy them on stocks with the usage of your public address.
4. Other advanced commands you may find in [filecoin-docs about lotus CLI](https://docs.filecoin.io/get-started/lotus/installation/#interact-with-the-daemon "lotus CLI documentation").

## Fuhon miner establishing
To start mining at first you need to configure and run Fuhon or Lotus node. Before all make sure that your node had successfully **connected** and updated to the current **block height** (actual network block number). Also, your node's api port should be opened for connection.

To provide wallet functions and oracle information be sure to run lotus-cli at first. You can find how to do it in **Lotus-CLI establishing**.

Once all the needed  modules will be started, you can continue with the miner configuration.

1. Create miner work directory (f.e. `$mkdir miner`)
2. Download [miner executable](https://github.com/filecoin-project/cpp-filecoin/releases "executables releases") and put it in work directory
   3.Copy proof parameters json file with command: `cp /<path to cpp-filecoin>/core/proofs/parameters.json MINER_REPO/proof-params.json`
4. Run your miner with ` --repo REPO --miner-repo MINER_REPO --owner PUBLIC_ADRESS --miner-api MINER_API_PORT --sector-size SECTOR_SIZE`

## Configuration Lotus-Fuhon
### Lotus-node and Fuhon-miner
1. To start up a lotus node at first you need to get a [lotus executable](https://github.com/filecoin-project/lotus/releases/ "lotus releases")
2. You can run your lotus node with the next command: `$lotus --repo REPO daemon --genesis genesis.car --profile bootstrapper`, more about running lotus software can be found in [filecoin-docs](https://docs.filecoin.io/get-started/lotus/installation "lotus filecoin documentation").
3. Fuhon-miner establishing is described in **Fuhon miner establishing**
### Fuhon-node and Lotus-miner
1. Establishing a Fuhon node is described in **Fuhon node establishing**
2. Lotus miner can be established in the following way:
   - to begin with you need your own bls wallet(described in **Working with Lotus-CLI**)
   - init your miner with the command: `lotus-miner --repo REPO --miner-repo MINER_REPO init --nosync --owner BLS_WALLET_ADDR --sector-size SECTOR_SIZE`
   - to know more about lotus software establishing, read [filecoin-docs](https://docs.filecoin.io/get-started/lotus/installation/#minimal-requirements "lotus establishing guide")

