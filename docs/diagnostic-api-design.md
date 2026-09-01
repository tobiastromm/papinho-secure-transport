# Controlled diagnostic exposure design

Status: Phase 7.A complete. The public diagnostic ABI foundation, abuse-resistance rules, and optional consumer-controlled logging integration passed closure audit; SPI, TLS, readiness, diagnostic ABI, and wire behavior are unchanged.

## Decision

The first public subset is staged, explicit and typed: caller-owned `PST_DIAGNOSTIC_INFO` value snapshots, typed copy functions for live runtime/connection objects, and compatibility-preserving extended constructor variants accepting an optional diagnostic destination. Existing constructor signatures remain supported wrappers. Native detail remains private.

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

## 7.A4 gate disposition

The focused ABI review requested by 7.A4 was completed in 7.A5. The limited snapshot, explicit conversion, typed copies and extended constructors were accepted with the constraints below.

## 7.A5 implemented ABI

`PST_DIAGNOSTIC_INFO` is a 56-byte C89 value on VC6: `struct_size` at offset 0, `api_version` at 4, `valid` at 8, `generation` at 12, normalized result at 16, coarse operation at 20, and a copied 32-byte backend ID at 24. `pst_diagnostic_info_init` initializes current size/version in caller storage, consistently with the Phase 1 initializer. Queries and extended constructors validate those fields. The implementation rejects a too-small record or incompatible API major before constructor side effects, accepts a larger same-major record, writes only the known 56-byte prefix, preserves the unknown tail, and guarantees backend-ID termination. The inline textual backend ID capacity is 32 bytes: at most 31 ID bytes plus the terminating NUL; longer validated internal IDs are deterministically truncated.

The public operations are `NONE`, `RUNTIME`, `CONFIGURATION`, `TRANSPORT`, `CONNECTION`, `HANDSHAKE`, `AUTHENTICATION`, `READ`, `WRITE`, `WAIT`, `SHUTDOWN`, and `PEER_INFO`. Their numeric values are explicitly frozen from 0 through 11.

| Internal phase | Public operation |
|---|---|
| `NONE` or unknown | `NONE` |
| backend initialize, runtime create, capability validate, backend select | `RUNTIME` |
| TLS configure, ALPN, identity setup | `CONFIGURATION` |
| transport attach | `TRANSPORT` |
| connection create | `CONNECTION` |
| handshake | `HANDSHAKE` |
| peer authenticate, hostname verify | `AUTHENTICATION` |
| read | `READ` |
| write | `WRITE` |
| wait | `WAIT` |
| shutdown | `SHUTDOWN` |
| peer info | `PEER_INFO` |

No internal phase number crosses the ABI.

Live snapshots use `pst_runtime_copy_diagnostic` and `pst_connection_copy_diagnostic`. Failed-constructor snapshots use `pst_runtime_create_ex` and `pst_connection_create_ex`; the diagnostic pointer is optional. The original constructors call the extended forms with no diagnostic, preserving source and binary behavior. No global/thread-local last-error state exists. A successful operation exports `valid=0`, while a relevant failure exports a normalized result and copied backend ID. A copied value survives source mutation and object destruction without allocation.

Redaction is structural. The public record has no fields for native domain/code, flags, internal phase, arbitrary/native text, pointers, paths, environment data, hostname, ALPN, certificate/DER, trust, credentials, keys, tokens, payload, timestamps, threads, history, or severity. Portable control flow continues to use the returned `PST_RESULT` and progress records.

The API version is now 1.1.0 and the library version is 0.2.0. The internal backend SPI remains 2.3 because public snapshot export and constructor wrappers require no backend-vtable change.

## 7.A6 hardening contract

Phase 7.A6 freezes the 7.A5 public layout and values without an API, ABI, library-version, or SPI-version change. `PST_DIAGNOSTIC_INFO` remains 56 bytes on VC6 with minimum prefix 56, backend ID capacity 32, API 1.1.0, library 0.2.0, and SPI 2.3.

