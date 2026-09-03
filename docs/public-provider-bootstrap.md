# Public built-in provider bootstrap architecture

Status: Phase 9.D0-A complete. Phase 9.D remains blocked pending 9.D0-B implementation/validation and the limited 9.B addendum. No public header or production source changed in this architecture round.

## Problem and boundary

The release SDK currently exposes only `pst_win32_register_retrozilla_nss()`. Schannel and OpenSSL registration is available only through private SPI/provider headers, so an external application cannot activate those providers or reproduce the tested combined selection using only `include/` and public libraries.

The bootstrap defined here registers providers already compiled into one PST target. It is not provider discovery, DLL probing, a dynamic-plugin loader, provider selection, or provider initialization. Selection remains a separate runtime operation.

## Decision

Add this declaration to `papinho_secure_transport_win32.h` in 9.D0-B:

```c
PST_API PST_RESULT PST_CALL
pst_win32_register_builtin_providers(void);
```

The Win32 header is correct because every current target manifest and native transport adapter is Win32-specific, and the existing public registration helper already lives there. The generic header must not acquire a platform bootstrap before a portable target model exists. The declaration remains C89, VC6 and C++ `extern "C"` compatible and exposes no NSS, NSPR, SSPI, OpenSSL, descriptor, socket, or SPI type.

The existing `pst_win32_register_retrozilla_nss()` remains supported as a compatibility API and is not deprecated. New applications should use the generic built-in bootstrap.

Per-provider public registration functions were rejected. They would expand the public API for every provider, force beginners to know package internals, duplicate target ordering in applications, and make future provider additions source changes for consumers. Stable IDs remain available after bootstrap for applications that deliberately choose EXACT or ORDERED.

Implicit registration during library loading, DllMain, runtime creation, or global initialization was rejected. It would create invisible global mutation, obscure failure handling and ordering, complicate static linking, and conflict with setup-time SPI semantics. Bootstrap is explicit, silent, deterministic, and returns a normalized `PST_RESULT`.

## Target manifests and source sets

9.D0-B should add one target-manifest implementation per deliberately shipped provider set. The selected manifest is part of the target's public library source set and is the single ordered list used by bootstrap. Provider objects referenced by that manifest must also be members of the public target library so a public-only static consumer resolves them.

Current build reality is weaker: `Makefile.vc6`, `Makefile.msvc`, and `Makefile.openssl.msvc` put only the common core into their `papinho_secure_transport.lib`; provider objects are linked separately into tests. Therefore no modern provider is currently a true library built-in. 9.D0-B must correct the target libraries and avoid maintaining a conflicting second provider list in tests or documentation.

Normative manifests are:

| Target | Ordered built-ins |
| --- | --- |
| Legacy VC6 / NT4 | `[retrozilla-nss]` |
| Modern Schannel | `[schannel]` |
| Modern OpenSSL | `[openssl]` |
| Deliberate future/combined modern target | `[schannel, openssl]` |
| Future providerless target | `[]` |

The combined order is policy, not a heuristic. Bootstrap never reorders for TLS version, provider version, trust model, benchmark, availability on PATH, or perceived modernity. AUTOMATIC later walks registered providers in this order and skips capability-incompatible candidates. EXACT uses the named provider only. ORDERED uses the caller's list. No failure after runtime selection triggers another provider.

Schannel bootstrap only registers the compiled adapter; capability probing remains in existing provider initialization. OpenSSL bootstrap does not locate or load arbitrary DLLs; the target's documented link/runtime dependency model remains authoritative. RetroZilla NSS uses the existing internal descriptor/registration path, with no modern provider reference entering the VC6 target.

## State, idempotency, and failures

Registration remains setup-time. The registry should become sealed when the first runtime creation attempt begins. Bootstrap after sealing returns `PST_RESULT_INVALID_STATE`, even if the manifest appears already registered; callers must complete setup before concurrent/runtime use. Direct SPI registration after sealing should be rejected consistently in 9.D0-B if the existing frozen setup-time rule is not currently enforced. This is a semantic enforcement of SPI 2.4, not a new SPI shape; if implementation reveals incompatibility, stop and reopen the impact audit.

Before sealing, bootstrap is idempotent for its exact canonical manifest:

