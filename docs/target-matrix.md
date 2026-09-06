# Target Matrix

This is the canonical PapinhoSecureTransport target matrix. The ecosystem-wide naming authority is PapinhoEngineering ADR-0002 revision 2; this document applies that decision to PST. Target identity, build inputs, support policy, and observed validation are separate facts.

## Official 0.4.0 targets

| Target ID | Platform / ABI | Architecture | Built with | Provider(s) | Provider version / provenance | Linkage | CRT / runtime model | Supported on | Tested on | Validation status | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `win32-x86-vc6-retrozilla-nss` | Win32 | x86 | Visual C++ 6 SP5 + Processor Pack; `cl.exe` 12.00.8804; `link.exe` 6.00.8447 | RetroZilla NSS | NSS 3.42 Beta; NSPR 4.7.7; RetroZilla revision `2f274574d3c6ee8769914046920d649bbae9f81b`; patched snapshot SHA-256 `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388` | Static PST library; NSS/NSPR runtime DLLs | Compiler-default static CRT for PST consumers; retained NSS binaries also import their documented runtime | No formal operating-system range declared | Windows NT 4.0 SP6 x86 | Release gates passed | `nt4` is validation evidence, not target identity. |
| `win32-x64-msvc-19.51-schannel` | Win32 | x64 | Visual Studio Build Tools 2026 18.9.2; MSVC toolset 14.51.36231; `cl.exe` 19.51.36256.0; link/NMAKE 14.51.36256.0; Windows SDK 10.0.26100.0 | Schannel | Operating-system supplied | Static PST library; Windows system libraries | `/MD` | No formal operating-system range declared | Windows 10 build 19045 x64; separate clean Windows 10 Pro 22H2 x64 | Release gates passed | Schannel has no package-supplied runtime. |
| `win32-x64-msvc-19.51-openssl3` | Win32 | x64 | Same PST MSVC/SDK baseline; OpenSSL built with MSVC 19.51.36256, Perl 5.42.2 and NASM 3.02 | OpenSSL 3 | OpenSSL 3.5.8 LTS; `VC-WIN64A shared no-legacy no-fips no-autoload-config`; retained source and SHA-256 manifests | Static PST library; OpenSSL import libraries and shared DLLs | `/MD` | No formal operating-system range declared | Windows 10 build 19045 x64; separate clean Windows 10 Pro 22H2 x64 | Release gates passed | `openssl3` identifies the runtime ABI generation; 3.5.8 is the exact release input. |
| `win32-x64-msvc-19.51-schannel-openssl3` | Win32 | x64 | Same PST MSVC/SDK baseline | Schannel + OpenSSL 3 | OS Schannel plus retained OpenSSL 3.5.8 LTS | Static PST library; Schannel system libraries; OpenSSL import libraries/shared DLLs | `/MD` | No formal operating-system range declared | Windows 10 build 19045 x64; separate clean Windows 10 Pro 22H2 x64 | Release gates passed; official optional SDK | Built-in order is Schannel then OpenSSL. It is not the default or recommended package. |

`Supported on` is deliberately not inferred from `Tested on`. An unlisted system is unvalidated, not automatically incompatible.

## Historical identifier crosswalk

The following identifiers name the already completed pre-ADR validation artifacts. Historical evidence under `docs/codex/release-evidence/0.4.0/` retains those names verbatim.

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

## Naming and download rule

`win32` denotes the Win32 API/ABI family and does not imply x86. Architecture is a separate field. Full compiler servicing builds, SDK versions, dependency patch versions, support claims, and test environments remain matrix/provenance data unless they define an incompatible artifact.

| If you use | Download target |
|---|---|
| Windows NT 4.0 SP6 x86 in the validated configuration | `win32-x86-vc6-retrozilla-nss` |
| Windows x64 and need the validated OS-native TLS 1.2 path | `win32-x64-msvc-19.51-schannel` |
| Windows x64 and need the validated OpenSSL 3 TLS 1.2/TLS 1.3 path | `win32-x64-msvc-19.51-openssl3` |
| Windows x64 and explicitly need both providers in one static PST library | `win32-x64-msvc-19.51-schannel-openssl3` |
