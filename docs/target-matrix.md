# Target Matrix

This is the canonical PapinhoSecureTransport target matrix. The ecosystem-wide naming authority is PapinhoEngineering ADR-0002 revision 3; this document applies that decision to PST.

Target identity, build inputs, operating-system support policy, and observed validation are separate facts. A toolchain name is not an operating-system support claim.

## 0.6.3 VC6 CRT-qualified targets

From one source commit, the VC6/NSS `-ml` and `-md` targets use explicit
`/ML` and `/MD` builds and isolated SDK packages. Package metadata identifies
the CRT and is checked against library COFF directives. Both variants passed
package-only consumers, real Windows NT 4.0 SP6 x86 CLIENT/SERVER TLS, and a
separate clean-machine TLS run. The `/MD` package additionally passed real
PapinhoBrowser `/MD` TLS 1.3 integration. No ML/MD functional or lifecycle
divergence was observed. The historical unsuffixed 0.6.1 package remains
immutable. The published 0.6.2 release uses library 0.6.2, API 2.1.0 and SPI
3.0. The 0.6.3 candidate retains API 2.1.0/SPI 3.0 and corrects only the NSS
peer TLS-version metadata normalization; targeted external recertification is pending.

## Historical 0.6.1 graceful-shutdown contract bugfix

The 0.6.1 patch retains API 2.1.0, SPI 3.0 and all target identities. It adds the public `PST_CAP_GRACEFUL_SHUTDOWN` vocabulary bit and corrects `require_graceful_shutdown` tri-state validation and pre-binding eligibility. Provider masks below reflect this additive capability; the published 0.6.0 binaries and packages are not modified.

## Historical 0.6.0 release candidate after M9

The 0.6.0 release track keeps the same four build-target identities used by 0.5.0. The new work changes the public secure-transport/scheduler contract, not the target naming model.

Current release versions are API `2.1.0`, SPI `3.0`, and library `0.6.0`. M9 has completed the cross-provider scheduler/security/stress matrix. Final physical NT4, clean-machine, package reproduction and publication validation are M10 work; the rows below record the factual release-candidate evidence.

| Target ID | Architecture | Provider(s) | M0–M9 development status |
|---|---:|---|---|
| `win32-x86-vc6-retrozilla-nss` | x86 | RetroZilla NSS/NSPR | scheduler/readiness, TLS-after-plaintext, partial I/O and cross-provider network gates PASS; final 0.6.0 NT4/package rerun pending M10 |
| `win32-x64-msvc-19.51-schannel` | x64 | Schannel | scheduler/readiness and M9 TLS 1.2 matrix PASS; TLS 1.3 remains outside the validated Schannel capability set; final clean-machine/package rerun pending M10 |
| `win32-x64-msvc-19.51-openssl3` | x64 | OpenSSL 3.5.8 LTS | scheduler/readiness and M9 TLS 1.2/1.3 matrix PASS; final clean-machine/package rerun pending M10 |
| `win32-x64-msvc-19.51-schannel-openssl3` | x64 | Schannel + OpenSSL 3.5.8 LTS | EXACT/ORDERED/AUTOMATIC and capability pre-binding selection PASS; final clean-machine/package rerun pending M10 |

M8 locked the current role-scoped capability masks and M9 reconfirmed them without API/SPI changes:

| Provider | Aggregate | CLIENT | SERVER |
|---|---:|---:|---:|
| OpenSSL | `0x00067fff` | `0x00067eb7` | `0x0004777b` |
| Schannel | `0x00067efd` | `0x00067eb5` | `0x00047679` |
| RetroZilla NSS | `0x00047aff` | `0x00047ab7` | `0x0004727b` |

Important asymmetries remain intentional and factual: OpenSSL and Schannel provide full independent CLIENT SNI control; RetroZilla NSS remains partial because its published snapshot couples the client hostname/SNI behavior through `SSL_SetURL`. RetroZilla NSS is not patched to manufacture parity. Complete SERVER ALPN and SERVER SYSTEM_TRUST remain absent from the NSS role mask, while Schannel TLS 1.3 remains unadvertised for the validated target/environment. Unsupported requirements are filtered before provider binding; M9 observed zero post-binding provider switches.

