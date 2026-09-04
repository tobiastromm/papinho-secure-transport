# Build targets

Run commands from the repository root.

| Target | Command | Output |
|---|---|---|
| VC6 / RetroZilla NSS | `tools\build-vc6.bat clean` then `tools\build-vc6.bat test test-nss-unit` | `build\vc6` |
| Modern Schannel x64 | `tools\build-modern-msvc.bat clean` then `tools\build-modern-msvc.bat test` | `build\win64-modern-msvc` |
| Modern OpenSSL x64 | `tools\build-modern-msvc-openssl.bat clean` then `tools\build-modern-msvc-openssl.bat test` | `build\win64-modern-msvc-openssl` |
| Combined validation | `tools\build-modern-msvc-combined.bat clean` then `tools\build-modern-msvc-combined.bat combined-test` | `build\win64-modern-msvc-combined` |

VC6 uses C89 and `/W4`. Its preserved NSS/NSPR SDK, runtime, hashes, source snapshot, patches, and notices are repository-contained; active builds do not depend on `C:\PSTW`. Reproducibility is Level B, not a byte-identical-build claim.

Modern builds use the documented MSVC bootstrap, x64 `/MD /W4`. Schannel is supplied by Windows; tested TLS 1.2 evidence covers Windows 10 build 19045, where this adapter does not advertise TLS 1.3. OpenSSL uses the staged 3.5.8 headers, libraries, runtime DLLs, and manifests. Public-network SYSTEM_TRUST tests are opt-in and environment-dependent. The combined target is a validation model, not a promised release package; Phase 9.E decides packaging.

Details: [VC6](../build-vc6.md), [modern MSVC](../build-modern-msvc.md), [OpenSSL](../build-openssl.md).