# Provider evolution

Status: Phase 8.B deterministic multi-backend core and selection matrix complete. No second production backend, public API change, or SPI version change was implemented. Phase 8.C backend metadata / production priority / transport genericization is next.

## SPI 2.3 audit

| SPI surface | Classification | Finding |
|---|---|---|
| descriptor ID/name/capabilities | generic; future limitation | Stable ID and capability snapshot are sufficient for selection; implementation/TLS-library version metadata is absent. |
| initialize/shutdown | generic | Provider-global acquisition/release; the core does not impose NSS single-instance behavior. |
| runtime create/destroy | generic | Per-runtime state supports different providers concurrently; no core singleton exists. |
| query/validate capabilities | generic | Separates provider ability from consumer requirements. Dynamic query can refine descriptor claims. |
| connection create/destroy | generic | Opaque provider state and exactly-once cleanup. |
| attach transport/ownership accepted | generic with current-adapter limitation | The ownership transition is provider-neutral. Current Win32 wrapper routes only to `retrozilla-nss`; transport kind/routing needs design before Schannel. |
| identity/TLS configuration | generic; legacy compatibility | `connection_configure_identity` carries frozen generic config, despite its historical name. Current DER credential shape may not cover native-store identities. |
| handshake/read/write | generic | Incremental normalized operations do not encode NSS calls or errors. |
| interest/wait | generic; potential limitation | READ/WRITE masks are provider-neutral and can map to PR_Poll, select, SSPI, OpenSSL WANT states, or engine buffers. The current public runtime always requires backend-managed wait. |
| shutdown step | generic | Already represents COMPLETE, FAILED, NEED_READ, NEED_WRITE and NEED_READ_WRITE; NSS merely completes locally in one step. |
| close kind | generic | Providers report CLEAN or TRUNCATED from authenticated TLS semantics; the core knows no NSS close-notify flag. |
| peer info | generic | TLS version, cipher, authentication flags, fingerprint and leaf DER are provider-neutral snapshots. Full chain remains out of scope. |
| diagnostic copy | generic, optional | Struct-size guarded value copy; provider-native domains remain private. |

SPI compatibility is major-compatible today: descriptor and vtable major 2 are accepted, required prefixes are size checked, and the diagnostic hook is read only when the full appended layout is present. The existing `PST_BACKEND_DESCRIPTOR_MIN_SIZE` uses the current whole descriptor size, so a future append must freeze the old prefix with an offset-based minimum. SPI 2.3 is sufficient for 8.B and an initial second-backend skeleton. No concrete SPI 2.4 hook is yet required.

## Selection, registration and lifecycle

Exact selection performs one stable-ID lookup. Ordered selection follows the caller's list and skips missing/incompatible candidates. Automatic selection currently follows in-process registration order. That is deterministic for a fixed registration sequence but is not yet a frozen provider-priority policy; before two production providers register, 8.B must freeze and test a documented built-in registration/priority order. There is no fallback after runtime selection and connection creation.

The registry is a fixed-capacity, pointer-retaining setup-time array. It is intentionally not thread-safe and must not mutate while runtimes use descriptors. Static/known providers remain the Phase 8 model; dynamic provider DLL loading is not required.

Each runtime owns its own descriptor, backend state, provider runtime state, capabilities, diagnostics and logger. Therefore backend A and B can be active simultaneously in the generic core. RetroZilla NSS's one-active-state rule lives inside its initializer and must not constrain other providers. The Phase 8.B matrix proves simultaneous A/B/C runtimes, two runtimes of one backend, release isolation, provider-local singleton failure, diagnostics, logging, ownership, readiness, shutdown, close classification and peer snapshots.

## Phase 8.B deterministic matrix

The VC6 test test_multi_backend registers three providers with distinct capabilities. It proves exact, ordered, and automatic selection; missing/incompatible candidates; initialization and runtime-create fallback; all-candidate failure; and no reselection after connection failure. AUTOMATIC selects the first compatible provider in registration order. Production priority remains an explicit 8.C decision.

It also covers three simultaneous providers, two runtimes of one provider, provider-local singleton failure, target subsets, the eight-entry registry bound, duplicate/invalid IDs, larger descriptors, minimum vtable rejection, absent diagnostic_copy, and a valid legacy vtable prefix. The prefix case exposed an unconditional read of appended identity, peer, and ALPN hooks. The core now checks size, capability, and null hooks and reports UNSUPPORTED when optional service is absent. This restores SPI 2.x prefix compatibility without a version bump.

Isolation covers separate log sinks, copied rejected-candidate diagnostics, ownership counters, different readiness and shutdown progressions, clean versus truncated close, and provider-distinct peer snapshots. Mismatched transport/backend IDs remain rejected without ownership transfer. Win32 transport genericization remains for 8.C/8.D.

VC6 C89 /W4 completed with zero warnings. The portable, SPI, NSS, TLS policy, diagnostic, logging, lifecycle, and multi-backend tests pass. NSS runtime, failure, and lifecycle integrations build. Existing NT4 evidence is reused because NSS behavior did not change.

## Provider-neutral boundaries and gaps

Capability bits cover TLS 1.2/1.3, ALPN, client authentication, custom/system trust, hostname verification, peer info, resumption, early data, nonblocking operation and backend wait. Server authentication and cipher metadata are expressed through trust/hostname policy and peer-info rather than independent bits. No new capability is justified without a candidate implementation need.

The public Win32 factory accepts a socket as `pst_size`, so no Winsock type escapes, but the internal transport wrapper embeds `backend_id="retrozilla-nss"` and an NSS-native transport record. That is the main accidental first-provider assumption. Phase 8 must decide whether one generic Win32 socket transport can expose a neutral native record to multiple providers, or whether provider-specific factories remain explicit. It must not export NSS/SSPI handles or loosen ownership.

