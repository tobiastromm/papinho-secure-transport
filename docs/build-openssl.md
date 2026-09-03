# OpenSSL provider build

The OpenSSL provider is a separate modern-MSVC x64 target. It never participates in the VC6/RetroZilla NSS or Schannel-only builds.

## Dependency preparation

From an ordinary `cmd.exe`, run `tools\openssl-build-env.bat`. It first invokes the canonical modern MSVC bootstrap, then resolves Perl and NASM from `PST_OPENSSL_PERL` / `PST_OPENSSL_NASM`, inherited PATH, or conventional machine-wide Strawberry Perl and NASM locations. The overrides must name executable files; no user-specific path is embedded in a Makefile.

Verify `third_party\openssl\source\MANIFEST.sha256`, extract into a disposable build directory, and configure the exact source as follows (choose disposable absolute prefix/config paths):

```bat
perl Configure VC-WIN64A shared no-legacy no-fips no-autoload-config --prefix=<staging> --openssldir=<private-config>
nmake
nmake test
nmake install_sw
```

The validated build used OpenSSL 3.5.8, Perl 5.42.2, NASM 3.02, MSVC 19.51.36256 x64, linker 14.51.36256.0, SDK 10.0.26100.0, and `/MD`. The mandatory upstream suite passed 4137 tests across 346 test files. Do not replace the retained prebuilt directory unless the exact source hash, complete upstream test gate, staging audit, and manifests are regenerated.

## PST OpenSSL target

```bat
tools\build-modern-msvc-openssl.bat clean
tools\build-modern-msvc-openssl.bat test
```

`Makefile.openssl.msvc` writes only to `build\win64-modern-msvc-openssl`, compiles PST code with `/MD /W4`, links only the retained OpenSSL import library, and copies the exact retained DLLs next to the test executable. The target registers only `openssl`. Its public-header consumer includes only `papinho_secure_transport.h`.

The target implements OSSL-C TLS 1.2/TLS 1.3, nonblocking handshake, secure I/O, bounded readiness and shutdown. Run `tools\build-modern-msvc-openssl.bat runtime-integration` to build the functional client; the canonical PowerShell runner is `tests\run_openssl_runtime_integration.ps1`. Real evidence is written under ignored `build\phase-ossl-c`. Full public trust, hostname, ALPN, mTLS and peer information remain OSSL-D.


## OSSL-D identity integration

The build also produces `test_openssl_identity_integration.exe` for custom-trust, hostname, ALPN, mTLS and peer-snapshot gates. Server PEM files are fixture-only; the PST client consumes trust, certificate and PKCS#8 DER exclusively from memory. Evidence is stored under ignored `build\phase-ossl-d`.

## Windows SYSTEM_TRUST integration

The default `test` target remains offline and includes the deterministic Win32 adapter test. Public SYSTEM trust is an explicit environment-dependent target:

```bat
tools\build-modern-msvc-openssl.bat system-trust-integration
```

It invokes `tests\run_openssl_system_trust_integration.ps1`. Override its configurable endpoint list with `-Endpoints`; DNS, reachability and remote-policy failures are reported separately from a PST trust failure. The runner sends no application payload and does not configure a custom CA.