# Error and diagnostic model hardening

Status: Phase 7.A in progress. Phase 7.A5 exposes the first structurally redacted public diagnostic value ABI; Phase 7.A7 defines, but does not implement, a consumer-controlled event sink. TLS behavior and remote-visible behavior are unchanged, and the internal SPI remains 2.3.

## Policy

Local diagnostics can be specific; remote disclosure must be minimal. Backend-native conditions are normalized to `PST_RESULT` for portable control flow. A future local diagnostic snapshot may retain safe native and contextual detail, but must never drive portable branching or be serialized to the peer. PST does not currently send result strings, native numeric codes, filesystem paths, trust-store details, credential presence, or state-machine internals to a remote peer.

## Public result inventory

The public result space is contiguous from 0 through 15: `OK`, `INVALID_ARGUMENT`, `INVALID_STATE`, `UNSUPPORTED`, `UNAVAILABLE`, `OUT_OF_MEMORY`, `RESOURCE_FAILURE`, `TRANSPORT_FAILURE`, `PROTOCOL_FAILURE`, `AUTH_FAILURE`, `HOSTNAME_MISMATCH`, `POLICY_VIOLATION`, `BACKEND_FAILURE`, `TRUNCATED`, `CLOSED`, and `INCOMPATIBLE_API`. `pst_result_string` provides one stable generic English string for every defined value and `unknown result` outside the range.

These values are normalized categories, not backend-native error numbers. Operation APIs additionally return incremental operation state; I/O returns bytes transferred, operation, close kind, and normalized error.

## NSS/NSPR mapping audit

| Native/backend condition | Normalized result | Local detail today | Remote-visible effect | Ambiguity or improvement |
|---|---|---|---|---|
| `PR_WOULD_BLOCK_ERROR` | `OK` plus NEED_READ/WRITE state | Private last error and optional trace | None | Correctly represents progress, not failure |
| `PR_CONNECT_RESET_ERROR`, `PR_END_OF_FILE_ERROR` | `TRUNCATED` | Private last error | Connection terminates | Reset and EOF collapse intentionally; clean-vs-truncated context needs audit |
| `PR_IO_ERROR`, network unreachable, connect aborted/refused, host unreachable | `TRANSPORT_FAILURE` | Private last error | Transport failure/close | Specific local cause is unavailable publicly |
| `SSL_ERROR_BAD_CERT_DOMAIN` | `HOSTNAME_MISMATCH` | Private last error | TLS handshake rejection | Adequate portable category |
| Bad/revoked/expired/unsupported/unknown certificate alerts and SEC trust/certificate failures | `AUTH_FAILURE` | Private last error | TLS authentication rejection/alert selected by NSS | Trust, expiry, revocation, signature, and missing credential are locally ambiguous |
| `SEC_ERROR_NO_MEMORY` | `OUT_OF_MEMORY` | Private last error | Connection fails | Adequate category |
| Other `IS_SSL_ERROR` values | `PROTOCOL_FAILURE` | Private last error | TLS failure/alert selected by NSS | Version, cipher, ALPN, and malformed-record causes collapse |
| Other `IS_SEC_ERROR` and unknown native values | `BACKEND_FAILURE` | Private last error | Generic failure | Broad fallback; local diagnostic needed |
| `PR_POLL_ERR` or `PR_POLL_NVAL` | `TRANSPORT_FAILURE` | Poll trace only | Connection fails | No native error captured for a consumer |
| `PR_POLL_HUP` | `CLOSED` | Poll trace only | Connection closes | Buffered-data and clean/truncated distinction remains contextual |
| WinSock `FIONBIO` failure | `TRANSPORT_FAILURE` | WSA code stored privately | No PST detail sent | Native code inaccessible to consumer |
| DLL load failure | `UNAVAILABLE` | `GetLastError` stored privately | Runtime selection fails locally | `pst_runtime_create` may later collapse this to `UNSUPPORTED` |
| NSS initialization/shutdown failure | `BACKEND_FAILURE` | Private backend last error | Runtime fails locally | Cause is lost across public runtime selection |
| Credential/trust import failures | `AUTH_FAILURE` or `BACKEND_FAILURE` | Several paths do not capture native error | Handshake/configuration fails | Needs consistent capture before any public diagnostic design |
| Required ALPN unavailable/no overlap | `UNAVAILABLE` or protocol failure by path | No public native detail | TLS negotiation fails | Same policy failure can surface through different paths |

