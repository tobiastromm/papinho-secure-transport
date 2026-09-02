# TLS Policy Negative Matrix

## Status and scope

Phase 7.D is in progress. This audit maps the current public policy contract, its enforcing layer, and evidence from Phases 4, 5, 6, 7.A, 7.B, and 7.C. `PROVED` means existing deterministic or recorded functional evidence establishes the result. `PARTIAL` means only part of the requested case is proved. `GAP` requires a focused test. Existing NT4 evidence is reused because this audit changes no production behavior.

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
| Range to 1.2 | Client 1.2-1.3; server 1.2 | Negotiate 1.2 | complete/established | Range application audited; run absent | - | GAP |
| Range to 1.3 | Client 1.2-1.3; server 1.3 | Provider negotiates 1.3 | complete/established | Default range exists; isolated run absent | - | GAP |
| No downgrade | Client 1.3 only; server 1.2 | `PROTOCOL_FAILURE`, no version, no weaker retry | handshake/failed | Phase 5 host, Phase 6 target | PASS | PROVED |
| Inverse mismatch | Client 1.2 only; server 1.3 | Handshake failure | handshake/failed | No focused run | - | GAP |
| Valid min/max | 1.2/1.2, 1.3/1.3, 1.2/1.3 | Accepted | configuration/no connection | `test_tls_policy` | - | PROVED |
| Invalid min/max | min greater than max | `INVALID_ARGUMENT`; no reorder/mutation | configuration/no connection | `test_tls_policy`, including transactional retention | - | PROVED |
| Unknown TLS value | zero/unknown/future value | `INVALID_ARGUMENT`; no clamp | configuration/no connection | `test_tls_policy` | - | PROVED |
| Required ALPN match | `fixture/1`; matching server | PASS, negotiated value copied | post-handshake/established | Phase 5/6 | PASS | PROVED |
| Required ALPN mismatch | `fixture/1`; incompatible server | Protocol failure or PST `POLICY_VIOLATION`; never operational | handshake/failed | Phase 5 | Not separately claimed | PROVED |
| Required ALPN absent | `fixture/1`; no server ALPN | `POLICY_VIOLATION` if TLS otherwise succeeds | post-handshake/failed | Backend check only | - | GAP |
| Optional ALPN match | Optional `fixture/1`; matching server | PASS with negotiated ALPN | handshake/established | Deterministic provider contract proved; real NSS run pending | - | PARTIAL |
| Optional ALPN absent | Optional `fixture/1`; no ALPN | PASS with ALPN unavailable | handshake/established | Deterministic provider contract proved; real NSS run pending | - | PARTIAL |
| Multiple/ordered ALPN | Ordered list; server selects offered value | PASS only for offered value; no false client-order guarantee | handshake/established | Exact copy/order and offered-value check proved; real NSS run pending | - | PARTIAL |
| Wrong hostname | Peer auth, valid CA, wrong name | `HOSTNAME_MISMATCH` | authentication/failed | Phase 4/5 host, Phase 6 target | PASS | PROVED |
| Required hostname missing | Peer auth, no hostname | `POLICY_VIOLATION` at freeze | configuration/no connection | `test_identity` | Portable PASS | PROVED |
| Empty/embedded-NUL hostname | Empty or malformed input | Empty pointer/size form and embedded NUL rejected; missing fails freeze when required | configuration/no connection | `test_tls_policy` | - | PROVED |
| Hostname lifetime/freeze | Copied name; mutate after freeze | Independent copy; `INVALID_STATE` | configuration/no connection | Implementation/partial tests | - | PARTIAL |
| Valid custom CA | Fixture CA/peer | PASS, authenticated peer | authentication/established | Phase 4/5/6 | PASS | PROVED |
| Untrusted CA | Unrelated CA | `AUTH_FAILURE`; no fallback | authentication/failed | Phase 4/5 host, Phase 6 target | PASS | PROVED |
| Malformed trust | Empty custom data/system with data | `INVALID_ARGUMENT` | constructor/no connection | Partial identity coverage | - | PARTIAL |
| System trust unsupported | System source on RetroZilla NSS | `UNSUPPORTED`; no hidden default | capability/no connection | NSS capability/selection tests | Unit PASS | PROVED |
| Peer-info success | Trusted correct peer | Presence/chain/hostname/authentication true after success | authentication/established | Phase 4/5/6 | PASS | PROVED |
| Peer-info after failure | Bad hostname/trust | Never expose authenticated operational peer | authentication/failed | Terminal failure; booleans not fully asserted | Failure PASS | PARTIAL |
| Valid mTLS | Valid credential; required server | PASS, server `AUTH=True` | handshake/established | Phase 4/5/6 | PASS | PROVED |
| Missing client credential | No credential; required server | Handshake rejected, no bypass | handshake/failed | Phase 5/6 | PASS | PROVED |
| Client-auth flag without credential | Flag set, no credential | `POLICY_VIOLATION` at freeze | configuration/no connection | `test_tls_policy` | - | PROVED |
| Malformed credential | Incomplete cert/key pair | `INVALID_ARGUMENT` | constructor/no connection | One missing-key assertion | Unit PASS | PARTIAL |
| Credential, server-auth only | Credential configured; server does not request it | Ordinary server-auth flow passes | handshake/established | Focused run absent | - | GAP |
| TLS capability missing | Config needs absent backend version | `UNSUPPORTED` | capability/no connection | TLS 1.2-only and TLS 1.3-only mocks in `test_tls_policy` | - | PROVED |
| ALPN capability missing | Required or optional configured ALPN; backend lacks it | `UNSUPPORTED` | capability/no connection | `test_tls_policy`; current contract requires capability for either mode | - | PROVED |
| Selection A incompatible/B compatible | Explicit runtime requirement | Skip A, select B, and clear rejected-candidate diagnostic | runtime creation | Capability-different ordered mocks in `test_tls_policy` | - | PROVED |
| All candidates incompatible | Capability absent everywhere | `UNSUPPORTED` | runtime creation/no runtime | Ordered mocks, normalized diagnostic and one ERROR in `test_tls_policy` | - | PROVED |
| No post-selection fallback | Selected backend later fails policy | Terminal failure, no reselection | connection/handshake failed | Exact incompatible config and post-handshake policy failure in `test_tls_policy` | - | PROVED |
| Frozen TLS setter | Set TLS or identity policy after freeze | `INVALID_STATE`; unchanged policy | configuration/no connection | `test_tls_policy` | - | PROVED |
| No resurrection | Retry/read/write/wait after failure | No establishment or secure application I/O | terminal failed | Full representative operation matrix in `test_tls_policy` | Existing negatives | PROVED |
| Diagnostic coherence | Representative policy failures | Result equals normalized diagnostic; sensible operation | failure-specific/failed | Deterministic capability/ALPN cases proved; real negative snapshots pending | - | PARTIAL |
| Logging safety | OFF/ERROR/TRACE on failures | OFF none; one logical ERROR; safe structured progression | failure-specific/failed | Deterministic policy scenarios proved; real negative integration pending | - | PARTIAL |

