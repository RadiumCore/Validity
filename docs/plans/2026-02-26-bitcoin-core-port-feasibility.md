# Feasibility Study: Porting Validity (Bitcoin Core 0.13.2 Fork) to Latest Bitcoin Core

> **Research conducted by:** Claude Code (Opus 4.6) — February 26, 2026
> **Conclusion:** Technically feasible but represents 12-24 months of work with 2-3 experienced C++ developers. Not recommended as primary path.

---

## 1. Current State of Bitcoin Core

**Latest version: Bitcoin Core 30.0** (released late October 2025, with patches 30.1 and 30.2). This represents approximately **9 years and 18+ major releases** since Bitcoin Core 0.13.2 (January 2017).

Version numbering changed: 0.13 -> 0.14 -> ... -> 0.21 -> 22.0 (dropped leading zero) -> 23 -> ... -> 30.

---

## 2. MAJOR Architectural Changes Between 0.13.2 and 30.0

### 2.1 The main.cpp Dissolution (CRITICAL)
Validity currently has `src/main.cpp` and `src/main.h` as its monolithic validation/networking file — this is where the vast majority of PoS consensus hooks live. In modern Bitcoin Core:

- **`main.cpp` no longer exists.** Split into:
  - `src/validation.cpp` / `src/validation.h` — block and transaction validation
  - `src/net_processing.cpp` / `src/net_processing.h` — P2P message handling
  - `src/node/blockstorage.cpp` — block storage management
  - `src/txmempool.cpp` was significantly refactored

This is the single biggest obstacle — Validity's PoS logic is scattered across ~30+ locations in `main.cpp`.

