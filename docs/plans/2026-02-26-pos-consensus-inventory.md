# Validity Wallet: Proof of Stake Consensus Code Inventory

> **Research conducted by:** Claude Code (Opus 4.6) — February 26, 2026
> **Purpose:** Complete inventory of all consensus-touching PoS code for porting reference

---

## Executive Summary

Validity contains **~1,670 lines of custom PoS consensus code** across **12 core files**. The implementation is BlackCoin PoS v3 with Qtum stake cache, plus Validity-specific reward schedule and Dev Fund. **Zero blockchain-incompatible changes** in our UI work — all modifications are PoS-specific and don't touch Bitcoin's base consensus.

---

## 1. Core PoS Kernel Files

### 1.1 src/pos.h (46 lines)
- `ComputeStakeModifier()` — Hash modifier computation
- `CheckCoinStakeTimestamp()` — Protocol-specific timestamp validation
- `CheckStakeKernelHash()` — Core kernel hash against target
- `CheckProofOfStake()` — Main PoS proof validation
- `CheckKernel()` — Two overloads with stake cache support (Qtum)
- `CacheKernel()` — Stake cache implementation
- `VerifySignature()` — Coinstake signature verification

### 1.2 src/pos.cpp (258 lines)

**ComputeStakeModifier()** (lines 28-36)
```cpp
hash(kernel || pindexPrev->nStakeModifier)
```

**CheckCoinStakeTimestamp()** (lines 39-46)
- V2: block time == tx time, with alignment mask `(nTime & nStakeTimestampMask) == 0`
- V1: simple equality check

**CheckStakeKernelHash()** (lines 73-119) — BlackCoin Kernel Protocol v3
```
hash(nStakeModifier || txPrev.nTime || prevout.hash || prevout.n || nTime) < bnTarget * nWeight
```

**CheckProofOfStake()** (lines 122-163)
- Protocol-specific maturity: V4: 120 blocks, V3: 60 blocks, V1-V2: 6 hours

**CheckKernel()** (lines 179-231) — Direct + cache-enabled overloads

**CacheKernel()** (lines 233-257) — Populates `CStakeCache` map

---

## 2. Consensus Parameters

### 2.1 src/consensus/params.h (138 lines)

**Protocol Version Checks:**
```cpp
IsProtocolV1RetargetingFixed(nTime)  // Always true
IsProtocolV2(nTime)                  // nTime > nGenesisBlockTime
IsProtocolV3(nTime)                  // nTime > 1461851161
IsAvgFeeProtocol(nTime)              // nTime > 1470919889
IsProtocolV4(nHeight)                // nHeight >= 1655000 (DEV_FUND_BLOCK_HEIGHT)
IsBlockDevFund(nHeight)              // nHeight % 10080 == 0 && IsProtocolV4
```

**PoS Parameters:**
```cpp
nStakeTimestampMask = 0xf           // Alignment mask
nStakeMaturity: V4=120, V3=60      // Variable block maturity
nStakeMinAge = 6 * 60 * 60         // Pre-V3: 6 hours
GetTargetSpacing: V4=57s, else=60s
nLastPOWBlock = 20160               // Last PoW-allowed block
```

### 2.2 src/chainparams.cpp (Mainnet)
```cpp
posLimit = 0x0000ffffffffffff...
posLimitV2 = 0x00000fffffffffff...
nTargetTimespan = 60s
nTargetTimespanNEW = 15 * 60s
nProtocolV3Time = 1461851161
nStakeTimestampMask = 0xf
nCoinbaseMaturity = 60
nStakeMinAge = 21600s
```
Genesis Block: 1431857735 (May 17, 2015)

---

## 3. Block Validation & Difficulty

### 3.1 src/pow.cpp (119 lines)

**GetNextTargetRequired()** — Entry point for difficulty
- Finds last block of same type (PoS/PoW)
- V3+: Caps actual spacing at `nTargetSpacing * 10`
- Dynamic interval: Before block 48: 1 interval, After: 15 intervals

### 3.2 src/main.cpp — Block Validation

**Key integration points (~45 lines):**
- Stake modifier computation (line 2728)
- Difficulty check (line 2731)
- Coinstake kernel validation (line 2749)
- Kernel maturity check (line 2744)
- Coinstake maturity enforcement (lines 2331-2339)