M9 also found and corrected two real Schannel shutdown defects in the 0.6.0 release tree: reciprocal `close_notify` completion after the peer alert had already been observed, and processing of already-buffered TLS before requesting another socket read. The fixes changed production code but did not change API, SPI or target identity.

---
CRT/runtime model is also an ABI-relevant fact. Per PapinhoEngineering/ADR-0002 revision 3 and PST/ADR-0005, when CRT variants are binary-incompatible and must coexist or be selected by consumers, CRT becomes a discriminating target/variant dimension rather than metadata alone.

The unsuffixed VC6/NSS ID is historical. Its factual CRT is /ML, but new canonical CRT-qualified identities are symmetric:

\`\`\`text
historical: win32-x86-vc6-retrozilla-nss
canonical /ML: win32-x86-vc6-retrozilla-nss-ml
canonical /MD: win32-x86-vc6-retrozilla-nss-md
\`\`\`

Historical packages/evidence keep the unsuffixed name. If /ML is built/distributed again, the new artifact uses `-ml`.

## Current and historical targets at a glance

| Target ID | Architecture | Provider(s) | TLS | Tested on |
|---|---:|---|---|---|
| `win32-x86-vc6-retrozilla-nss` | x86 | RetroZilla NSS/NSPR | historical `/ML` identity; 1.2 / 1.3 | Windows NT 4.0 SP6 x86 |
| `win32-x86-vc6-retrozilla-nss-ml` | x86 | RetroZilla NSS/NSPR | canonical `/ML` variant; 1.2 / 1.3 | Windows NT 4.0 SP6 x86; separate clean Windows machine |
| `win32-x86-vc6-retrozilla-nss-md` | x86 | RetroZilla NSS/NSPR | canonical `/MD` variant; 1.2 / 1.3 | Windows NT 4.0 SP6 x86; separate clean Windows machine; real PapinhoBrowser integration |
| `win32-x64-msvc-19.51-schannel` | x64 | Schannel | 1.2 validated | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-openssl3` | x64 | OpenSSL 3.5.8 LTS | 1.2 / 1.3 | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-schannel-openssl3` | x64 | Schannel + OpenSSL 3.5.8 LTS | capability-dependent | Windows 10 build 19045 x64 |

The compact table is only a navigation summary. The factual details for each target are below, vertically, so they remain readable on narrow screens.

---

## `win32-x86-vc6-retrozilla-nss` — historical unsuffixed ID

| Field | Value |
|---|---|
| Platform / ABI | Win32 |
| Architecture | x86 |
| Built with | Visual C++ 6 SP5 + Processor Pack |
| Compiler | `cl.exe` 12.00.8804 |
| Linker | `link.exe` 6.00.8447 |
| Provider | RetroZilla NSS / NSPR |
| Provider version | NSS 3.42 Beta; NSPR 4.7.7 |
| Provenance | RetroZilla revision `2f274574d3c6ee8769914046920d649bbae9f81b` |
| Patched snapshot SHA-256 | `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388` |
| Linkage | Static PST library; NSS/NSPR runtime DLLs |
| CRT / runtime model | Compiler-default static CRT for PST consumers; retained NSS binaries also import their documented runtime |
| Formal OS support range | Not declared |
| Tested on | **Windows NT 4.0 SP6 x86** |
| Validation status | Release/package gates passed |
| CLIENT | TLS 1.2 / 1.3 validated |
| SERVER | TLS 1.2 / 1.3 validated |
| SERVER trust | CUSTOM_TRUST |
| SERVER SYSTEM_TRUST | Not advertised |
| SERVER ALPN | Complete PST semantics not advertised |
| Notes | `nt4` is validation evidence, not target identity |

### Canonical CRT-qualified identity

The factual CRT for this historical target is `/ML`. Its canonical variant identity for any new artifact is:

\`\`\`text
win32-x86-vc6-retrozilla-nss-ml
\`\`\`

No historical package, hash, release evidence or path is renamed. This is a crosswalk, not history rewriting.

### Why NT4 is not in the target name

The target name identifies durable build facts: Win32 ABI, x86 architecture, VC6 toolchain, and the RetroZilla NSS provider family.

Windows NT 4.0 appears under **Tested on** because that is observed validation evidence. The same artifact is not automatically restricted to NT4 merely because it was tested there, and an unlisted Windows version is not automatically supported.

---

## `win32-x86-vc6-retrozilla-nss-md` — validated CRT-qualified variant

This is the ABI-distinct VC6 variant required and validated by PapinhoBrowser.

| Field | Value |
|---|---|
| Platform / ABI | Win32 |
| Architecture | x86 |
| Toolchain | Visual C++ 6 family |
| Provider | RetroZilla NSS / NSPR |
| PST CRT / runtime model | `/MD` |
| Relationship | ABI-distinct variant of the VC6/NSS line |
| Consumer driver | PapinhoBrowser VC6 `/MD` integration |
| Validation status | M1-M4 build, package, real TLS and lifecycle gates passed |
| Package status | 0.6.3 candidate; targeted external recertification pending |
| Tested on | Windows NT 4.0 SP6 x86; separate clean Windows machine; real PapinhoBrowser `/MD` integration |
| Notes | TLS 1.2/1.3, required mTLS, truncation, reciprocal shutdown and `PR_Poll` authority passed; Browser `/MD` TLS 1.3 passed |

The `/ML` and `/MD` targets have equivalent PST/TLS semantics; their CRT and artifact identities remain deliberately distinct.

---

## `win32-x64-msvc-19.51-schannel`

| Field | Value |
|---|---|
| Platform / ABI | Win32 |
| Architecture | x64 |
| Built with | Visual Studio Build Tools 2026 18.9.2 |
| MSVC toolset | 14.51.36231 |
| Compiler | `cl.exe` 19.51.36256.0 |
| Link/NMAKE | 14.51.36256.0 |
| Windows SDK | 10.0.26100.0 |
| Provider | Windows Schannel |
| Provider version | Operating-system supplied |
| Linkage | Static PST library; Windows system libraries |
| CRT / runtime model | `/MD` |
| Formal OS support range | Not declared |
| Tested on | Windows 10 build 19045 x64; separate clean Windows 10 Pro 22H2 x64 |
| Validation status | Release gates passed |
| CLIENT | TLS 1.2 validated |
| SERVER | TLS 1.2 validated |
| SERVER TLS 1.3 | Not advertised on the validated environment |
| SERVER ALPN | Complete PST semantics not advertised |
| Notes | Schannel has no package-supplied runtime |

The absence of SERVER TLS 1.3 in this row is a factual statement about the validated PST/Schannel environment, not a universal statement that Schannel can never support TLS 1.3.

---

## `win32-x64-msvc-19.51-openssl3`

| Field | Value |
|---|---|
| Platform / ABI | Win32 |
| Architecture | x64 |
| Built with | PST MSVC/SDK baseline used by the x64 targets |
| OpenSSL build | MSVC 19.51.36256, Perl 5.42.2, NASM 3.02 |
| Provider | OpenSSL 3 |
| Exact provider release | OpenSSL 3.5.8 LTS |
| Configure profile | `VC-WIN64A shared no-legacy no-fips no-autoload-config` |
| Provenance | Retained source and SHA-256 manifests |
| Linkage | Static PST library; OpenSSL import libraries and shared DLLs |
| CRT / runtime model | `/MD` |
| Formal OS support range | Not declared |
| Tested on | Windows 10 build 19045 x64; separate clean Windows 10 Pro 22H2 x64 |
| Validation status | Release gates passed |
| CLIENT | TLS 1.2 / 1.3 validated |
| SERVER | TLS 1.2 / 1.3 validated |
| SERVER ALPN | Complete PST semantics validated |
| Notes | `openssl3` identifies the runtime ABI generation; 3.5.8 is the exact release input |

---

## `win32-x64-msvc-19.51-schannel-openssl3`

| Field | Value |
|---|---|
| Platform / ABI | Win32 |
| Architecture | x64 |
| Built with | Same PST MSVC/SDK baseline |
| Providers | Schannel + OpenSSL 3 |
| OpenSSL release | 3.5.8 LTS |
| Linkage | Static PST library; Schannel system libraries; OpenSSL import libraries/shared DLLs |
| CRT / runtime model | `/MD` |
| Formal OS support range | Not declared |
| Tested on | Windows 10 build 19045 x64; separate clean Windows 10 Pro 22H2 x64 |
| Validation status | Release gates passed; official optional SDK |
| Built-in provider order | Schannel, then OpenSSL |
| Selection | EXACT / ORDERED / AUTOMATIC validated |
| SERVER ALPN filtering | Schannel becomes ineligible before binding when complete SERVER ALPN is required; OpenSSL may then be selected |
| Post-binding fallback | Never performed |
| Notes | Optional selection package; not a fourth TLS implementation and not the default/recommended package |

---

## Which target should I download?

### Windows NT 4.0 SP6 x86 in the validated configuration

Use:

```text
win32-x86-vc6-retrozilla-nss
```

This is the target actually validated on real NT4 SP6 x86, including TLS 1.2 and TLS 1.3 CLIENT/SERVER scenarios.

### Windows x64 and you want the OS-native TLS path validated by PST

Use:

```text
win32-x64-msvc-19.51-schannel
```

### Windows x64 and you need the validated OpenSSL 3 TLS 1.2 / TLS 1.3 path

Use:

```text
win32-x64-msvc-19.51-openssl3
```

### Windows x64 and you explicitly need both Schannel and OpenSSL in one PST library

Use:

```text
win32-x64-msvc-19.51-schannel-openssl3
```

The Combined package exists for provider selection. It is optional and is not a default recommendation.

---

## Supported on vs. Tested on

`Supported on` and `Tested on` are deliberately different concepts.

PST does not infer a formal OS support range merely from a successful test. Likewise, a target built with VC6 is not “an NT4 target” just because NT4 is one validated system.

An unlisted operating system is **unvalidated**, not automatically incompatible.

---

## 0.5.0 role-scoped provider status

OpenSSL SERVER is implemented and validated on the x64 OpenSSL target with TLS 1.2/1.3.

Schannel SERVER is validated for TLS 1.2. TLS 1.3 SERVER and complete PST SERVER ALPN remain unadvertised factual limitations of the validated environment/provider behavior.

RetroZilla NSS SERVER is validated for TLS 1.2/1.3 on the VC6 x86 target, including real NT4 SP6 execution, and advertises only its proven role-scoped capabilities.

Cross-process network interoperability between x86 NSS and both x64 providers is validated. Same-process three-provider composition is not applicable because the NSS target is x86 while the current Combined target is x64.

### Exact role-scoped capability masks

| Provider | Aggregate | CLIENT | SERVER |
|---|---:|---:|---:|
| OpenSSL | `0x00067fff` | `0x00067eb7` | `0x0004777b` |
| Schannel | `0x00067efd` | `0x00067eb5` | `0x00047679` |
| RetroZilla NSS | `0x00047aff` | `0x00047ab7` | `0x0004727b` |

---

## Historical identifier crosswalk

The following identifiers name already completed pre-ADR validation artifacts. Historical evidence under `docs/codex/release-evidence/0.4.0/` retains those names verbatim.

```text
windows-nt4-x86-vc6-retrozilla-nss
-> win32-x86-vc6-retrozilla-nss
-> win32-x86-vc6-retrozilla-nss-ml   (canonical CRT-qualified identity for new /ML artifacts)

windows-x64-msvc-schannel
-> win32-x64-msvc-19.51-schannel

windows-x64-msvc-openssl-3.5.8
-> win32-x64-msvc-19.51-openssl3

windows-x64-msvc-schannel-openssl-3.5.8
-> win32-x64-msvc-19.51-schannel-openssl3
```

## Naming rule

`win32` denotes the Win32 API/ABI family and does not imply x86. Architecture is a separate field.

Full compiler servicing builds, SDK versions, exact dependency patch releases, support claims, and test environments remain matrix/provenance data unless they define an incompatible artifact.

CRT follows the same rule: it may remain matrix metadata when it does not discriminate artifacts; when it creates an incompatible selectable variant, it is represented in the target/variant identity.