No row permits downgrade, plaintext fallback, trust-all, hostname/ALPN/client-auth bypass, hidden CA composition, capability approximation, weaker retry, or application I/O before policy satisfaction. Local diagnostic specificity does not authorize more detailed remote TLS alerts.

## Evidence reused and real gaps

Phases 4/5 prove host custom trust, mTLS, wrong hostname, untrusted CA, missing client credential, no downgrade, and required ALPN mismatch. Phase 6 proves on real Windows NT 4.0 SP6 TLS 1.2/1.3, mTLS, required ALPN, wrong hostname, untrusted CA, missing client credential, and no downgrade. Phase 7.A proves diagnostic/logging ABI and redaction, 7.B terminal failure/close classification, and 7.C readiness/progress; these are reused rather than duplicated.

The audit and deterministic matrix are complete, but Phase 7.D is not. `tests/test_tls_policy.c` is part of the regular VC6 `/W4` suite and proves min/max validation and transactional rejection, ALPN validation/copy/order, identity/trust/credential edges, config-derived capability gating, capability-different selection, freeze, multi-config isolation, provider-policy terminal behavior, diagnostics, logging, and no resurrection.

Next minimally extend the existing local integration runner/fixture for both TLS range outcomes, inverse mismatch, required ALPN absent, optional ALPN with/without negotiation, multiple offered protocols, credentials presented when the server does not request them, and representative diagnostic/logging assertions. The post-correction exact TLS 1.2 and TLS 1.3 success baselines already pass; preserve them plus 7.B and 7.C regressions.

The deterministic boundary test exposed one defensive production defect: the ALPN wire-size accumulator could overflow `pst_size` for an impossible-size public list before allocation. The core now rejects a count whose minimum encoding cannot fit and guards every addition. Rejection is transactional. Valid policy behavior, API/ABI, result model, diagnostic/logging ABI, SPI, and NSS behavior are unchanged. Host TLS 1.2 and TLS 1.3 mTLS/ALPN echoes passed after the correction. Because only invalid oversized input changed and the portable VC6 test exercises that branch, no NT4 rerun or version bump is required.

The separate RetroZilla NSS source/provenance preservation task remains deferred so `C:\PSTW` can eventually be disposable. It is not part of 7.D.