`pst_backend_nss_last_error` and per-state `last_error` storage are private implementation/testing mechanisms. `PST_NSS_TRACE_FILE` is an opt-in backend trace that can contain module paths and native codes; it is not a stable consumer API and must remain local-only.

## Loss and ambiguity points

- `pst_runtime_create` tries candidates and ultimately returns `UNSUPPORTED`, discarding whether a candidate was unavailable, failed initialization, lacked capability, or failed runtime creation.
- Public connection operation errors preserve only `PST_RESULT`; native codes are not retained by the portable connection object.
- Several identity, ALPN, poll-flag, NSS lifecycle, and loader paths either do not capture a native error or keep it only in backend-private state.
- `AUTH_FAILURE` intentionally protects portability but cannot locally distinguish trust anchor, expiry, revocation, bad signature, peer alert, or absent client credential.
- `PROTOCOL_FAILURE` intentionally combines TLS version, cipher, ALPN, record, and other protocol failures.
- Timeout is represented by `PST_WAIT_RESULT.timed_out`, not a `PST_RESULT`; callers must combine both surfaces.
- Generic public result strings are suitable for stable summaries, not detailed administration or troubleshooting.

## Version and backend diagnostics

Already available separately:

- library and API versions through `pst_get_version`, `pst_library_version`, and `pst_api_version`;
- backend ID and capabilities through `pst_runtime_get_info`;
- negotiated TLS version through `PST_PEER_INFO_SUMMARY`.

A limited structured local snapshot is now available publicly through `PST_DIAGNOSTIC_INFO`. Backend implementation version remains unavailable and distinct provider metadata; it is not duplicated into error snapshots.

## Backend naming

The stable current ID is `retrozilla-nss`. A future name `nss-modern` would use a different ordering. Family-plus-variant (`nss-retrozilla`, `nss-modern`) is the preferred convention to evaluate, but the current ID must not be renamed before auditing runtime selection, tests, logs, documentation, compatibility promises, and release stability. An alias/versioning strategy may be safer than replacement.

## Next 7.A step

Design and test an internal-only diagnostic snapshot first: normalized result, operation phase, backend ID, optional native domain/code, and safe local flags. Define capture/reset/lifetime rules and redaction before considering any public API. Remote behavior and TLS alerts must remain unchanged.
## 7.A1 internal diagnostic snapshot

Diagnostic is structured local data; logging is a later optional presentation mechanism owned by the consumer. `PST_RESULT` remains portable control flow. The internal `pst_internal_diagnostic` stores normalized result, real operation phase, native domain and primary/secondary codes, classification flags, generation, validity, and a lifetime-safe copied backend ID. It contains no native pointers, arbitrary strings, paths, hostname, certificate/DER, application payload, or credential material.

The snapshot is embedded and requires no allocation on failure. Initialization produces an empty generation zero. A relevant failure replaces all fields and increments generation. Successful completion/reset clears validity and all detail while incrementing generation, preventing stale failure A from being attributed to successful operation B. Native errors are captured only immediately after a documented failing NSS/NSPR/WinSock operation; `PR_GetError` is never sampled after success and `PR_WOULD_BLOCK_ERROR` is progress state rather than a diagnostic failure.

Initial NSS integration covers DLL/NSS initialization, WinSock transport attach, handshake (with authentication and hostname phase refinement), poll/wait, secure read, secure write, and shutdown. The core internal header exposes the backend-neutral facility for native-domain-NONE diagnostics without changing the public header or SPI. Deterministic tests cover empty state, normalized-only capture, native domains/codes, replacement, reset, copied backend ID, two AUTH causes sharing one `PST_RESULT`, and preserved protocol/hostname/transport distinctions.

Future logging remains consumer-owned and optional: a browser, accelerator, mail client, or minimal consumer may choose its own sink or no logging. No logger, automatic file, severity API, public ABI, TLS alert, or remote disclosure is introduced by 7.A1.

## 7.A2 internal transport, copy, and redaction

