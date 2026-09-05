# Phase 8 closure audit

Status: **Phase 8 - Multiple Backends / Provider Evolution: complete**. Phase 9 has not started.

## Subphase evidence

8.A froze the provider-neutral architecture and identified the legacy assumptions. 8.B proved exact, ordered and automatic selection, capability filtering, all-incompatible failure, same/different-provider runtimes, provider-local singleton behavior, diagnostics, logging, ownership, readiness and shutdown isolation. 8.C added append-only internal metadata under SPI 2.4, explicit target priority and a neutral Win32 native transport. 8.D created the isolated modern x64 build and Schannel skeleton. 8.E implemented real incremental Schannel TLS 1.2, secure I/O, readiness, close and shutdown. 8.F completed system/custom trust, hostname validation, ALPN, explicit DER/PKCS#8 mTLS, peer snapshots, logging gates and B1/C/A/B2 same-runtime isolation. 8.G proved the common cross-provider semantics and retained all legacy regressions. The separate RetroZilla NSS provenance task preserved exact source, patch, SDK, runtime, manifests, notices and build procedure.

No later evidence contradicts a completed subphase and no mandatory gate remains open.

## Providers, targets and selection

`retrozilla-nss` and `schannel` are independent first-class providers; neither replaces nor hides the other. The source tree supports both, while packages intentionally select their provider set:

- NT4/Win32 x86/VC6: `retrozilla-nss`, output `build/vc6`.
- Modern Windows x64/MSVC: `schannel`, output `build/win64-modern-msvc`.

Schannel sources, SDK types and libraries are absent from the VC6 target. RetroZilla NSS headers/runtime are absent from the modern target. A future combined target must declare providers in an explicit manifest order.

Automatic selection means the first declared/registered available provider satisfying every required capability. It contains no version, performance or "best" heuristic. Exact and ordered selection override automatic order. Runtime/connection failure after selection does not trigger silent reselection or downgrade.

The core supports concurrent runtimes and different providers. RetroZilla NSS's one-active-state rule is provider-local. Internal SPI 2.4 metadata reports stable identity and adapter/provider components: NSS 3.42 Beta and NSPR 4.7.7 for the legacy adapter; Schannel reports adapter/provider identity without inventing an OS Schannel semantic version. Public metadata remains deferred to Phase 9.

`PST_BACKEND_DESCRIPTOR_MIN_SIZE` ends at the required legacy `vtable` field using `offsetof + sizeof`. Larger descriptors and permitted legacy prefixes are accepted; appended fields/hooks are read only after `struct_size` checks. No post-8.C layout change requires SPI 2.5.

The internal `PST_NATIVE_TRANSPORT` identifies the native transport kind, not a TLS backend. Shared Win32 transport contains no `backend_id="retrozilla-nss"` or name routing. Backend identity comes from the selected descriptor/runtime. Before `ownership_accepted`, the caller/core owns the transport; after acceptance, the provider owns and closes it exactly once.

## Capability and semantic matrix

| Provider | Mask | Capabilities |
|---|---:|---|
| RetroZilla NSS | `0x00000e5f` | TLS 1.2/1.3, custom trust, hostname, ALPN, client auth, peer info, nonblocking, backend wait |
| Schannel | `0x00000e7d` | TLS 1.2, system/custom trust, hostname, ALPN, client auth, peer info, nonblocking, backend wait |
| Intersection | `0x00000e5d` | TLS 1.2, custom trust, hostname, ALPN, client auth, peer info, nonblocking, backend wait |

TLS 1.2 with root/intermediate/leaf custom trust, hostname, ALPN, explicit mTLS and bidirectional I/O passed for both providers. NSS TLS 1.3 passed. The tested Schannel runtime cannot complete TLS 1.3, does not advertise it, and a TLS-1.3 requirement rejects it rather than downgrading. Schannel system trust has positive and negative evidence; NSS reports it unsupported. Custom trust is exclusive and a wrong root cannot be rescued by system trust.

Correct hostname passes and a wrong hostname yields `HOSTNAME_MISMATCH`. Required missing ALPN yields `POLICY_VIOLATION`; optional missing ALPN succeeds and queries as `UNAVAILABLE`; multiple offers and selected-protocol normalization pass. Both use explicit copied certificate DER and private-key PKCS#8 DER. Missing/wrong identities fail closed; Schannel disables automatic client-certificate selection and proves temporary key cleanup, while NSS has no hidden identity source.

Peer snapshots normalize TLS version, standard numeric cipher suite, authentication flags, ALPN, SHA-256 fingerprint and owned leaf DER. They contain no native pointer and survive connection destruction. Both happened to negotiate `0xC030` in the shared fixture; equal ciphers are not a policy requirement.

Authenticated close-notify produces `CLOSED/CLEAN`; application data is preserved before clean close; raw EOF without close-notify produces `FAILED/TRUNCATED`. Classification is authenticated provider evidence, not a socket-HUP heuristic. NSS may use `PR_Poll` and `NEED_READ_WRITE`; Schannel uses bounded WinSock readiness. Both preserve partial I/O, bound progress, prevent spin, isolate per-connection progress, make fatal wait terminal and implement bounded release-safe shutdown.

