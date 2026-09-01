# Repository instructions

## VC6 BUILD — READ THIS BEFORE BUILDING

- Do not rediscover the compiler installation or search the disk for `cl.exe`/library paths unless the documented bootstrap reports that they are unavailable.
- Read `docs/build-vc6.md` and use `tools\vc6-env.bat` (or `tools\build-vc6.bat`).
- `Makefile.vc6` is the build definition; VC6 `/W4` is required.
- Run the regular and NSS regression commands documented in `docs/build-vc6.md`.
- If bootstrap fails, report the exact missing dependency and requested environment variable before attempting broader discovery. Do not silently switch toolchains.

Codex quick start: run `tools\build-vc6.bat clean`, then `tools\build-vc6.bat test test-nss-unit`.