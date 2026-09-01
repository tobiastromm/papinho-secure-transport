# Error and diagnostic model hardening

Status: Phase 7.A in progress. Public API/ABI, TLS behavior, and remote-visible behavior are unchanged; Phase 7.A2 adds one optional append-only hook to the internal SPI.

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

Not currently available publicly: backend implementation version and a structured local diagnostic snapshot. These must remain distinct concepts if added. No ABI addition is approved by this audit.

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
