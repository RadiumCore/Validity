# Changelog

##v13.0.9-alpha-peer_blocks_64 (2010-2-11)
- Change max concurrent blocks per peer from 16 to 64

##v13.0.8-alpha-block_window_4096 (2010-2-11) DONE
- Change block download window from 4096

##v13.0.7-alpha-block_window_2048 (2010-2-11) DONE!
- Change block download window from 1024 to 2048
- Change max concurrent blocks per peer from 16 to 32

##v13.0.6-alpha-set_conf_name (2010-2-11) DONE!
- Revert config file name back to radium.conf

##v13.0.5-alpha-avg_fee_optimize (2010-2-11) DONE!

- Attempt to optimize average fee calculation loop

##v13.0.4-alpha (2010-2-11)
- Fix syncing delay
- Fix v13 nodes not reporting completions of IBD

##v13.0.2-alpha (2010-2-3)
- Update fixed seed nodes
- Fix wrong port in fixed node generation script
- Refactor code causing windows build issues. 

##v13.0.0-alpha (2010-1-12)
- First Radium alpha build
- Implemented Radium Chain

## v2.13.2.4 (2019-11-11)
- Updated fixed seeds
- Added burn RPC call
- Set default MAX_OP_RETURN_RELAY to 15000
- Removed unit selector from status bar

## v2.13.2.3 (2019-04-02)
- Updated fixed seeds
- Some small fixes and refactorings
- Fixed wrongly displayed balances in GUI and RPC
- Added header spam filter (fake stake vulnerability fix)
- Added total balance in RPC call getwalletinfo

## v2.13.2.2 (2019-03-13)
- Updated dependencies
- Updated fixed seeds
- Some small fixes and updates
- Fixed walletpassphrase RPC call (wallet now can be unlocked for staking only)
- Allowed connections from peers with protocol version 60016
- Disabled BIP 152

## v2.13.2.1 (2018-12-03)

- Updated to Bitcoin Core 0.13.2
- Some small fixes and updates from Bitcoin Core 0.14.x branch
- Fixed testnet and regtest
- Added Qt 5.9 support for cross-compile
- Added Qt support for ARMv7
- Added out-of-sync modal window (backport of Core's PR8371, PR8802, PR8805, PR8906, PR8985, PR9088, PR9461, PR9462)
- Added support for nested commands and simple value queries in RPC console (backport of Core's PR7783)
- Added abortrescan RPC call (backport of Core's PR10208)
- Added reservebalance RPC call
- Removed SegWit
- Removed replace-by-fee
- Removed address indexes
- Removed relaying of double-spends
- Removed drivechain support using OP_COUNT_ACKS
- Proof-of-stake related code optimized and refactored

## v2.12.1.1 (2018-10-01)

- Rebranded to Blackcoin More
- Some small fixes and updates from Bitcoin Core 0.13.x branch
- Added use available balance button in send coins dialog (backport of Core's PR11316)
- Added a button to open the config file in a text editor (backport of Core's PR9890)
- Added uptime RPC call (backport of Core's PR10400)
- Removed P2P alert system (backport of Core's PR7692)