### 2.2 ChainstateManager and CChainState (CRITICAL)
- **`ChainstateManager`** introduced (PR #17737) to manage one or two chainstates (for AssumeUTXO)
- Global `mapBlockIndex` is gone; replaced by `BlockManager` through `ChainstateManager`
- All Validity PoS code accessing `mapBlockIndex` as a global would need rewriting

### 2.3 libbitcoinkernel Consensus Extraction (CRITICAL)
Bitcoin Core is extracting its consensus engine into `src/kernel/`. Adding PoS to this would mean extending a library specifically designed around PoW.

### 2.4 Process Separation / Multiprocess Architecture
- `src/interfaces/` defines abstract interfaces: `interfaces::Chain`, `interfaces::Node`, `interfaces::Wallet`
- Wallet code can no longer freely call into validation code
- The staking miner (`ThreadStakeMiner`) directly accesses wallet and chain state — this tight coupling would need decomposition

### 2.5 Build System: Autotools to CMake (CRITICAL)
- Bitcoin Core 29+ uses **CMake** exclusively (Autotools deleted entirely)
- Validity's entire build system would need rewriting

### 2.6 C++ Standard: C++11 to C++20 (CRITICAL)
- Validity uses **C++11**; Bitcoin Core 27+ requires **C++20**
- Modern features used extensively: `std::optional`, `std::variant`, `std::filesystem`, concepts, ranges
- OpenSSL entirely removed; replaced with native crypto

### 2.7 SegWit
- Completely changes transaction serialization (witness data)
- `CTransaction` now has witness fields; `GetHash()` vs `GetWitnessHash()` distinction
- Validity's `CTransaction` has a **`nTime` field** (PoS) and `IsCoinStake()` — must coexist with SegWit

### 2.8 Taproot/Schnorr (BIP 340/341/342)
- SegWit v1 with Schnorr signatures
- Major script verification engine changes
- Validity uses older `secp256k1` with custom Schnorr module

### 2.9 Descriptor Wallets
- Legacy wallets (Berkeley DB) deprecated and being removed
- New wallets use **SQLite** with output descriptors
- BDB dependency being phased out

### 2.10 Serialization Framework Rewrite
- Old `ADD_SERIALIZE_METHODS` / `SerializationOp` with `nType`/`nVersion` replaced by `SERIALIZE_METHODS` macro
- Every serializable class would need updating

### 2.11 P2P Protocol Changes
- BIP155 (addrv2): Tor v3 and other address types
- BIP324 (v2 P2P transport): Encrypted connections
- Compact Block Relay improvements
- Package Relay

### 2.12 CCoins vs Coin
- 0.13.2 uses `CCoins` (all outputs of a transaction)
- Modern uses `Coin` (individual UTXO entries)
- Validity's PoS code directly creates `CCoins` objects — all UTXO access patterns change

---

## 3. Validity's PoS Code That Must Be Ported

| File | Purpose | Porting Complexity |
|------|---------|-------------------|
| `src/pos.cpp` (~257 lines) | Stake kernel hash, proof verification, stake cache | Very High |
| `src/pos.h` (~45 lines) | PoS interface declarations | High |
| `src/pow.cpp` / `src/pow.h` | Modified for dual PoW/PoS difficulty | Very High |
| `src/miner.cpp` | `ThreadStakeMiner`, PoS block assembly | Very High |
| `src/primitives/block.h` | `vchBlockSig`, `IsProofOfStake()` | Very High |
| `src/primitives/transaction.h` | `nTime` field, `IsCoinStake()` | Very High |
| `src/chain.h` | `nStakeModifier`, `BLOCK_PROOF_OF_STAKE` | Very High |
| `src/consensus/params.h` | PoS parameters, protocol versions | High |
| `src/main.cpp` | ~30+ PoS integration points | EXTREME |
| `src/chainparams.cpp` | PoS chain parameters | High |

---

## 4. Estimated Scope

### Phase 1: Foundation (3-6 months, 1-2 devs)
1. Fork Bitcoin Core 30.0
2. Add `nTime` to `CTransaction`, `vchBlockSig` to `CBlock`
3. Add `nStakeModifier` to `CBlockIndex`
4. Extend `Consensus::Params` with PoS parameters
5. Port Scrypt hashing, set up CMake, update to C++20

### Phase 2: Consensus Engine (4-8 months)
1. Port `pos.cpp` to use `Coin` and `ChainstateManager`
2. Modify `validation.cpp` with all ~30+ PoS integration points
3. Modify `node/miner.cpp` for PoS block assembly
4. Integrate with `libbitcoinkernel`

### Phase 3: Wallet and RPC (2-4 months)
1. Staking wallet integration with descriptor wallets
2. Port staking RPCs
3. Update `interfaces::Chain` for PoS

### Phase 4: Testing (3-6 months)
1. Regression testing against existing blockchain
2. Network protocol compatibility or migration plan

**Total estimated files: ~55-90 | New PoS code: 2,000-4,000 lines | Total delta: 10,000-20,000 lines**

---

## 5. Biggest Risks

1. **Consensus Divergence** — Every historical block must validate identically under new code
2. **Serialization Compatibility** — Custom fields must remain backward-compatible
3. **SegWit/Taproot + PoS Interaction** — Uncharted territory for coinstake transactions
4. **Global State Elimination** — All PoS functions need context objects instead of globals
5. **Maintainability** — Future Bitcoin Core releases would require careful merging

---

## 6. Alternative Approaches

| Option | Effort | Risk | Benefit |
|--------|--------|------|---------|
| **A: Full Rebase to 30.0** | 12-24 months | Extreme | Modern everything |
| **B: Incremental (0.13→0.15→...→30)** | 18-36 months | High per step | Lower risk per step |
| **C: Cherry-pick security fixes** | 2-4 months | Low | Fast, minimal risk |
| **D: Rebase onto BlackCoin More 26.x** | 1-3 months | Medium | Best effort/reward ratio |

---

## 7. Bottom Line

**Porting to Bitcoin Core 30.0 directly is roughly equivalent to building a new PoS cryptocurrency from scratch on a modern base.** The recommended path is **Option D: Rebase onto BlackCoin More 26.x** (see separate report).
