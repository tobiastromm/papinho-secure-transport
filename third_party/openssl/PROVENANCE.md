# OpenSSL 3.5.8 provenance

- Upstream: OpenSSL project official release `openssl-3.5.8.tar.gz`
- Release: OpenSSL 3.5.8 LTS, 2026-08-25
- Source URL: `https://github.com/openssl/openssl/releases/download/openssl-3.5.8/openssl-3.5.8.tar.gz`
- Source SHA-256: `a8f84a39918ec6415ce765d9b429d313ba97b8143169c172e734b9514464f5b2`
- License: Apache License 2.0; exact upstream `LICENSE.txt` preserved
- Local source patches: none
- Target: `VC-WIN64A`, shared
- Configure: `perl Configure VC-WIN64A shared no-legacy no-fips no-autoload-config --prefix=C:/Projetos/PapinhoSecureTransport/build/openssl-3.5.8-stage --openssldir=C:/Projetos/PapinhoSecureTransport/build/openssl-3.5.8-config`
- Perl: Strawberry Perl 5.42.2, `MSWin32-x64-multi-thread`
- NASM: 3.02 (2026-06-28)
- MSVC: compiler 19.51.36256 x64; linker 14.51.36256.0; Build Tools 2026 18.9.2
- Windows SDK: 10.0.26100.0
- CRT: dynamic `/MD`
- `nmake`: PASS
- `nmake test`: PASS, 346 files / 4137 tests
- `nmake install_sw`: PASS

The staged runtime contains only `libcrypto-3-x64.dll` and `libssl-3-x64.dll`. The OpenSSL default cryptographic provider is built into libcrypto and is loaded explicitly into each PST runtime's private `OSSL_LIB_CTX`. No legacy or FIPS module is built or staged. Configuration autoload is disabled; the skeleton test succeeds with deliberately nonexistent `OPENSSL_CONF` and `OPENSSL_MODULES` values.

The manifests record the exact source and staged artifact hashes. Tool versions, timestamps, compiler behavior, and other inputs can affect bytes, so this record does not claim reproducible binaries beyond the retained artifacts and hashes.
