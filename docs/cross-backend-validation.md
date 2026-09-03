# Cross-backend validation

Status: Phase 8.G complete, with the post-Phase-8 OpenSSL SYSTEM_TRUST follow-up incorporated. This matrix compares public PST semantics, not provider implementation details.

## Capability boundary

| Provider | Mask | Decoded capabilities |
|---|---:|---|
| RetroZilla NSS | `0x00000e5f` | TLS 1.2, TLS 1.3, client auth, ALPN, custom trust, hostname verification, peer info, nonblocking, backend wait |
| Schannel | `0x00000e7d` | TLS 1.2, client auth, ALPN, custom trust, system trust, hostname verification, peer info, nonblocking, backend wait |
| OpenSSL | `0x00000e7f` | TLS 1.2, TLS 1.3, client auth, ALPN, custom trust, system trust, hostname verification, peer info, nonblocking, backend wait |
| Three-provider intersection | `0x00000e5d` | TLS 1.2, client auth, ALPN, custom trust, hostname verification, peer info, nonblocking, backend wait |

TLS 1.3 is shared by NSS and OpenSSL. System trust is shared by the two modern Windows providers, Schannel and OpenSSL, but is not a three-provider common capability because NSS does not advertise it. Exact selection rejects an unavailable or capability-incompatible provider without substitution; ordered and automatic selection retain the frozen generic-core rules.

## Canonical semantic matrix

| Case | RetroZilla NSS | Schannel | Classification/verdict |
|---|---|---|---|
| TLS 1.2 custom-trust baseline | TLS `0x0303`, authenticated, 25-byte echo | TLS `0x0303`, authenticated, 25-byte echo | COMMON / TESTED / EQUIVALENT |
| Explicit mTLS | server `AUTH=True` | server `AUTH=True`, exact identity fingerprint | COMMON / TESTED / EQUIVALENT |
| Custom root/intermediate/leaf | PASS | PASS | COMMON / TESTED / EQUIVALENT |
| Wrong custom CA | `AUTH_FAILURE`, terminal | `AUTH_FAILURE`, terminal | COMMON / TESTED / EQUIVALENT |
| Wrong hostname | `HOSTNAME_MISMATCH` | `HOSTNAME_MISMATCH` | COMMON / TESTED / EQUIVALENT |
| ALPN required, selected | `fixture/1` | `fixture/1` | COMMON / TESTED / EQUIVALENT |
| ALPN required, absent | `POLICY_VIOLATION`, no resurrection | `POLICY_VIOLATION`, no resurrection | COMMON / TESTED / EQUIVALENT |
| ALPN optional, absent | established, query `UNAVAILABLE` | established, query `UNAVAILABLE` | COMMON / TESTED / EQUIVALENT |
| Multiple ALPN | server selection returned from offered set | `fixture/2` established evidence | COMMON / TESTED; selection is server-driven |
| Missing client credential | `AUTH_FAILURE`, no hidden identity | `AUTH_FAILURE`, no default identity | COMMON / TESTED / EQUIVALENT |
| Wrong valid client identity | server rejects; `AUTH_FAILURE` where alert observed | server rejects; normally `AUTH_FAILURE` | COMMON / TESTED; see error-race note |
| Clean close | `CLOSED/CLEAN` | `CLOSED/CLEAN` | COMMON / TESTED / EQUIVALENT |
| Data then clean close | payload retained, then `CLOSED/CLEAN` | plaintext remainder/clean-close evidence | COMMON / TESTED / EQUIVALENT |
| Abrupt TCP close | `FAILED/TRUNCATED`, READ diagnostic | `FAILED/TRUNCATED` | COMMON / TESTED / EQUIVALENT |
| Protocol mismatch/non-TLS | fail-closed, terminal | fail-closed, terminal | COMMON / TESTED; exact native stage differs |
| Readiness | bounded `PR_Poll`, possible READ_WRITE | bounded `select`, READ/WRITE | COMMON / TESTED / EQUIVALENT semantics |
| Partial output/input | no duplication/loss | partial output/input and `SECBUFFER_EXTRA` PASS | COMMON / TESTED / EQUIVALENT contract |
| Shutdown | bounded provider completion | bounded close-token drain | COMMON / TESTED / EQUIVALENT contract |
| Release without shutdown | bounded, exactly-once close | bounded, exactly-once close | COMMON / TESTED / EQUIVALENT |
| Ownership | caller until accepted; provider afterward | same | COMMON / TESTED / EQUIVALENT |
| Peer info | normalized TLS/cipher/auth/cert fields | same normalized fields | COMMON / TESTED / EQUIVALENT |
| TLS 1.3 requirement | supported and real PASS | rejected before handshake | CAPABILITY-DIFFERENT |
| System trust requirement | unsupported | positive and negative real evidence | CAPABILITY-DIFFERENT; OpenSSL also passes post-Phase-8 SYSTEM evidence |
| NSS process singleton | provider-local restriction | not imposed | BACKEND-SPECIFIC |

