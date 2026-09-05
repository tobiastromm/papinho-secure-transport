# Consumer linking and runtime manifest

Consumers include only `papinho_secure_transport.h` and, for Win32 transport/bootstrap, `papinho_secure_transport_win32.h`. They never include the provider SPI or private source headers.

| Target ID | PST/static link inputs | Additional link inputs | Package runtime | OS/toolchain runtime |
|---|---|---|---|---|
| `windows-nt4-x86-vc6-retrozilla-nss` | `papinho_secure_transport.lib` | `wsock32.lib` | `nss3.dll`, `ssl3.dll`, `nssutil3.dll`, `nspr4.dll`, `plc4.dll`, `plds4.dll`, `softokn3.dll/.chk`, `freebl3.dll/.chk` | Windows DLLs; NSS also imports `MSVCRT.dll`; current PST consumer flags statically link the VC6 CRT |
| `windows-x64-msvc-schannel` | `papinho_secure_transport.lib` | `ws2_32.lib secur32.lib crypt32.lib ncrypt.lib` | none | Windows system DLLs plus MSVC `/MD` runtime/UCRT |
| `windows-x64-msvc-openssl-3.5.8` | `papinho_secure_transport.lib libssl.lib libcrypto.lib` | `ws2_32.lib crypt32.lib` | `libssl-3-x64.dll`, `libcrypto-3-x64.dll` | Windows system DLLs plus MSVC `/MD` runtime/UCRT |

The NSS PST library loads provider DLLs privately, so NSS import libraries are not consumer inputs. `nssdbm3.dll/.chk` are excluded from the minimal no-database runtime: real provider evidence loaded NSS, SSL, NSPR, softokn and freebl without nssdbm. Future database behavior would require a package-manifest and runtime-proof update.

OpenSSL import libraries are third-party link inputs and live under the target library directory. Its DLLs should be copied beside the final application executable. The Schannel package redistributes no Windows DLL.

Canonical tool inspection confirmed COFF static archives, not DLL import libraries. VC6 contains x86 core, Win32 transport, built-in manifest and NSS provider objects. MSVC archives contain x64 core plus respectively Schannel, or OpenSSL with its Windows system-trust adapter. The combined validation archive contains both.

PST handles are released through PST functions; consumers do not free PST allocations with their CRT. A future DLL still requires a new CRT-boundary audit.