Public state is consistent: successful handshake becomes ESTABLISHED; clean close becomes CLOSED; truncation and fatal protocol/policy/auth/transport/wait failures become FAILED; terminal state cannot resurrect. Normalized results cover OK, UNSUPPORTED, TRANSPORT_FAILURE, PROTOCOL_FAILURE, AUTH_FAILURE, HOSTNAME_MISMATCH, POLICY_VIOLATION, TRUNCATED, CLOSED and BACKEND_FAILURE without fabricating specificity. A rare Schannel wrong-client alert/close race may honestly expose only generic SSPI terminal failure and normalize as `BACKEND_FAILURE`; a previously observed specific TLS failure still outranks later cleanup errors.

## Diagnostics, logging, lifecycle and security

Diagnostics expose normalized result, operation, selected backend ID and copied generation/value semantics. Native NSS/NSPR/Win32/SSPI errors remain internal; snapshots survive source destruction and there is no global last-error.

Papinho Logging Levels v1 remain OFF=0, ERROR=1, WARN=2, INFO=3, DEBUG=4 and TRACE=5. Both providers use the common observational sink. Public logs/diagnostics contain no password, token, private key, DER, trust data, hostname or ALPN text, native code/string, endpoint, handle, pointer, payload or Schannel key-container name. Explicit peer-info is a separate requested functional API.

Creation, destruction, ownership, credentials, trust, diagnostics and logging are balanced and isolated. Schannel passed B1-valid/C-wrong/A-system/B2-valid in one runtime with four balanced connections and no residual key. NSS passed failure/success lifecycle cycles and the Phase 7 bounded stress suite. Schannel also passed 100 provider lifecycle cycles, repeated functional connections, deterministic partial-buffer tests and the four-connection isolation sequence; a larger soak is optional, not a Phase 8 blocker.

Provider-neutral security review found no policy/capability mismatch, selection fallback, trust/credential crossover, plaintext fallback, protocol widening, TLS downgrade, trust/hostname/ALPN bypass, automatic credential disclosure, close misclassification, double ownership, terminal resurrection or secret logging.

## Provenance, builds and versions

RetroZilla revision `2f274574d3c6ee8769914046920d649bbae9f81b`, NSS 3.42 Beta, NSPR 4.7.7 and patch `0001-win32-secure-rng-fail-closed-nt4.patch` are preserved under `third_party/retrozilla-nss`. Exact source, generated SDK, canonical runtime, SHA-256 manifests, original notices and reproduction documentation are present. `REQUIRED_UNIQUE_FILES_IN_PSTW=0`, active `C:\PSTW` references are absent and reproducibility remains Level B; byte-identical rebuild is not claimed.

Clean closure builds:

- VC6 x86: C89, `/W4`, portable/multi-backend/policy/diagnostic/logging/lifecycle and NSS tests PASS, zero warnings.
- modern MSVC x64: `/MD /W4`, public-header consumer, Schannel provider/lifecycle/ownership and deterministic TLS buffer/readiness tests PASS, zero warnings.

Existing real NT4 evidence remains valid. Phase 8 generalized internal transport/selection and kept the NSS ABI/behavior/runtime intact; subsequent VC6 and NSS regression matrices passed. No new NT4 retest is outstanding.

API remains 1.2.0, library remains 0.3.0 and SPI remains 2.4. A library 0.4.0 bump may be appropriate for a release containing the second production provider, but release/version policy belongs to Phase 9; closure does not change it automatically.

## Production-change audit and limitations

Core changes added deterministic multi-provider registration/selection, capability validation and safe optional-hook handling. SPI gained append-only internal metadata. Transport routing became provider-neutral. RetroZilla NSS retained provider-private import/readiness/close behavior and gained no unrelated feature. Schannel added the second real provider and its modern TLS/trust/identity implementation. All changes directly support Phase 8.

Known non-blocking limitations: Schannel TLS 1.3 is unavailable on the tested runtime; NSS system trust is unsupported; NSS permits one active provider state; the rare Schannel wrong-client terminal race may yield `BACKEND_FAILURE`; system-trust positive evidence uses an external already-trusted endpoint; NSS byte-identical rebuild is unproved; Windows 2000/XP and 95/98 are not currently validated; no POSIX provider exists.

Deferred work includes OpenSSL, modern NSS, BearSSL, POSIX/adapters, dynamic plugins, public backend metadata, public full-chain API, dynamic logging levels, ECDSA expansion, additional OS certification and a larger Schannel soak. These were not Phase 8 requirements.

## Decision

Phase 8 achieved multiple backends/provider evolution: generic multi-backend core, deterministic selection, metadata, explicit target provider sets, neutral transport, a second real provider, cross-provider semantic validation, legacy preservation and reproducible NSS provenance. Mandatory blockers: none.

The next action is a user decision, not an automatic implementation: add another backend now; close provider expansion for now and proceed to Phase 9; or defer additional providers to community/future work.