Canonical C89 initialization is:

```c
PST_DIAGNOSTIC_INFO diagnostic;
pst_runtime *runtime;
PST_RESULT result;

runtime = NULL;
if (pst_diagnostic_info_init(&diagnostic) != PST_RESULT_OK) {
    /* invalid local pointer */
}
result = pst_runtime_create_ex(&options, &runtime, &diagnostic);
if (result != PST_RESULT_OK && diagnostic.valid) {
    /* Branch portably on result/diagnostic.normalized_result. */
    /* operation and backend_id are supplemental local troubleshooting context. */
    /* pst_result_string(diagnostic.normalized_result) is stable generic text. */
}
```

A caller may instead zero storage and assign `struct_size` and `api_version`. No other bytes need initialization. Consumers must not overlap diagnostic output storage with another argument or output object; overlapping argument buffers are outside the contract. `diagnostic == NULL` remains allowed only for the extended constructors. Typed copy functions require both a live typed object and valid output storage.

Every diagnostic-consuming API rejects sizes 0, 1, field-boundary representatives, and 55 before backend/object side effects. Size 56 is accepted. Larger same-major records are accepted, only the known 56-byte prefix is written, and the unknown tail is preserved by runtime query, connection query, runtime creation, and connection creation. Major 1 versions, including later minor values, are compatible; zero and other majors are incompatible.

The backend ID is a copied stable provider ID, never display name or requested selector reflection. Empty through 31-byte values are terminated directly; 32-byte and longer internal values truncate deterministically to 31 bytes plus NUL. A shorter later value clears stale bytes. An unknown caller-supplied exact selector exports an empty backend ID.

Generation wraps modulo 2^32, is local to one source/context, and cannot order different objects. Capture/reset increment it; copy preserves it; older caller snapshots remain unchanged by later capture/reset or destruction. Runtime, connection, and constructor-attempt contexts remain independent.

WOULD_BLOCK/NEED states and normal wait timeout remain progress, not failure diagnostics. Clean CLOSED remains distinct from TRUNCATED; a retained truncation diagnostic, when present, carries normalized `TRUNCATED` without native detail. Native domains/codes, flags, detailed phases, paths, hostnames, peer text, credentials, trust, keys, tokens, payload and arbitrary messages remain structurally absent. Diagnostics do not affect TLS alerts or wire bytes.

Application policy remains: normalized result for portable decisions; operation/backend ID for local support context; `pst_result_string` for generic stable English text; localization, UI wording and logging policy belong to the application. No logger, callback, sink, severity, automatic file, or backend-version API is introduced.

## Relationship to the 7.A7 event design

The future event sink is documented in [logging-design.md](logging-design.md). It consumes an inline redacted diagnostic value or equivalent projection captured at emission time; it never becomes the source of diagnostic truth and never requires callback reentry to query the emitting object. Logging OFF and a null sink leave all diagnostic query and constructor-output behavior intact. `PST_DIAGNOSTIC_INFO` remains 56 bytes and its fields, operation numbers, generation, version compatibility, and redaction contract are unchanged.
## 7.A8 diagnostic/event integration

PST_LOG_EVENT carries selected redacted value fields at emission time: normalized result, public operation and copied backend ID. It does not embed or alter PST_DIAGNOSTIC_INFO, and contains no native domain/code, diagnostic flags, generation, hostname, peer text or payload. This avoids stale callback queries while keeping diagnostics authoritative and independently queryable with logging OFF. The logging addition moves the current API/library versions to 1.2.0/0.3.0; the diagnostic layout and SPI 2.3 remain unchanged.

## 7.A closure

The final audit confirms the 56-byte diagnostic ABI, append-only validation, tail preservation, context-local generation, copy lifetime, constructor coverage, multi-object isolation, progress semantics, redaction, and local-versus-remote disclosure policy. No mandatory diagnostic gap remains. API is 1.2.0, library is 0.3.0, and SPI remains 2.3.