## Fresh common baseline

Both clients used the same generated `root -> intermediate -> localhost leaf`, the same explicit client DER/PKCS#8 identity, TLS 1.2-only policy, custom trust and ALPN `fixture/1`.

RetroZilla NSS, loaded exclusively from the versioned runtime, reported TLS `0x0303`, cipher `0xC030`, `WRITE=25`, `READ=25`, `CONTENT_MATCH=1`, ALPN size 9 and authenticated peer. The server reported `AUTH=True`, `ALPN=fixture/1`, `RECV=25`, `SEND=25`.

Schannel reported TLS `0x0303`, cipher `0xC030`, the same 25-byte echo, `fixture/1`, authenticated peer flags, SHA-256 size 32, owned leaf DER and snapshot-after-destroy. The server confirmed the expected client identity fingerprint. Because both servers loaded the same `server-chain.pem`, the leaf DER and its SHA-256 are identical by fixture provenance; public snapshot ownership and hashing were independently tested for both providers.

## State, diagnostics, logging, and lifecycle

Both providers expose ESTABLISHED after success, CLOSED/CLEAN after authenticated close, and FAILED/TRUNCATED after unauthenticated EOF. Fatal policy, authentication, protocol and wait failures are terminal and reject resurrection. Diagnostics expose only valid, normalized result, operation and backend ID; copied snapshots outlive connections. Backend IDs intentionally differ.

OFF emits zero events. INFO/TRACE provide bounded structured progress, and representative failures emit one logical ERROR. Exact progress counts differ. Fixed public events disclose no hostname, ALPN text, DER, key, trust data, native code/string, endpoint, payload, pointer, handle or provider key-container. Logging does not alter remote security outcome.

Schannel's B1/C/A/B2 single-runtime sequence and NSS lifecycle/stress evidence prove failure-then-success isolation. Generic same-process multi-provider registry isolation is covered with mocks. The modern combined tests also prove Schannel/OpenSSL selection and independent release; production target packaging remains deliberate. NSS's provider singleton is not elevated into a generic PST restriction.

## Schannel alert/close race

One wrong-client run returned `BACKEND_FAILURE` while the server reported certificate rejection; bounded repetitions returned `AUTH_FAILURE`. The observable SSPI result can race between a specific certificate/TLS-alert status and a generic terminal provider status when the peer closes immediately after sending the alert. There is no reliable earlier specific status in the generic-status branch. Mapping every generic SSPI failure to authentication would misclassify unrelated backend failures, so no production change was made. Precedence remains: preserve and publish a specific status when one was actually observed; otherwise retain the honest generic failure. Cleanup never overwrites an already captured root diagnostic.

## Target and security invariants

Legacy packages ship RetroZilla NSS; modern x64 builds ship Schannel. Cross-backend validation uses separate artifacts and does not require either target to ship both. Both remain fail-closed with no plaintext fallback, downgrade, trust/hostname/ALPN bypass, automatic credential leak, terminal resurrection, native diagnostic disclosure or secret logging.

The canonical VC6 `/W4` and modern MSVC x64 `/MD /W4` suites pass with zero warnings. No shared/core/NSS/Schannel production behavior changed in 8.G, so existing real NT4 evidence remains applicable and no NT4 rerun is required.