---

## 4. Transaction Types

### src/primitives/transaction.h (8 lines)
```cpp
bool IsCoinStake() const {
    return (vin.size() > 0 && (!vin[0].prevout.IsNull()) &&
            vout.size() >= 2 && vout[0].IsEmpty());
}
```

### src/primitives/block.h (8 lines)
```cpp
bool IsProofOfStake() const {
    return (vtx.size() > 1 && vtx[1].IsCoinStake());
}
```

---

## 5. Staking Miner & Block Assembly

### src/wallet/wallet.cpp — CreateCoinStake() (lines 688-888, ~200 lines)
1. Lock wallet, gather unspent coins
2. Iterate with descending value preference
3. Call `CheckKernel()` with stake cache
4. Extract script pubkey, get private key
5. Build coinstake inputs/outputs
6. Sign all inputs with `SIGHASH_ALL`
7. Validate size < MAX_STANDARD_TX_SIZE

### src/miner.cpp — Block Template (~150 lines)
- `GetProofOfWorkReward()` — Fixed 10,000 VAL per PoW block
- `CreateNewBlock()` — Handles both PoW and PoS assembly
- `ThreadStakeMiner()` — Main staking thread

---

## 6. Reward Functions

### getFixedStakeSubsidy() (~100 lines)
Complex schedule with 30+ phases:
- Blocks 0-2779: 0 VAL
- Blocks 2880-30240: 25 VAL
- Blocks 30241-337999: 5 VAL
- Blocks 338000+: Graduated decline (4.5, 4, 3.5, ..., 0.22 VAL)
- Blocks 1655000+ (V4): Dev fund tiers

### GetProofOfStakeSubsidy() (~25 lines)
```cpp
nSubsidy = getFixedStakeSubsidy(nHeight);
// Add running average fees (protocol version dependent)
if (nHeight >= AVG_FEE_START_BLOCK_V2)
    return nSubsidy + GetRunningFee(pindexPrev, nFees);
```

### GetDevSubsidy() (~70 lines)
- V4+ only (block 1,655,000+)
- Weekly distribution (every 10,080 blocks)
- 25+ yearly declining tiers from 264 VAL to 0

### GetRunningFee() (~40 lines)
- Rolling 10-block average of transaction fees
- Cache to avoid repeated disk lookups

---

## 7. Summary Table

| File | Lines | Component |
|------|-------|-----------|
| pos.h | 46 | Headers & declarations |
| pos.cpp | 258 | Kernel validation, coinstake proof |
| consensus/params.h | 30 | Protocol versions, PoS params |
| chainparams.cpp | 20 | Mainnet PoS config |
| pow.cpp | 85 | Difficulty retargeting (PoS-specific) |
| main.cpp (validation) | 100 | Block validation, stake modifier |
| primitives/transaction.h | 8 | IsCoinStake() |
| primitives/block.h | 8 | IsProofOfStake() |
| wallet/wallet.cpp | 200 | CreateCoinStake() |
| miner.cpp | 150 | Block template, rewards |
| main.cpp (rewards) | 270 | Subsidy, fees, dev fund |
| chain.h | 2 | nStakeModifier field |
| **TOTAL** | **~1,177** | **Custom PoS consensus** |

---

## 8. Portability Assessment

### Must Be Ported for PoS:
1. pos.h/pos.cpp (100%) — Core kernel validation
2. Consensus params (100%) — All PoS difficulty parameters
3. Block/Transaction extensions (100%) — IsProofOfStake(), IsCoinStake(), nStakeModifier
4. Difficulty retargeting (100%) — GetNextTargetRequired() with V2/V3/V4 logic
5. Reward schedules (90%) — Custom to Validity
6. CreateCoinStake() (100%) — Staking miner logic
7. Block validation hooks (100%) — ConnectBlock() PoS checks

### Reusable from Bitcoin Core:
- Standard script validation
- Transaction serialization
- Network protocol
- Mempool management
- All PoW components (unchanged)

### Key Dependencies:
- All PoS functions depend on `Params().GetConsensus()` for config
- Stake validation needs `mapBlockIndex` and transaction lookup
- Block assembly needs `CBlockIndex` with nStakeModifier
- Difficulty needs access to previous block timestamps/targets
