<!-- SPDX-License-Identifier: MPL-2.0 -->

# Modern MSVC x64 build

The canonical modern Windows target uses the newest complete Visual Studio C++ Build Tools installation discovered by `vswhere.exe`. It is independent from the VC6/RetroZilla NSS target.

## Canonical environment

The validated installation is Visual Studio Build Tools 2026 18.9.2 with MSVC tools 14.51.36231, `cl.exe` 19.51.36256.0, linker 14.51.36256.0, and Windows SDK 10.0.26100.0. The validated bootstrap is:

```text
C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat
```

`tools\msvc-env.bat` discovers the newest complete C++ installation through the official `vswhere.exe`. Set `PST_MODERN_VCVARS64` only to deliberately override discovery; the override path is validated before use.

The build consumes `LIB` and `INCLUDE` exactly as produced by `vcvars64.bat`. Do not reconstruct standard MSVC or SDK library paths in `Makefile.msvc`, and do not name a project make macro `LIB`, because NMAKE exports that macro over the linker's library search environment.

The current CRT choice is dynamic `/MD`. Phase 9 may revisit redistribution policy.

## Commands

From an ordinary clean `cmd /d` shell:

```bat
tools\build-modern-msvc.bat clean
tools\build-modern-msvc.bat test
```

Outputs are isolated under `build\win64-modern-msvc`. The test target compiles and links the common PST core for x64, runs a public-header-only consumer, and runs the Schannel lifecycle/selection/ownership matrix and deterministic TLS buffer/readiness tests. `/W4` with zero warnings is required.

The modern target does not build RetroZilla NSS. The legacy target remains `tools\build-vc6.bat` and is documented separately in `docs\build-vc6.md`.

## Schannel TLS integration

After the normal `test` target builds the integration executable, run the local Python/OpenSSL TLS 1.2 gate with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\run_schannel_runtime_integration.ps1 -TlsVersion 12 -Port 8476 -Exchanges 10
```

Use `-CloseMode peer-clean` or `-CloseMode peer-abrupt` for the close-classification gates. The runner temporarily adds and removes the exact test root under `CurrentUser/Root`. TLS 1.3 is not available from the currently validated Schannel runtime and is not advertised.