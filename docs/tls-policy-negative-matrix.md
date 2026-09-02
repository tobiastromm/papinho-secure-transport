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
| Valid min/max | 1.2/1.2, 1.3/1.3, 1.2/1.3 | Accepted | configuration/no connection | Implementation only | - | GAP |
| Invalid min/max | min greater than max | `INVALID_ARGUMENT`; no reorder/mutation | configuration/no connection | Implementation only | - | GAP |
| Unknown TLS value | zero/unknown/future value | `INVALID_ARGUMENT`; no clamp | configuration/no connection | Implementation only | - | GAP |
| Required ALPN match | `fixture/1`; matching server | PASS, negotiated value copied | post-handshake/established | Phase 5/6 | PASS | PROVED |
| Required ALPN mismatch | `fixture/1`; incompatible server | Protocol failure or PST `POLICY_VIOLATION`; never operational | handshake/failed | Phase 5 | Not separately claimed | PROVED |
| Required ALPN absent | `fixture/1`; no server ALPN | `POLICY_VIOLATION` if TLS otherwise succeeds | post-handshake/failed | Backend check only | - | GAP |
| Optional ALPN match | Optional `fixture/1`; matching server | PASS with negotiated ALPN | handshake/established | Current runner cannot express optional | - | GAP |
| Optional ALPN absent | Optional `fixture/1`; no ALPN | PASS with ALPN unavailable | handshake/established | Current runner cannot express optional | - | GAP |
| Multiple/ordered ALPN | Ordered list; server selects offered value | PASS only for offered value; no false client-order guarantee | handshake/established | API/provider support only | - | GAP |
| Wrong hostname | Peer auth, valid CA, wrong name | `HOSTNAME_MISMATCH` | authentication/failed | Phase 4/5 host, Phase 6 target | PASS | PROVED |
| Required hostname missing | Peer auth, no hostname | `POLICY_VIOLATION` at freeze | configuration/no connection | `test_identity` | Portable PASS | PROVED |
| Empty/embedded-NUL hostname | Empty or malformed input | Missing fails freeze when required; malformed rejected | configuration/no connection | Implementation; focused assertions absent | - | PARTIAL |
| Hostname lifetime/freeze | Copied name; mutate after freeze | Independent copy; `INVALID_STATE` | configuration/no connection | Implementation/partial tests | - | PARTIAL |
| Valid custom CA | Fixture CA/peer | PASS, authenticated peer | authentication/established | Phase 4/5/6 | PASS | PROVED |
| Untrusted CA | Unrelated CA | `AUTH_FAILURE`; no fallback | authentication/failed | Phase 4/5 host, Phase 6 target | PASS | PROVED |
| Malformed trust | Empty custom data/system with data | `INVALID_ARGUMENT` | constructor/no connection | Partial identity coverage | - | PARTIAL |
| System trust unsupported | System source on RetroZilla NSS | `UNSUPPORTED`; no hidden default | capability/no connection | NSS capability/selection tests | Unit PASS | PROVED |
| Peer-info success | Trusted correct peer | Presence/chain/hostname/authentication true after success | authentication/established | Phase 4/5/6 | PASS | PROVED |
| Peer-info after failure | Bad hostname/trust | Never expose authenticated operational peer | authentication/failed | Terminal failure; booleans not fully asserted | Failure PASS | PARTIAL |
| Valid mTLS | Valid credential; required server | PASS, server `AUTH=True` | handshake/established | Phase 4/5/6 | PASS | PROVED |
| Missing client credential | No credential; required server | Handshake rejected, no bypass | handshake/failed | Phase 5/6 | PASS | PROVED |
| Client-auth flag without credential | Flag set, no credential | `POLICY_VIOLATION` at freeze | configuration/no connection | Core; focused assertion absent | - | PARTIAL |
| Malformed credential | Incomplete cert/key pair | `INVALID_ARGUMENT` | constructor/no connection | One missing-key assertion | Unit PASS | PARTIAL |
| Credential, server-auth only | Credential configured; server does not request it | Ordinary server-auth flow passes | handshake/established | Focused run absent | - | GAP |
| TLS capability missing | Config needs absent backend version | `UNSUPPORTED` | capability/no connection | Core path; focused assertion absent | - | GAP |
| ALPN capability missing | Config has ALPN; backend lacks it | `UNSUPPORTED` | capability/no connection | Core path; focused assertion absent | - | GAP |
| Selection A incompatible/B compatible | Explicit runtime requirement | Skip A and select B under existing contract | runtime creation | Current ordered test skips missing ID only | - | GAP |
| All candidates incompatible | Capability absent everywhere | `UNSUPPORTED` | runtime creation/no runtime | Exact NSS system-trust case only | - | PARTIAL |
| No post-selection fallback | Selected backend later fails policy | Terminal failure, no reselection | connection/handshake failed | Architecture; focused two-backend test absent | - | PARTIAL |
| Frozen TLS setter | Set TLS policy after freeze | `INVALID_STATE`; unchanged policy | configuration/no connection | Identity setter covered, TLS setter absent | - | PARTIAL |
| No resurrection | Retry/read/write/wait after failure | No establishment or secure application I/O | terminal failed | Handshake retry plus 7.B rules; full matrix absent | Existing negatives | PARTIAL |
| Diagnostic coherence | Representative policy failures | Result equals normalized diagnostic; sensible operation | failure-specific/failed | 7.A ABI; old fixtures lack snapshots per row | - | GAP |
| Logging safety | OFF/ERROR/TRACE on failures | OFF none; bounded safe events; no sensitive/native data | failure-specific/failed | 7.A tests, no negative-policy integration | - | GAP |

No row permits downgrade, plaintext fallback, trust-all, hostname/ALPN/client-auth bypass, hidden CA composition, capability approximation, weaker retry, or application I/O before policy satisfaction. Local diagnostic specificity does not authorize more detailed remote TLS alerts.

## Evidence reused and real gaps

Phases 4/5 prove host custom trust, mTLS, wrong hostname, untrusted CA, missing client credential, no downgrade, and required ALPN mismatch. Phase 6 proves on real Windows NT 4.0 SP6 TLS 1.2/1.3, mTLS, required ALPN, wrong hostname, untrusted CA, missing client credential, and no downgrade. Phase 7.A proves diagnostic/logging ABI and redaction, 7.B terminal failure/close classification, and 7.C readiness/progress; these are reused rather than duplicated.

The audit is complete, but Phase 7.D is not. Next add a focused deterministic public-policy test, preferably `tests/test_tls_policy.c`, for min/max and transactional rejection, ALPN validation/copy, identity/trust/credential edges, config-derived capability gating, capability-different mock selection, freeze, and no resurrection.

Then minimally extend the existing local integration runner/fixture for both TLS range outcomes, inverse mismatch, required ALPN absent, optional ALPN with/without negotiation, multiple offered protocols, credentials presented when the server does not request them, and representative diagnostic/logging assertions. Preserve TLS 1.2/1.3 success plus 7.B and 7.C regressions.

No production defect is established. No API/ABI, result model, diagnostic/logging ABI, SPI, core, or NSS behavior changed, so no NT4 rerun or version bump is required.

The separate RetroZilla NSS source/provenance preservation task remains deferred so `C:\PSTW` can eventually be disposable. It is not part of 7.D.
