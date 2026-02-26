# Validity Wallet Security Audit Report

**Date:** 2026-02-26
**Scope:** Full codebase — wallet encryption, key management, serialization, network, RPC, Qt UI
**Constraint:** No consensus/blockchain changes (UI and client-side only)

---

## Summary

15 findings across the codebase. 2 critical/medium issues were fixed immediately. The remaining findings are documented below for future work.

| Severity | Count | Fixed |
|----------|-------|-------|
| CRITICAL | 1     | 1     |
| HIGH     | 2     | 0     |
| MEDIUM   | 4     | 1     |
| LOW      | 2     | 0     |
| INFO     | 6     | —     |

---

## Fixed Issues

### 1. CRITICAL — CExtKey::Unserialize Buffer Overflow
**File:** `src/key.h` (CExtKey::Unserialize)
**Issue:** `ReadCompactSize(s)` returns an attacker-controlled length that was used directly in `s.read()` without validation. A crafted serialized extended key could cause a stack buffer overflow writing into a fixed `BIP32_EXTKEY_SIZE` (74-byte) array.
**Fix:** Added length validation: `if (len != BIP32_EXTKEY_SIZE) throw std::runtime_error("Invalid extended key size\n");`
**Note:** CExtPubKey::Unserialize already had this check. CExtKey was missing it.

### 2. MEDIUM — UnlockStaking Case Fall-Through
**File:** `src/qt/askpassphrasedialog.cpp`
**Issue:** The `UnlockStaking` case in the constructor's switch statement fell through into the `Unlock` case, causing the staking checkbox to be immediately unchecked by the Unlock case's `setChecked(false)`. This made the "Unlock for staking only" feature unreliable.
**Fix:** Added `break;` after UnlockStaking case and duplicated the necessary UI setup for the Unlock case independently.

---

## Open Issues (Not Fixed — Require Deeper Changes)

### 3. HIGH — #include of .cpp File
**File:** `src/rpc/misc.cpp`
**Issue:** Contains `#include "rpc/blockchain.cpp"` which includes an entire implementation file. This is fragile — any change to blockchain.cpp could break misc.cpp in unexpected ways.
**Recommendation:** Extract shared functions into a proper header file (`rpc/blockchain.h`) and include that instead.

### 4. HIGH — MSVC memory_cleanse May Be No-Op
**File:** `src/support/cleanse.cpp`
**Issue:** `memory_cleanse()` uses `OPENSSL_cleanse()` which may be optimized away by MSVC in release builds. On Windows, `SecureZeroMemory()` should be used instead to guarantee the memory is actually zeroed.
**Recommendation:** Add `#ifdef _WIN32` / `SecureZeroMemory()` path, or use a volatile function pointer pattern to prevent optimization.

### 5. MEDIUM — Passphrase in Non-Secure Memory
**File:** `src/qt/askpassphrasedialog.cpp` (accept method)
**Issue:** `ui->passEdit1->text().toStdString()` creates temporary `std::string` objects in non-mlock'd heap memory. The passphrase briefly exists outside secure allocator protection before being assigned to `SecureString`.
**Recommendation:** Consider using `QLineEdit::text().toUtf8().constData()` to reduce copies, or implement a custom QLineEdit that stores text in secure memory.

### 6. MEDIUM — Division by Zero in Staking Calculations
**File:** `src/qt/overviewpage.cpp`
**Issue:** Staking reward time estimation divides by network weight without checking for zero. If `nNetworkWeight` is 0 (e.g., during initial sync), this causes undefined behavior.
**Recommendation:** Add `if (nNetworkWeight > 0)` guard before division.

### 7. MEDIUM — pwalletMain Null Dereference Risk
**File:** `src/qt/overviewpage.cpp`
**Issue:** `pwalletMain` is accessed without null checks in several staking-related functions. During early startup or if wallet is disabled, this could cause a crash.
**Recommendation:** Add null checks before all `pwalletMain` accesses.

### 8. LOW — ReadVarInt Unbounded Loop
**File:** `src/serialize.h`
**Issue:** `ReadVarInt()` loops reading bytes until it finds one without the continuation bit. A malformed stream could cause excessive looping.
**Recommendation:** Add a maximum iteration count (e.g., 10 for uint64_t).

### 9. LOW — RPC Cookie Not Wiped on Shutdown
**File:** `src/rpc/protocol.cpp`
**Issue:** The `.cookie` authentication file is deleted on shutdown but its contents are not overwritten first. The data may remain on disk in unallocated sectors.
**Recommendation:** Overwrite cookie file contents before deletion.

---

## Informational (No Action Required)

1. **Theme loading is safe** — QSS loaded from Qt resource system (compiled into binary), not from external files. No injection risk.
2. **Serialization limits work** — `ReadCompactSize()` enforces `MAX_SIZE` limit (0x2000000), preventing memory exhaustion from oversized allocations.
3. **Network peer validation** — Peer addresses are validated before use; no IP spoofing risk in the address management layer.
4. **File permissions** — Wallet file permissions are set correctly on creation (0600 on Unix).
5. **Single KDF method** — Only one key derivation function is supported for wallet encryption. Adding scrypt or Argon2 would be a future improvement but is not a vulnerability.
6. **Qt resource paths** — All resource paths use `:/` prefix (compiled resources), not filesystem paths. Safe from path traversal.
