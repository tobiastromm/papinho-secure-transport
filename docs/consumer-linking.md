<!-- SPDX-License-Identifier: MPL-2.0 -->

# Consumer linking and runtime manifest

Consumers include only `papinho_secure_transport.h` and, for Win32 transport/bootstrap, `papinho_secure_transport_win32.h`. They never include the provider SPI or private source headers.

| Target ID | PST/static link inputs | Additional link inputs | Package runtime | OS/toolchain runtime |
|---|---|---|---|---|
| `win32-x86-vc6-retrozilla-nss` | `papinho_secure_transport.lib` | `wsock32.lib` | `nss3.dll`, `ssl3.dll`, `nssutil3.dll`, `nspr4.dll`, `plc4.dll`, `plds4.dll`, `softokn3.dll/.chk`, `freebl3.dll/.chk` | Historical unsuffixed identity; factual PST CRT `/ML`; frozen packages/evidence retain this name |
| `win32-x86-vc6-retrozilla-nss-ml` | `papinho_secure_transport.lib` | `wsock32.lib` | same RetroZilla NSS/NSPR runtime set | Explicit `/ML` 0.6.3 SDK candidate; targeted external recertification pending |
| `win32-x86-vc6-retrozilla-nss-md` | `papinho_secure_transport.lib` | `wsock32.lib` | same RetroZilla NSS/NSPR runtime set | Explicit `/MD` 0.6.3 SDK candidate; targeted external recertification pending |
| `win32-x64-msvc-19.51-schannel` | `papinho_secure_transport.lib` | `ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib` | none | Windows system DLLs plus MSVC `/MD` runtime/UCRT |
| `win32-x64-msvc-19.51-openssl3` | `papinho_secure_transport.lib libssl.lib libcrypto.lib` | `ws2_32.lib crypt32.lib` | `libssl-3-x64.dll`, `libcrypto-3-x64.dll` | Windows system DLLs plus MSVC `/MD` runtime/UCRT |
| `win32-x64-msvc-19.51-schannel-openssl3` | `papinho_secure_transport.lib libssl.lib libcrypto.lib` | `ws2_32.lib secur32.lib crypt32.lib ncrypt.lib bcrypt.lib` | `libssl-3-x64.dll`, `libcrypto-3-x64.dll` | Windows system DLLs plus MSVC `/MD` runtime/UCRT |

The NSS PST library loads provider DLLs privately, so NSS import libraries are not consumer inputs. `nssdbm3.dll/.chk` are excluded from the minimal no-database runtime: real provider evidence loaded NSS, SSL, NSPR, softokn and freebl without nssdbm. Future database behavior would require a package-manifest and runtime-proof update.

OpenSSL import libraries are third-party link inputs and live under the target library directory. Its DLLs should be copied beside the final application executable. The Schannel package redistributes no Windows DLL.

Canonical tool inspection confirmed COFF static archives, not DLL import libraries. VC6 contains x86 core, Win32 transport, built-in manifest and NSS provider objects. MSVC archives contain x64 core plus respectively Schannel, or OpenSSL with its Windows system-trust adapter. The combined validation archive contains both.

PST handles are released through PST functions; consumers do not free PST allocations with their CRT. This ownership boundary does not make incompatible static-library CRT variants interchangeable: consumers must select a package whose documented CRT/runtime model is compatible with their build. A future DLL still requires a new CRT-boundary audit.

Do not change a multithreaded consumer to VC6 `/ML` merely to accommodate the historical static-CRT package. Produce/select the compatible PST variant instead.

For 0.6.2 and later VC6 packages, read `crt=ml|md` and `crt_compiler_flag=/ML|/MD`
from `consumer-link.ini` before compiling. The same CRT appears in
`manifest.ini`; the package validator also checks the library's COFF
default-library directives. Neither variant is a substitute for the other.

API 2.0 consumers set `PST_CONNECTION_CONFIG.role` explicitly. Applications own listening sockets; PST accepts ownership only of an already accepted/connected transport after `pst_connection_attach` reports `ownership_accepted`. SERVER selection is role- and capability-scoped, so consumers must not infer SERVER support from an aggregate provider mask.

## Current API 2.2 transport-termination contract

The current candidate is library 0.6.5 (0x00000605UL), additive public API 2.2.0 and unchanged provider SPI 3.0. API 2.1 layouts and symbols remain compatible; no SPI vtable hook or layout was added.

API 2.2 adds pst_transport_termination_info_init(PST_TRANSPORT_TERMINATION_INFO *) and pst_connection_copy_transport_termination(const pst_connection *, PST_TRANSPORT_TERMINATION_INFO *). The structure has eight pst_u32 fields in order: struct_size, api_version, valid, termination, operation, native_domain, native_code, native_code_valid. Minimum and actual size are 32 bytes, alignment 4, offsets 0,4,8,12,16,20,24,28. A prefix below 32 or incompatible API major is rejected; a larger same-major record preserves its unknown tail.

valid=0 means no event was observed. valid=1 with UNKNOWN means an event was observed but not classified portably. native_code_valid=0 means unavailable; otherwise native_domain names the provider namespace and native_code is an opaque 32-bit pattern. TLS result, portable FIN/RST/SOCKET_ERROR/UNKNOWN, and optional native evidence are independent. Native evidence is diagnostic, not causal proof or authority for trust, PST_RESULT_TRUNCATED, retry, fallback, framing, shutdown, ownership or selection.

The first event is immutable while the connection exists. close_notify alone does not establish FIN; timeout, wake, local cancel and release fabricate no event. After peer close_notify, read may report CLOSED/CLEAN; later application read/write are invalid while get_interest and reciprocal shutdown remain available until shutdown_step reaches final CLOSED. That intermediate bookkeeping state is private. Fatal/truncated states remain terminal, with exactly-once release and no resurrection.

NSS maps factual NSPR EOF/reset/socket errors, Schannel maps recv zero/WSAECONNRESET/socket errors, and OpenSSL maps unexpected EOF or reliable SSL_ERROR_SYSCALL socket evidence; ambiguity remains UNKNOWN. No provider switches after binding. VC6/NSS /ML and /MD remain distinct; Browser uses /MD and Accelerator the governed x64 OpenSSL target.
