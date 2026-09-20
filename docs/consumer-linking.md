<!-- SPDX-License-Identifier: MPL-2.0 -->

# Consumer linking and runtime manifest

Consumers include only `papinho_secure_transport.h` and, for Win32 transport/bootstrap, `papinho_secure_transport_win32.h`. They never include the provider SPI or private source headers.

| Target ID | PST/static link inputs | Additional link inputs | Package runtime | OS/toolchain runtime |
|---|---|---|---|---|
| `win32-x86-vc6-retrozilla-nss` | `papinho_secure_transport.lib` | `wsock32.lib` | `nss3.dll`, `ssl3.dll`, `nssutil3.dll`, `nspr4.dll`, `plc4.dll`, `plds4.dll`, `softokn3.dll/.chk`, `freebl3.dll/.chk` | Historical unsuffixed identity; factual PST CRT `/ML`; frozen packages/evidence retain this name |
| `win32-x86-vc6-retrozilla-nss-ml` | `papinho_secure_transport.lib` | `wsock32.lib` | same RetroZilla NSS/NSPR runtime set, when built/validated | Canonical identity for any new VC6 `/ML` artifact; no new package implied |
| `win32-x86-vc6-retrozilla-nss-md` | `papinho_secure_transport.lib` | `wsock32.lib` | same RetroZilla NSS/NSPR runtime set, subject to package validation | PST compiled with VC6 `/MD`; intended for `/MD` consumers such as PapinhoBrowser; **not yet released/validated** |
| `win32-x64-msvc-19.51-schannel` | `papinho_secure_transport.lib` | `ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib` | none | Windows system DLLs plus MSVC `/MD` runtime/UCRT |
| `win32-x64-msvc-19.51-openssl3` | `papinho_secure_transport.lib libssl.lib libcrypto.lib` | `ws2_32.lib crypt32.lib` | `libssl-3-x64.dll`, `libcrypto-3-x64.dll` | Windows system DLLs plus MSVC `/MD` runtime/UCRT |
| `win32-x64-msvc-19.51-schannel-openssl3` | `papinho_secure_transport.lib libssl.lib libcrypto.lib` | `ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib` | `libssl-3-x64.dll`, `libcrypto-3-x64.dll` | Windows system DLLs plus MSVC `/MD` runtime/UCRT |

The NSS PST library loads provider DLLs privately, so NSS import libraries are not consumer inputs. `nssdbm3.dll/.chk` are excluded from the minimal no-database runtime: real provider evidence loaded NSS, SSL, NSPR, softokn and freebl without nssdbm. Future database behavior would require a package-manifest and runtime-proof update.

OpenSSL import libraries are third-party link inputs and live under the target library directory. Its DLLs should be copied beside the final application executable. The Schannel package redistributes no Windows DLL.

Canonical tool inspection confirmed COFF static archives, not DLL import libraries. VC6 contains x86 core, Win32 transport, built-in manifest and NSS provider objects. MSVC archives contain x64 core plus respectively Schannel, or OpenSSL with its Windows system-trust adapter. The combined validation archive contains both.

PST handles are released through PST functions; consumers do not free PST allocations with their CRT. This ownership boundary does not make incompatible static-library CRT variants interchangeable: consumers must select a package whose documented CRT/runtime model is compatible with their build. A future DLL still requires a new CRT-boundary audit.

Do not change a multithreaded consumer to VC6 `/ML` merely to accommodate the historical static-CRT package. Produce/select the compatible PST variant instead.

API 2.0 consumers set `PST_CONNECTION_CONFIG.role` explicitly. Applications own listening sockets; PST accepts ownership only of an already accepted/connected transport after `pst_connection_attach` reports `ownership_accepted`. SERVER selection is role- and capability-scoped, so consumers must not infer SERVER support from an aggregate provider mask.