Runtime and connection core objects now embed separate diagnostic values. NSS backend-global, backend-runtime, and per-connection states likewise retain separate snapshots; one connection cannot overwrite another. After each backend connection operation, the core uses the optional internal SPI 2.3 `diagnostic_copy` hook to copy the backend-owned value. The core does not interpret or remap native codes. Core-originated connection errors use domain `NONE` and native code zero.

`pst_diagnostic_copy` preserves every field, including validity and generation, and is safe for self-copy. A copied value owns all of its bytes and remains valid after the source connection/state is destroyed. Capture and clear are the only operations that increment generation; copy preserves it. Clear increments generation, invalidates the snapshot, and zeros result detail, phase, domains, codes, flags, and backend ID. A relevant successful backend operation clears only its own context. `PR_WOULD_BLOCK_ERROR`/NEED states and `PST_WAIT_RESULT.timed_out` remain progress outcomes and do not create failure diagnostics.

Redaction is structural: the snapshot contains only normalized result, numeric phase/domain/codes/flags/generation/validity and the validated copied backend ID. It has no fields for hostname, paths, DLL paths, certificates, keys, passwords, tokens, payload, peer-controlled text, environment values, or native error strings. Future optional logging/event code may receive a safe copy, never a transient backend pointer. Diagnostic remains distinct from logging, public control flow remains `PST_RESULT`, and remote disclosure/wire behavior are unchanged.

Deterministic tests cover valid/invalid/self-copy, generation preservation and reset, replacement, independent contexts, source-destruction lifetime, backend ID/domain/code preservation, core `NONE/0`, distinct NSS native causes, successful clearing, would-block, timeout, phase association, runtime/connection separation, and the redacted fixed schema.

## 7.A3 pre-runtime diagnostic retention

Constructor failures can precede the lifetime of the object that would normally own their diagnostic. The internal solution is an explicit stack/value-like `pst_internal_operation_context`, passed to internal transactional creation functions. Public constructors remain unchanged wrappers and discard their private context for now. There is no process-global, static, singleton, thread-local, `errno`-like, or `pst_get_last_error` state.

### Constructor/failure-path matrix

| Constructor/path | Class | Current diagnostic finding |
|---|---|---|
| runtime option validation / exact ID not found | A/B | Core-only structured failure; no native error exists and no runtime survives. The operation context retains phase `BACKEND_SELECT` or `RUNTIME_CREATE`, domain `NONE`, code zero. |
| backend selection, initialize, capability query and backend runtime creation | B | A runtime may not survive. The operation context retains the candidate diagnostic before immediate cleanup. |
| config creation | A | Validation/allocation only; no backend/native diagnostic exists and no object survives OOM. The explicit context pattern can be reused later without a second mechanism. |
| credentials/trust creation | A | Memory copy, validation and OOM paths only. They deliberately retain no DER/key/path/text diagnostic; public API is unchanged. |
| config identity/TLS setters and freeze | C | The config survives failure. Failures are normalized policy/validation/OOM results; no backend native error is produced. |
| connection creation/configuration | B | Backend connection creation or identity configuration can fail before a public connection survives. The same operation-context pattern now retains runtime/connection backend detail before cleanup. |
| peer-info creation | B/C | The live connection survives, but the newly requested peer-info object may not. Its backend operation can use the connection diagnostic transport in a future focused integration; no separate mechanism is required. |
| Win32 transport wrapper creation | A/D | Only argument/allocation failure occurs before the wrapper exists; it performs no socket syscall/import. Native `SOCKET` ownership remains unchanged. Native import occurs later during connection attach, where the connection survives and already owns a diagnostic. |

Creation is transactional: no failed public object escapes. A backend `initialize` failure may return an opaque cleanup-only state; the core copies its diagnostic through SPI 2.3 and immediately calls `shutdown`. The state never escapes, and the copied snapshot remains valid after cleanup. A backend that returns no failure state still receives a normalized domain-`NONE` fallback diagnostic.

Each internal creation call initializes its explicit context at generation zero. Every candidate failure is captured into that context and increments its local generation. Copy preserves generation. Explicit reset increments generation and invalidates detail. Generation orders replacements only within the originating context; it is not globally unique.

