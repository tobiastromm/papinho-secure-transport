<!-- SPDX-License-Identifier: MPL-2.0 -->

# Security, lifecycle and negative matrix

SS-7 closes the API 2.0/SPI 3.0 security and lifecycle negative matrix. The
matrix combines fresh deterministic and real-provider execution with the
accepted SS-3 through SS-6 provider evidence. SS-7 changed tests and current
documentation only; it did not change production behavior, API, SPI or
third-party NSS/NSPR material.

`NOT_SUPPORTED` and `NOT_APPLICABLE` below are factual boundaries, not PASS.
Provider selection rejects capability-ineligible configurations before
transport binding.

## Canonical matrix

| Case | Role/provider/phase | Trigger and ownership | Expected normalized outcome | Close/fallback/cleanup | Executed result |
|---|---|---|---|---|---|
| Invalid role | core/config | unknown role; caller owns inputs | `INVALID_ARGUMENT`, configuration | no provider init, no close, no fallback | PASS, fresh deterministic |
| Invalid selection mode | core/config | unknown enum | `INVALID_ARGUMENT`, configuration | no provider init | PASS, fresh deterministic |
| EXACT unknown | either/core selection | unknown ID, unbound | `UNSUPPORTED`, selection | no provider init/binding | PASS, fresh deterministic |
| EXACT capability-ineligible | either/core selection | SYSTEM trust against custom-only mock | `UNSUPPORTED`, selection | skipped before init | PASS, fresh deterministic |
| ORDERED empty | either/core config | empty list | `INVALID_ARGUMENT`, configuration | no provider init | PASS, fresh deterministic |
| ORDERED unknown then eligible | CLIENT/core selection | first missing, second eligible | eligible provider selected | skip only before binding | PASS, fresh deterministic |
| ORDERED duplicate | CLIENT/core selection | repeated same valid ID | frozen order remains deterministic | first eligible occurrence wins | PASS, fresh deterministic |
| AUTOMATIC init failure | CLIENT/core selection | first provider init fails | second eligible provider selected | partial first-provider cleanup; pre-binding fallback only | PASS, fresh deterministic |
| TLS min greater than max | core/config | 1.3 minimum, 1.2 maximum | `INVALID_ARGUMENT`, configuration | no provider init | PASS, fresh deterministic and `test_tls_policy` |
| SERVER without Local Identity | core/config | no credentials | `POLICY_VIOLATION`, configuration | no provider init | PASS, fresh deterministic |
| SERVER Expected Peer Name | core/config | peer name supplied | `POLICY_VIOLATION`, configuration | no provider init | PASS, provider validation regressions |
| Malformed ALPN shape | core/config | zero-length item | `INVALID_ARGUMENT`, configuration | transactional rejection | PASS, fresh deterministic |
| Duplicate ALPN | core/config | duplicate byte strings | `INVALID_ARGUMENT`, configuration | transactional rejection | PASS, `test_tls_policy` |
| Caller mutation after copy | core/config | mutate hostname/ALPN/cert/key/CA | frozen values unchanged | retained copies released with snapshot | PASS, lifecycle/policy regressions |
| Private-key release | core/identity | final credential release | key copy overwritten before free | volatile zeroization loop | PASS, structural audit plus release-path regression |
| Runtime release with children | core/runtime | release requested while two children live | destruction deferred | reverse successful-init order after last child | PASS, fresh deterministic |
| Attach failure before acceptance | all/SPI lifecycle | `ownership_accepted=0` | transport failure | consumer closes; PST close count zero | PASS, fresh deterministic |
| Attach failure after acceptance | all/SPI lifecycle | `ownership_accepted=1` then failure | terminal failure | PST closes exactly once; no fallback | PASS, fresh deterministic |
| Handshake failure after binding | all/core lifecycle | provider returns protocol failure | `FAILED`, immutable provider/cause | PST closes once; provider switch count zero | PASS, fresh deterministic |
| Calls after FAILED | all/core lifecycle | handshake/read/write/wait/shutdown repeated | `INVALID_STATE` | no resurrection or second close-notify | PASS, fresh deterministic |
| Calls after CLOSED | all/core lifecycle | second shutdown | `INVALID_STATE` | second close-notify count zero | PASS, fresh deterministic stress |
| Local Identity malformed | CLIENT/SERVER, all eligible providers | missing/malformed DER or key, mismatch, malformed chain | bounded policy/auth failure | no leaked credential/provider state | PASS, provider validation regressions |
| CUSTOM trust malformed/unrelated | CLIENT/SERVER, advertised roles | malformed or unrelated anchors | `AUTH_FAILURE`; invalid/untrusted reason | no SYSTEM union/fallback | PASS, accepted provider negative matrices |
| SYSTEM trust negative | OpenSSL/Schannel advertised roles | untrusted root, wrong EKU/name where applicable | auth/name failure | temporary fixture cleanup | PASS, adapter/provider regressions |
| NSS SERVER SYSTEM trust | NSS SERVER | capability requested | pre-binding ineligible | no emulation or fallback | NOT_SUPPORTED factual limitation |
| Peer certificate OPTIONAL invalid | SERVER, all providers | invalid/untrusted cert presented | auth failure; never treated absent | terminal, no resurrection | PASS, accepted provider matrices |
| Peer certificate REQUIRED absent | SERVER, all providers | no certificate | `AUTH_FAILURE`, `PEER_CERT_ABSENT` | terminal cleanup | PASS, accepted provider matrices |
| Role-correct usage | all advertised roles | serverAuth/clientAuth crossed | `PEER_CERT_INVALID` | no usage widening | PASS, accepted SYSTEM/CUSTOM matrices |
| Disjoint TLS policy | all eligible pairs | no common version | `PROTOCOL_FAILURE`/TLS policy diagnostic | no downgrade or late fallback | PASS, accepted TLS-policy matrix |
| Schannel SERVER TLS 1.3 | Schannel SERVER | TLS 1.3 required | pre-binding ineligible | no attempt based on header presence | NOT_SUPPORTED on validated environment |
| OpenSSL SERVER ALPN negative | OpenSSL SERVER | required mismatch/no offer; optional mismatch | required fails, optional continues without ALPN | provider remains pinned | PASS, accepted ALPN matrix |
| Schannel/NSS SERVER ALPN | Schannel/NSS SERVER | ALPN required | pre-binding ineligible | no ClientHello parser | NOT_SUPPORTED complete PST semantics |
| Established raw EOF/reset | representative cross-provider | accepted transport, established TLS | `TRUNCATED`, READ | exactly one close; no provider switch | PASS, fresh OpenSSL/Schannel plus accepted NSS evidence |
| Data then abrupt EOF | representative cross-provider | plaintext delivered before EOF | data preserved, then `TRUNCATED` | exactly one close | PASS, accepted failure matrices |
| Reciprocal close | all providers/pairs | both close-notify alerts observed | `CLOSED`, CLEAN | local alert alone never completes | PASS, fresh positive integrations and SS-6 |
| Readiness timeout/no progress | all providers/core | would-block, auxiliary wake, timeout | bounded pending state | no busy loop or corrupted state | PASS, deterministic readiness regression |
| Provider diagnostic | all providers | auth/policy/transport failure | normalized result/operation/reason/role/provider | native detail not required for control flow | PASS, diagnostic/provider regressions |
| Structured logging | all providers | success and negative paths | structured facts only | no key, DER, payload, password or native handle fields | PASS, logging regressions and source scan |
| External certificate stores | Schannel/OpenSSL fixtures | temporary CurrentUser CA/Root entries | exact owned removal | preexisting preserved; normal residue zero | PASS, provider regressions and fresh runner cleanup |
| Repeated mixed lifecycle | core/mock and providers | 100 alternating success/failure connections | runtime remains reusable | 100 creates/destroys/closes, no leak/double close | PASS, fresh deterministic plus provider init cycles |

