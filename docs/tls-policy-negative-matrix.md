# TLS Policy Negative Matrix

## Status and scope

Phase 7.D is complete. The formal closure audit found every original mandatory TLS-policy goal satisfied by deterministic tests, real RetroZilla NSS fixtures, or preserved Phase 4/5/6/7 evidence. `PROVED` means existing deterministic, implementation-boundary, or recorded functional evidence establishes the contracted result. Existing NT4 evidence is reused because the 7.D production corrections affect only invalid oversized ALPN rejection and diagnostic observability, not valid TLS behavior or policy decisions.

Versions remain API 1.2.0, library 0.3.0, and SPI 2.3.

## Enforcement ownership

| Surface | Enforcement | Fail-closed boundary |
|---|---|---|
| TLS min/max | Core validates supported constants and order; NSS applies the exact `SSLVersionRange` | Bad configuration is rejected; incompatible peers fail handshake |
| ALPN | Core validates/copies the ordered length-delimited list; NSS configures it and checks required negotiation after handshake | Required ALPN without negotiation becomes `POLICY_VIOLATION` before public completion |
| Server identity/trust | Core freezes a coherent hostname/trust policy; NSS verifies chain and hostname | Authentication failure is terminal |
| Client identity | Core requires configured credentials; NSS imports/presents them | Missing/malformed credentials cannot produce operational mTLS |
| Capabilities | Runtime selection checks explicit runtime requirements; connection creation checks frozen-config-derived requirements | Unsupported policy returns `UNSUPPORTED` before operation |
| Mutation | Core freezes `pst_config` | Security setters return `INVALID_STATE` after freeze |

Backend selection is two-stage. Exact, ordered, and automatic selection use `PST_RUNTIME_OPTIONS.required_capabilities`. Once selected, frozen-config requirements are checked against that backend during `pst_connection_create`. There is no reselection after connection creation or handshake/policy failure. Thus candidate A incompatible / candidate B compatible belongs at runtime selection with explicit capabilities, not post-handshake fallback.

## Canonical matrix

A dash means the old evidence did not retain that artifact; it does not assert the artifact is unavailable.

