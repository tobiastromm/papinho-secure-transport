<!-- SPDX-License-Identifier: MPL-2.0 -->

# Repository instructions

## VC6 BUILD — READ THIS BEFORE BUILDING

- Do not rediscover the compiler installation or search the disk for `cl.exe`/library paths unless the documented bootstrap reports that they are unavailable.
- Read `docs/build-vc6.md` and use `tools\vc6-env.bat` (or `tools\build-vc6.bat`).
- `Makefile.vc6` is the build definition; VC6 `/W4` is required.
- Run the regular and NSS regression commands documented in `docs/build-vc6.md`.
- If bootstrap fails, report the exact missing dependency and requested environment variable before attempting broader discovery. Do not silently switch toolchains.

Codex quick start: run `tools\build-vc6.bat clean`, then `tools\build-vc6.bat test test-nss-unit`.
## MODERN MSVC X64 BUILD

- Read `docs/build-msvc-19.51.md`; do not manually rediscover MSVC/SDK paths unless its bootstrap fails.
- Use `tools\build-win32-x64-msvc-19.51-schannel.bat clean`, then `tools\build-win32-x64-msvc-19.51-schannel.bat test` from an ordinary shell.
- The bootstrap uses official `vswhere.exe` discovery and selects the newest complete C++ Build Tools installation. `PST_MSVC_19_51_VCVARS64` is the deliberate override.
- `Makefile.msvc` builds x64 with `/MD /W4` into `build\win32-x64-msvc-19.51-schannel`; zero warnings are required.
- Preserve the `LIB` and `INCLUDE` environment emitted by `vcvars64.bat`; never reuse `LIB` as an NMAKE project macro.
