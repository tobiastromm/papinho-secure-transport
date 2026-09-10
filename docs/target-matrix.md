# Target Matrix

This is the canonical PapinhoSecureTransport target matrix. The ecosystem-wide naming authority is PapinhoEngineering ADR-0002 revision 2; this document applies that decision to PST.

Target identity, build inputs, operating-system support policy, and observed validation are separate facts. A toolchain name is not an operating-system support claim.

## 0.5.0 candidate targets at a glance

| Target ID | Architecture | Provider(s) | TLS | Tested on |
|---|---:|---|---|---|
| `win32-x86-vc6-retrozilla-nss` | x86 | RetroZilla NSS/NSPR | 1.2 / 1.3 | Windows NT 4.0 SP6 x86 |
| `win32-x64-msvc-19.51-schannel` | x64 | Schannel | 1.2 validated | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-openssl3` | x64 | OpenSSL 3.5.8 LTS | 1.2 / 1.3 | Windows 10 build 19045 x64 |
| `win32-x64-msvc-19.51-schannel-openssl3` | x64 | Schannel + OpenSSL 3.5.8 LTS | capability-dependent | Windows 10 build 19045 x64 |

The compact table is only a navigation summary. The factual details for each target are below, vertically, so they remain readable on narrow screens.

---

## `win32-x86-vc6-retrozilla-nss`

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

### Why NT4 is not in the target name

The target name identifies durable build facts: Win32 ABI, x86 architecture, VC6 toolchain, and the RetroZilla NSS provider family.

Windows NT 4.0 appears under **Tested on** because that is observed validation evidence. The same artifact is not automatically restricted to NT4 merely because it was tested there, and an unlisted Windows version is not automatically supported.

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
| OpenSSL | `0x00027fff` | `0x00027eb7` | `0x0000777b` |
| Schannel | `0x00027efd` | `0x00027eb5` | `0x00007679` |
| RetroZilla NSS | `0x00007aff` | `0x00007ab7` | `0x0000727b` |

---

## Historical identifier crosswalk

The following identifiers name already completed pre-ADR validation artifacts. Historical evidence under `docs/codex/release-evidence/0.4.0/` retains those names verbatim.

```text
windows-nt4-x86-vc6-retrozilla-nss
-> win32-x86-vc6-retrozilla-nss

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