| Policy case | Configuration | Expected/observed result | Stage/final state | Evidence | NT4 | Status |
|---|---|---|---|---|---|---|
| TLS exact 1.2 | Client/server 1.2 | `0x0303`, authenticated 25-byte echo | complete/established | Phase 5, 7.C host; Phase 6 target | PASS | PROVED |
| TLS exact 1.3 | Client/server 1.3 | `0x0304`, authenticated 25-byte echo | complete/established | Phase 5, 7.C host; Phase 6 target | PASS | PROVED |
| Range to 1.2 | Client 1.2-1.3; server 1.2 | TLS `0x0303`, authenticated 25-byte echo | complete/established | 7.D3 real NSS | - | PROVED |
| Range to 1.3 | Client 1.2-1.3; server 1.3 | TLS `0x0304`, authenticated 25-byte echo | complete/established | 7.D3 real NSS | - | PROVED |
| No downgrade | Client 1.3 only; server 1.2 | `PROTOCOL_FAILURE`, no version, no weaker retry | handshake/failed | Phase 5 host, Phase 6 target | PASS | PROVED |
| Inverse mismatch | Client 1.2 only; server 1.3 | `PROTOCOL_FAILURE`, no widening | TLS negotiation/failed | 7.D3: diagnostic HANDSHAKE, one ERROR, no resurrection | - | PROVED |
| Valid min/max | 1.2/1.2, 1.3/1.3, 1.2/1.3 | Accepted | configuration/no connection | `test_tls_policy` | - | PROVED |
| Invalid min/max | min greater than max | `INVALID_ARGUMENT`; no reorder/mutation | configuration/no connection | `test_tls_policy`, including transactional retention | - | PROVED |
| Unknown TLS value | zero/unknown/future value | `INVALID_ARGUMENT`; no clamp | configuration/no connection | `test_tls_policy` | - | PROVED |
| Required ALPN match | `fixture/1`; matching server | PASS, negotiated value copied | post-handshake/established | Phase 5/6 | PASS | PROVED |
| Required ALPN mismatch | `fixture/1`; server `fixture/2` | `POLICY_VIOLATION`; never operational | post-handshake policy/failed | Phase 5 plus 7.D3 diagnostic/logging/no-resurrection | Not separately claimed | PROVED |
| Required ALPN absent | `fixture/1`; no server ALPN | `POLICY_VIOLATION`; never operational | post-handshake policy/failed | 7.D3 real NSS, OFF/ERROR/TRACE and no resurrection | - | PROVED |
| Optional ALPN match | Optional `fixture/1`; matching server | TLS `0x0304`, ALPN `fixture/1`, 25-byte echo | handshake/established | 7.D3 real NSS | - | PROVED |
| Optional ALPN absent | Optional `fixture/1`; no ALPN | TLS `0x0304`, `UNAVAILABLE`, 25-byte echo | handshake/established | 7.D3 real NSS | - | PROVED |
| Multiple/ordered ALPN | Client `fixture/1,fixture/2`; server selects `fixture/2` | Selected offered value, TLS `0x0304`, 25-byte echo; no preference claim | handshake/established | 7.D3 real NSS | - | PROVED |
| Wrong hostname | Peer auth, valid CA, wrong name | `HOSTNAME_MISMATCH` | authentication/failed | Phase 4/5 host, Phase 6 target | PASS | PROVED |
| Required hostname missing | Peer auth, no hostname | `POLICY_VIOLATION` at freeze | configuration/no connection | `test_identity` | Portable PASS | PROVED |
| Empty/embedded-NUL hostname | Empty or malformed input | Empty pointer/size form and embedded NUL rejected; missing fails freeze when required | configuration/no connection | `test_tls_policy` | - | PROVED |
| Hostname lifetime/freeze | Copied name; mutate after freeze | Independent copy; `INVALID_STATE` | configuration/no connection | Core owned-copy implementation; `test_identity`/`test_tls_policy` freeze tests | - | PROVED |
| Valid custom CA | Fixture CA/peer | PASS, authenticated peer | authentication/established | Phase 4/5/6 | PASS | PROVED |
| Untrusted CA | Unrelated CA | `AUTH_FAILURE`; no fallback | authentication/failed | Phase 4/5 host, Phase 6 target | PASS | PROVED |
| Malformed trust | Empty custom data/system with data | `INVALID_ARGUMENT` | constructor/no connection | `test_identity`, `test_tls_policy`, constructor validation | - | PROVED |
| System trust unsupported | System source on RetroZilla NSS | `UNSUPPORTED`; no hidden default | capability/no connection | NSS capability/selection tests | Unit PASS | PROVED |
| Peer-info success | Trusted correct peer | Presence/chain/hostname/authentication true after success | authentication/established | Phase 4/5/6 | PASS | PROVED |
| Peer-info after failure | Bad hostname/trust | Never expose authenticated operational peer | authentication/failed | Getter requires `ESTABLISHED`; Phase 4/5/6 terminal negatives | Failure PASS | PROVED |
| Valid mTLS | Valid credential; required server | PASS, server `AUTH=True` | handshake/established | Phase 4/5/6 | PASS | PROVED |
| Missing client credential | No credential; required server | Handshake rejected, no bypass | handshake/failed | Phase 5/6 | PASS | PROVED |
| Client-auth flag without credential | Flag set, no credential | `POLICY_VIOLATION` at freeze | configuration/no connection | `test_tls_policy` | - | PROVED |
| Malformed credential | Incomplete cert/key pair | `INVALID_ARGUMENT` | constructor/no connection | `test_identity`, `test_tls_policy`, constructor validation | Unit PASS | PROVED |
| Credential, server-auth only | Credential configured; server does not request it | TLS `0x0304`, ALPN and 25-byte echo pass; server `AUTH=False` | handshake/established | 7.D3 real NSS | - | PROVED |
| TLS capability missing | Config needs absent backend version | `UNSUPPORTED` | capability/no connection | TLS 1.2-only and TLS 1.3-only mocks in `test_tls_policy` | - | PROVED |
| ALPN capability missing | Required or optional configured ALPN; backend lacks it | `UNSUPPORTED` | capability/no connection | `test_tls_policy`; current contract requires capability for either mode | - | PROVED |
| Selection A incompatible/B compatible | Explicit runtime requirement | Skip A, select B, and clear rejected-candidate diagnostic | runtime creation | Capability-different ordered mocks in `test_tls_policy` | - | PROVED |
| All candidates incompatible | Capability absent everywhere | `UNSUPPORTED` | runtime creation/no runtime | Ordered mocks, normalized diagnostic and one ERROR in `test_tls_policy` | - | PROVED |
| No post-selection fallback | Selected backend later fails policy | Terminal failure, no reselection | connection/handshake failed | Exact incompatible config and post-handshake policy failure in `test_tls_policy` | - | PROVED |
| Frozen TLS setter | Set TLS or identity policy after freeze | `INVALID_STATE`; unchanged policy | configuration/no connection | `test_tls_policy` | - | PROVED |
| No resurrection | Retry/read/write/wait after failure | No establishment or secure application I/O | terminal failed | Full representative operation matrix in `test_tls_policy` | Existing negatives | PROVED |
| Diagnostic coherence | Version mismatch and required ALPN failures | Result equals normalized diagnostic; HANDSHAKE vs CONFIGURATION | failure-specific/failed | 7.D3 real NSS; backend ID normalized, no native fields | - | PROVED |
| Logging safety | Required ALPN absent at OFF/ERROR/TRACE | OFF zero; ERROR one; TRACE six safe structured events with one ERROR | post-handshake policy/failed | 7.D3 real NSS | - | PROVED |