- an unregistered manifest entry is scheduled for registration;
- an entry already registered with the identical canonical descriptor pointer is accepted;
- the same stable ID bound to a different descriptor is a conflict and returns `PST_RESULT_INVALID_STATE`;
- explicit `pst_win32_register_retrozilla_nss()` followed by legacy bootstrap succeeds because both identify the same descriptor;
- direct SPI duplicate registration continues returning its frozen duplicate error.

A batch is transactional. An internal batch helper first validates every manifest descriptor, detects ID conflicts, counts missing entries, and verifies capacity before mutation. It then registers missing entries in manifest order. Any unexpected mid-batch failure rolls back only entries added by that call, in reverse order, using the existing internal unregister machinery; pre-existing entries remain. The externally visible order after success equals manifest order relative to entries added by the batch. Mixing arbitrary pre-registered providers with a manifest is supported only when canonical manifest entries match; their existing earlier registry position is not silently reordered. Applications requiring canonical AUTOMATIC priority must call bootstrap before other direct registration.

A zero-built-in target returns `PST_RESULT_UNAVAILABLE`: the operation is valid but the target contains nothing to activate. Capacity shortage returns `PST_RESULT_RESOURCE_FAILURE`. A sealed registry or conflicting canonical ID returns `PST_RESULT_INVALID_STATE`. A target/build defect or unexpected normalized provider-registration failure returns that safe existing normalized result, with `PST_RESULT_BACKEND_FAILURE` as the fallback. `INVALID_ARGUMENT` should not be produced by the zero-argument public call, and `UNSUPPORTED` is reserved for an operation the target/API cannot implement rather than an empty manifest.

Bootstrap has no diagnostic object because it runs before a runtime exists; the returned normalized result is the complete public report. It emits no stdout/stderr, creates no global logger, and exposes no provider-native error. It is not thread-safe initialization: callers serialize it and finish before runtime creation or concurrent PST use.

Registry capacity remains eight. Bootstrap registers only compiled, deliberately linked descriptors; it creates no placeholders and inspects neither PATH nor ambient TLS installations.

## Version and ABI impact

The implementation adds exactly one public symbol and no structure, enum, constant, layout, calling-convention, or existing-symbol change. Recommendation:

- API: `1.2.0 -> 1.3.0` for compatible additive functionality;
- Library: remain `0.3.0` during implementation, then include the approved API in the existing Phase-9 `0.4.0` release candidate;
- SPI: remain `2.4`.

A limited 9.B addendum must inventory the new symbol, update the expected public function count from 40 to 41 if the audit confirms no other symbols, update ABI documentation/baseline, and re-run x86 VC6 and x64 MSVC public-header consumers. Existing ABI offsets and values must remain byte-for-byte unchanged.

## 9.D0-B validation plan

Legacy target:

- public bootstrap registers only NSS;
- repeated pre-runtime bootstrap succeeds;
- AUTOMATIC and EXACT `retrozilla-nss` create runtimes;
- explicit NSS compatibility helper before bootstrap succeeds;
- no modern provider symbol is referenced;
- VC6/C89 public-header-only consumer passes.

Modern Schannel target:

- public bootstrap registers only Schannel;
- repeated bootstrap succeeds;
- AUTOMATIC and EXACT `schannel` pass.

Modern OpenSSL target:

- public bootstrap registers only OpenSSL;
- repeated bootstrap succeeds;
- TLS 1.3 plus SYSTEM_TRUST can select OpenSSL;
- missing staged runtime dependencies retain existing loader/runtime failure behavior.

Combined target:

- bootstrap order is Schannel then OpenSSL;
- TLS 1.2 plus SYSTEM_TRUST AUTOMATIC selects Schannel;
- TLS 1.3 plus SYSTEM_TRUST AUTOMATIC selects OpenSSL;
- EXACT and ORDERED remain unchanged.

Generic gates:

- canonical pre-registration accepted; conflicting same-ID descriptor rejected;
- sealed/too-late bootstrap rejected;
- zero manifest returns UNAVAILABLE;
- capacity and deterministic transactional rollback covered;
- direct duplicate SPI behavior unchanged;
- all existing SPI, selection, lifecycle and provider tests pass.

The closure gate is a real executable compiled with only `include/`, the target public library and documented system/runtime libraries. It must call the public bootstrap, create a runtime and complete at least one real TLS connection without any include or symbol under `src/`.

Only after 9.D0-B passes and the limited 9.B addendum freezes API 1.3.0 may Phase 9.D documentation/examples resume.