Readiness is provider-neutral at the SPI boundary. A provider may wait on its encrypted descriptor, WinSock readiness, SSPI input/output buffers, OpenSSL WANT_READ/WANT_WRITE, or BearSSL engine state. Buffered progress may return an immediate relevant interest/result, but must retain Phase 7 anti-spin rules. Shutdown is already incremental. Clean/truncated classification remains the provider's authenticated responsibility.

Trust source kinds already express custom CA and system trust. Credentials currently express copied leaf DER plus unencrypted PKCS#8 DER; native certificate-store identities, PKCS#12, password callbacks and hardware handles remain future extensions unless the selected provider proves them necessary. Public diagnostics/logging require no change for a new provider.

## Metadata recommendation

Keep these concepts separate: PST API version, PST library version, SPI version, stable backend ID, backend implementation version, and underlying TLS library/OS provider version. Add no arbitrary version string now. In 8.C design a size/versioned runtime metadata record with bounded copied fields and structured numeric components plus explicit availability flags. For an OS provider, underlying version may be unavailable or represented as OS-backed rather than fabricated.

Do not rename `retrozilla-nss`. Future IDs should be stable implementation identities such as `schannel`, `openssl`, `bearssl`, and a distinct modern NSS ID. If a naming migration is ever required, use an explicit alias/deprecation policy rather than silently changing selection identity.

## Candidate assessment

| Candidate | Effort | Architectural value | TLS/features | Legacy/build/licensing assessment |
|---|---|---|---|---|
| Schannel | high | highest: different trust, lifecycle and SSPI readiness | Modern Windows supports TLS 1.2/1.3, ALPN, system trust and mTLS; custom trust requires deliberate validation design | Modern-Windows-only is acceptable; no bundled TLS runtime; Windows SDK/toolchain split required. |
| OpenSSL | medium-high | high portability and WANT_READ/WANT_WRITE proof | TLS 1.2/1.3, ALPN, custom trust and mTLS are mature | Adds runtime/dependency and modern toolchain; system trust is not naturally Windows-native; current releases use Apache-2.0. |
| modern NSS | medium | medium: validates lineage evolution but resembles existing provider | TLS 1.2/1.3, ALPN, custom trust and mTLS | Newer build/runtime likely; MPL-2.0 lineage; lower power to expose NSS assumptions. |
| BearSSL | medium | high state-machine/embedded diversity | TLS 1.2, ALPN, X.509 and client certificates; no TLS 1.3 | Compact C/static model and MIT license, but cannot satisfy the current full TLS 1.3 proof and is not the primary Phase 8 provider. |

Ranked recommendation: (1) Schannel, because it most strongly tests provider independence and system trust on modern Windows; (2) OpenSSL, for portable nonblocking diversity and complete modern TLS; (3) modern NSS, as a lower-risk lineage-evolution fallback. BearSSL is valuable for a later capability-limited/embedded provider, not as the principal Phase 8 proof.

## Minimum second-provider proof

The selected production provider must prove runtime/connection lifecycle, socket transport ownership, TLS 1.2 and TLS 1.3 when advertised, server authentication, hostname verification, at least one explicit trust model, ALPN, incremental read/write/readiness/shutdown, clean/truncated close, peer info, diagnostics and failure isolation. Capability sets need not be identical. For the recommended Schannel provider, mTLS is a Phase 8 closure gate because it proves the existing credential/identity contract across a materially different provider; a provider lacking it may still be valid but cannot alone close that cross-provider identity gate.

## Phase 8 plan

1. **8.A - SPI/provider-neutral audit:** this document; complete.
2. **8.B - Deterministic multi-backend core matrix:** complete; selection, fallback, concurrency, isolation, optional hooks, prefix compatibility and registry bounds are covered.
3. **8.C - Backend metadata / production priority / transport genericization:** next; design first, then add only justified size/versioned API or SPI extensions with version bumps.
4. **8.D - Transport routing and Schannel skeleton:** preserve generic ownership; introduce separate modern build output; no TLS behavior claim yet.
5. **8.E - Schannel TLS/readiness/close:** TLS 1.2/1.3 where OS supports them, ALPN, I/O, incremental SSPI state and clean/truncated classification.
6. **8.F - Trust/identity/peer info:** system/custom trust decision, hostname, mTLS and normalized peer snapshot/diagnostics.
7. **8.G - Cross-backend interoperability/regression:** selection and functional matrices while preserving the VC6/NT4 RetroZilla NSS provider.
8. **8.H - Phase 8 closure audit.**

Current versions remain API 1.2.0, library 0.3.0 and SPI 2.3. The internal Phase 8.B matrix and prefix-safety correction cause no bump. A public additive metadata record requires an API/library minor bump; an appended backend hook/descriptor contract requires an SPI minor bump; internal tests or a provider implementation behind 2.3 require neither automatically.

Keep current `build/vc6` outputs unchanged. New providers/toolchains must use non-overlapping provider/platform directories, for example `build/win64-modern-msvc` for Schannel. Do not reorganize existing artifacts for aesthetics.

RetroZilla NSS remains first-class: every Phase 8 production step must preserve VC6/NT4, TLS 1.2/1.3, mTLS, ALPN, readiness, close classification and lifecycle regressions. Platform-specific providers are valid and need not run on NT4.

Phase 9 retains final ABI freeze, packaging, certification and semantic-version policy. Separate RetroZilla NSS source/revision/patchset/build provenance work should happen before Phase 8 closure, ideally before broad second-provider integration makes the lineage harder to isolate; it is not part of 8.A or a blocker for 8.B.