No row permits downgrade, plaintext fallback, trust-all, hostname/ALPN/client-auth bypass, hidden CA composition, capability approximation, weaker retry, or application I/O before policy satisfaction. Local diagnostic specificity does not authorize more detailed remote TLS alerts.

## Phase 7.D3 functional NSS results

| Case | TLS / ALPN result | PST result and terminal evidence | Logging | Secure I/O | Status |
|---|---|---|---|---|---|
| Client 1.2-1.3 / server 1.2-only | `0x0303`, `fixture/1` | Established | OFF: 0 | `25/25`, match | PASS |
| Client 1.2-1.3 / server 1.3-only | `0x0304`, `fixture/1` | Established | OFF: 0 | `25/25`, match | PASS |
| Client 1.2-only / server 1.3-only | No negotiation | `PROTOCOL_FAILURE`; diagnostic HANDSHAKE; no resurrection | ERROR: 1 | None | PASS |
| Client 1.3-only / server 1.2-only | No downgrade | `PROTOCOL_FAILURE`; diagnostic HANDSHAKE; no resurrection | ERROR: 1 | None | PASS |
| Required `fixture/1` / server no ALPN | No negotiated ALPN | `POLICY_VIOLATION`; diagnostic CONFIGURATION; no resurrection | OFF 0; ERROR 1; TRACE 6 total/1 ERROR | None | PASS |
| Optional `fixture/1` / matching server | `0x0304`, `fixture/1` | Established | OFF: 0 | `25/25`, match | PASS |
| Optional `fixture/1` / no ALPN | `0x0304`, ALPN `UNAVAILABLE` | Established | OFF: 0 | `25/25`, match | PASS |
| Client `fixture/1,fixture/2` / server `fixture/2` | `0x0304`, selected offered `fixture/2` | Established | OFF: 0 | `25/25`, match | PASS |
| Required `fixture/1` / server `fixture/2` | No valid negotiated ALPN | `POLICY_VIOLATION`; diagnostic CONFIGURATION; no resurrection | ERROR: 1 | None | PASS |
| Credential configured / server-auth-only | `0x0304`, `fixture/1`; server `AUTH=False` | Established | OFF: 0 | `25/25`, match | PASS |

Version failures occur during TLS negotiation. Required-ALPN absent and mismatch reach the provider's successful TLS handshake result and then fail the provider-local required-ALPN policy check before PST reports operational completion. Exact backend selection has already completed, and no post-connection fallback occurs.

The first required-ALPN runs exposed a diagnostic-only provider defect: terminal `POLICY_VIOLATION` was returned without capture. The NSS backend now captures that normalized result at phase ALPN. Result, state, logging count, TLS wire behavior, and enforcement did not change. Revalidation produced `DIAG_VALID=1`, `POLICY_VIOLATION`, operation CONFIGURATION, backend `retrozilla-nss`, and no native fields.

After this provider-local correction, the clean build, regular suite, NSS unit, deterministic policy test, and runtime/failure integration builds passed with VC6 `/W4` and zero warnings. `clean_close`, `data_then_close` (29 bytes before clean close), and `abrupt_close` (truncated) passed. The 7.D2 oversized-ALPN rejection remains covered by `test_tls_policy`.
## Closure audit

Phases 4/5 prove host custom trust, mTLS, wrong hostname, untrusted CA, missing client credential, no downgrade, and required ALPN mismatch. Phase 6 proves on real Windows NT 4.0 SP6 TLS 1.2/1.3, mTLS, required ALPN, wrong hostname, untrusted CA, missing client credential, and no downgrade. Phase 7.A proves diagnostic/logging ABI and redaction, 7.B terminal failure/close classification, and 7.C readiness/progress; these are reused rather than duplicated.

`tests/test_tls_policy.c` is part of the regular VC6 `/W4` suite and proves min/max validation and transactional rejection, ALPN validation/copy/order, identity/trust/credential edges, config-derived capability gating, capability-different selection, freeze, multi-config isolation, provider-policy terminal behavior, diagnostics, logging, and no resurrection.

The deterministic and real NSS functional matrices are complete. The formal closure audit identified no untested mandatory 7.D requirement and no remaining blocker. Phase 7.D is complete; Phase 7 remains in progress and 7.E is next but was not started by this audit.

The deterministic boundary test exposed one defensive production defect: the ALPN wire-size accumulator could overflow `pst_size` for an impossible-size public list before allocation. The core now rejects a count whose minimum encoding cannot fit and guards every addition. Rejection is transactional. Valid policy behavior, API/ABI, result model, diagnostic/logging ABI, SPI, and NSS behavior are unchanged. Host TLS 1.2 and TLS 1.3 mTLS/ALPN echoes passed after the correction. Because only invalid oversized input changed and the portable VC6 test exercises that branch, no NT4 rerun or version bump is required.

The separate RetroZilla NSS source/provenance preservation task remains deferred so `C:\PSTW` can eventually be disposable. It is not part of 7.D.
