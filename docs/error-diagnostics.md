# Error and diagnostic model hardening

Status: Phase 7.A audit started. No public API, ABI, SPI, TLS behavior, or remote-visible behavior has changed.

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