## Evidence and closure decisions

- `test_security_lifecycle_negative` is compiled by VC6, Schannel, OpenSSL and
  Combined builds. It directly asserts transactional rejection before init,
  negative EXACT/ORDERED/AUTOMATIC selection, lazy initialization, ownership
  before/after acceptance, exactly-one-close, terminal pinning, deferred
  runtime destruction, reverse provider destruction and 100 mixed lifecycle
  cycles.
- Fresh real integrations passed OpenSSL TLS 1.2/TLS 1.3 and abrupt truncation,
  Schannel TLS 1.2 SYSTEM/CUSTOM trust with clean/abrupt closure, and NSS TLS
  1.2/TLS 1.3 mTLS/ALPN/25-byte echo/reciprocal shutdown using only the
  versioned NSS runtime.
- The bounded malformed corpus is deterministic, not a fuzzing claim. Generic
  constructors reject invalid pointer/count/enum/size forms; provider identity
  tests reject malformed DER, malformed PKCS#8, key mismatch and malformed
  chains without crashes or hangs.
- The public ABI never transfers provider-native pointers across CRTs. Public
  inputs are copied, outputs are fixed value records or PST-owned opaque
  handles, and each object is released by its matching PST function.
- Code/document scans found no claim that logical multi-connection sharing is
  general cross-thread safety. OpenSSL library initialization facts do not
  expand the PST threading contract.
- CurrentUser store cleanup completed with zero normal residue. ADR-0004's
  abrupt-process `CRASH_RESIDUE` limitation remains unchanged; PST never
  aggressively deletes externally owned certificates.

## Result

All mandatory SS-7 gates passed. No real production regression was reproduced.
Because SS-7 did not change the NSS/shared production runtime, the accepted
real NT4 SS-5 evidence remains applicable and a new NT4 run is not required.

