# Controlled diagnostic exposure design

Status: Phase 7.A4 design decision. No public API, ABI, implementation, SPI, TLS, readiness, logging, or wire behavior is changed by this document.

## Decision

Use a staged, explicit and typed public model in a later 7.A subtask. The recommended first public subset consists of a caller-owned value snapshot, typed copy functions for live runtime/connection objects, and compatibility-preserving extended constructor variants that accept an optional caller-owned diagnostic destination. Existing constructor signatures remain indefinitely supported. No public diagnostic API is implemented in 7.A4.

This is Option 3: expose a limited subset first. The internal taxonomy and native detail remain private until their stability and cross-backend policy are proven. This avoids freezing Phase 9 ABI prematurely while solving live-object and failed-constructor cases without global last-error state.

## Public failure-operation matrix

Legend: A = object survives; B = constructor can fail without object; C = stateless; D = existing result structure; E = potentially useful backend/native detail; F = normalized `PST_RESULT` is sufficient.

| Public operation | Classes | Recommended diagnostic boundary |
|---|---|---|
| `pst_api_version`, `pst_library_version` | C/F | No diagnostic; return values are complete. |
| `pst_version_info_init`, `pst_get_version` | C/F | Normalized validation result only. |
| `pst_result_string` | C/F | Presentation of normalized result, not a diagnostic producer. |
| `pst_credentials_create` | B/F | Future extended constructor may return coarse validation/OOM snapshot; never credential bytes. |
| `pst_trust_create` | B/F | Future extended constructor may return coarse validation/OOM snapshot; never trust contents/path. |
| credentials/trust release | C/F | No diagnostic; null-safe destruction. |
| `pst_config_create` | B/F | Future extended constructor only if consistency warrants it; current failures are argument/OOM. |
| config identity/TLS setters and freeze | A/F | Config survives. A future config diagnostic is optional, not required for portable behavior. |
| config release | C/F | No diagnostic. |
| `pst_runtime_create` | B/E | Primary failed-constructor case. Future compatible extended variant copies caller-owned snapshot. |
| `pst_runtime_get_info` | A/F | Runtime metadata, not diagnostic detail. |
| runtime release | C/F | No diagnostic; destruction must not be used as an error reporting channel. |
| `pst_connection_create` | B/E | Future extended variant uses the same snapshot destination pattern as runtime creation. |
| transport attach | A/E | Connection survives accepted/nonaccepted outcomes and can be queried through typed copy. Ownership output remains authoritative. |
| handshake | A/D/E | Incremental operation/result remains control flow; typed connection snapshot supplies optional troubleshooting detail. |
| readiness interest | A/F | State query only; invalid-state failures are normalized. |
| wait | A/D/E | Timeout is a normal `PST_WAIT_RESULT`; only actual backend failure may update diagnostic. |
| read/write | A/D/E | `PST_IO_RESULT` remains authoritative for progress/close; connection snapshot is supplemental. |
| peer-info creation | A/B/E | Connection survives even if peer-info allocation/extraction fails; query connection diagnostic rather than add peer-info last-error state. |
| peer-info summary/DER copy | A/D/F | Existing output sizing/result contract is sufficient; DER is identity data, never diagnostic data. |
| ALPN copy | A/D/F | Existing availability/truncation result is sufficient; peer-controlled ALPN bytes are not diagnostic payload. |
| shutdown | A/D/E | Incremental result remains control flow; typed connection snapshot may hold actual failure. |
| connection/peer-info/transport release | C/F | No diagnostic from destruction. |
| Win32 backend registration | C/F | Normalized registration result; no implicit diagnostic state. |
| Win32 socket transport wrapper creation | B/F | Validation/OOM only; it performs no native import. `SOCKET` remains outside generic diagnostics/API. |

Not every non-OK `PST_RESULT` warrants a valid diagnostic. Argument validation, unsupported optional behavior, clean close, sizing/truncation queries and ordinary timeout commonly need no native detail.

## Consumers and layering

Portable application control flow branches exclusively on `PST_RESULT`, operation state, `PST_IO_RESULT`, and `PST_WAIT_RESULT`. A diagnostic snapshot is for local troubleshooting, support bundles, advanced UI and a future consumer-owned logger. User-facing localized text remains application policy. A future logger consumes a copy; it is not the producer or owner of diagnostic state.

The layering remains:

```text
operation -> PST_RESULT/progress -> structured snapshot copy
                                    -> optional future application logging
```

No diagnostic field is sent to the TLS peer.

## Recommended future public snapshot

A future `PST_DIAGNOSTIC_INFO`-like value should follow existing size/version conventions, but the exact name is deliberately not frozen here. The first subset should contain only:

- `struct_size` and `api_version`;
- `valid`;
- context-local `generation`;
- normalized `PST_RESULT`;
- one coarse, stable public operation category;
- copied backend ID with a documented fixed capacity and guaranteed termination.

The first subset should not contain:

- internal fine-grained phase IDs;
- native domain or primary/secondary native codes;
- internal classification flags;
- backend/native pointers or objects;
- arbitrary strings or native error text;
- hostname, ALPN bytes, peer text, paths or environment contents;
- certificate/DER, trust data, credentials, keys, passwords, tokens or payload;
- timestamps, thread IDs, history/event arrays or logger severity.

Native domain and numeric code are useful for advanced local troubleshooting, but should remain internal in the first public subset. They can be considered as append-only fields only after a cross-backend domain registry is specified. If exposed later, unknown domains must remain representable and documentation must state that native codes are never portable control flow. `BACKEND` remains too underspecified to freeze today.

## Operation taxonomy

The current internal phases are backend-neutral enough for capture but too detailed and still evolving for public numeric ABI. A future public category should be coarser: selection/initialization, configuration, connection creation/attach, handshake/authentication, wait, read, write, peer information and shutdown. Internal phases such as capability validation, TLS configuration and hostname verification may map to those stable groups without exposing their exact numbers.

## Generation

Expose generation in the limited snapshot because it lets an application detect replacement/reset without comparing native detail. Its contract must be narrow: it is meaningful only for successive snapshots copied from the same runtime, connection or explicit constructor attempt; it may wrap; it is not globally unique, chronological across objects, a timestamp or an event ID. Copy preserves it and reset/success increments it while setting `valid=0`.

## Backend identity and version

Backend ID belongs in the snapshot because a failed constructor may leave no runtime and because the snapshot must remain self-contained after object destruction. The ID is copied, never borrowed, and is not renamed by this design.

Backend implementation version does not belong in each error snapshot. It is stable runtime/provider metadata and should eventually use a separate typed runtime/backend metadata query alongside PST library/API versions and capabilities. A diagnostic/support page can combine metadata with a snapshot without duplicating version fields in every error.

## Public access patterns for a later subtask

For live objects, prefer separate typed functions conceptually equivalent to runtime-copy and connection-copy. Avoid generic `void *`, object tags and introspection. The functions copy into caller storage and never return pointers to mutable internal state.

For failed constructors, preserve existing source/binary compatibility with new extended variants or distinctly named transactional functions accepting an optional diagnostic output. Existing constructors remain wrappers. Passing no diagnostic destination must not change behavior. Do not append parameters to existing exported functions and do not use global/thread-local fallback.

A separate allocated creation-result object is unnecessary for the basic snapshot and complicates OOM reporting. A public opaque operation context is viable but less ergonomic than a caller-owned output snapshot when only the final active diagnostic is required. Diagnostic history is explicitly out of scope and belongs to future events/logging.

## Lifetime, success and close semantics

Every public diagnostic is an independent snapshot. It remains valid after the source object changes or is destroyed. It embeds no PST object pointer and basic copy requires no allocation.

Success/reset exposes no active diagnostic (`valid=0`) and increments the source generation, preventing stale failures. A successful later backend candidate clears failed-candidate detail. A clean `CLOSED` outcome generally needs no diagnostic. `TRUNCATED` remains a distinct portable result/close classification and may carry a valid diagnostic when a concrete transport/protocol cause exists, but native detail is not mandatory.

WOULD_BLOCK/NEED states are progress, not errors. Wait timeout is a bounded normal outcome, not automatically a diagnostic failure.

## Threading and ABI evolution

Typed copy functions inherit the owning object's existing threading contract; they add no global lock and no broader cross-thread safety promise. Caller output storage must not be concurrently mutated.

The eventual structure must use explicit PST integer types, `struct_size`, `api_version`, stable numeric constants, append-only growth and no compiler-dependent pointers. Callers initialize size/version; implementations write only the supported prefix. Public constants should be frozen only in the implementation subtask after VC6 layout tests and cross-backend review.

## Security boundary

The proposed public subset is local structured data, not remote protocol data. It never changes TLS alerts, certificate validation, trust, credentials, hostname verification, ALPN, readiness, secure I/O or shutdown. No localization, logging callback, level, console output or automatic file is introduced.

## Next gate inside 7.A

The next subtask should specify and test the limited public snapshot ABI and conversion from internal snapshots, including VC6 layout/size-version behavior, without yet exposing native domains/codes. It should also prototype typed runtime/connection copy and compatible extended runtime/connection constructors. Public constants should be accepted only after that focused ABI review.