Selection precedence is deliberately simple. Exact selection retains its one failure. Ordered and automatic selection replace the active diagnostic for each declared/registry-order candidate, so if all candidates fail the final attempted candidate is retained. OOM in core allocation returns immediately and takes precedence. If a later candidate succeeds, the context is cleared and the returned runtime has no stale failure; rejected candidates may only become future optional trace/events, never the active runtime diagnostic.

The operation context contains only the existing redacted snapshot. It allocates nothing, retains no backend/runtime pointer and stores no path, environment content, native text, credentials, trust data, hostname, certificate, payload or peer-controlled text. Diagnostic remains distinct from logging, and public/wire behavior is unchanged.

## 7.A4 controlled exposure decision

The public boundary audit is recorded in [diagnostic-api-design.md](diagnostic-api-design.md). No API is added in 7.A4. The recommendation is a limited first public subset in a later 7.A subtask: caller-owned value snapshot, coarse stable operation category, normalized result, context-local generation and copied backend ID; typed runtime/connection copy functions; and compatibility-preserving extended constructors for failures with no returned object. Native domains/codes, fine internal phases and classification flags remain internal until a cross-backend registry and ABI review are complete.

This design solves live runtime, live connection and failed-constructor cases without `pst_get_last_error`, global or thread-local state. Diagnostic remains a latest active snapshot, not history or logging. Backend implementation version remains separate runtime/provider metadata. Success clears stale detail; clean close, WOULD_BLOCK and timeout are not forced into diagnostic failures. Public API/ABI, SPI, TLS and remote disclosure remain unchanged by 7.A4.

## 7.A5 public diagnostic ABI foundation

The public snapshot contains only size/version, validity, context-local generation, normalized `PST_RESULT`, a coarse public operation and a copied fixed-capacity backend ID. Native domains/codes, secondary codes, flags and fine internal phases remain private. Arbitrary strings, paths, hostname/ALPN, certificates, credentials, trust, keys, tokens, application payload and native pointers are structurally unrepresentable.

`pst_runtime_copy_diagnostic` and `pst_connection_copy_diagnostic` copy live-object state. `pst_runtime_create_ex` and `pst_connection_create_ex` cover failures for which no object survives, with optional caller-owned output. Existing constructors are unchanged wrappers. Output validation occurs before backend side effects; same-major larger records preserve their unknown tail. Success clears active detail, snapshots have independent lifetime, and there is no allocation or global/thread-local last error.

VC6 `/W4` ABI tests freeze the 56-byte layout and offsets, constants, version compatibility, tail preservation, backend-ID truncation/termination, explicit redaction mapping, failure/success constructor behavior, multiple contexts and lifetime after destruction. API 1.1.0 and library 0.2.0 identify the public addition. SPI 2.3 is unchanged. TLS 1.2 and TLS 1.3 mTLS/ALPN secure echo regressions both retained `WRITE=25 READ=25 CONTENT_MATCH=1`.

## 7.A6 abuse resistance

Deterministic VC6 tests now cover malformed sizes/versions, future tails across all four public producers, null semantics, constructor transactional behavior, unknown-selector non-reflection, backend-ID boundaries, generation wrap/locality, copied snapshot immutability, success clearing, runtime/connection isolation, normalized-result association, and redaction with adjacent fake path/token/payload fixtures. A standalone `/W4` C89 consumer includes only `papinho_secure_transport.h`.

The review found no public ABI flaw. One internal association bug was corrected: `pst_connection_create_ex` with a null connection-output pointer now reports coarse operation `CONNECTION`, not `RUNTIME`. API remains 1.1.0, library 0.2.0, and SPI 2.3. No native details, logging surface, backend version, TLS/readiness behavior, or remote disclosure were added.

## 7.A7 logging/event sink design

The future logging boundary is specified in [logging-design.md](logging-design.md). Diagnostics remain independent structured state and logging remains optional presentation. The selected direction is a limited public structured sink with richer internal TRACE detail until its taxonomy is stable. Delivery is synchronous, runtime-scoped, consumer-owned, non-authoritative, allocation-free for basic events, and absolutely redacted. No logger API, event ABI, callback, console/file output, worker, native error exposure, version bump, or functional behavior was added in 7.A7.
