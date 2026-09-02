# Visual C++ 6 build

This is the single source of truth for the Win32 x86 VC6 developer build. Future automation should use the bootstrap below instead of rediscovering compiler or linker paths.

## Quick start

From the repository root in a clean `cmd.exe`:

```bat
tools\build-vc6.bat clean
tools\build-vc6.bat test test-nss-unit
```

The first command removes `build\vc6`. The second performs the `/W4` portable build, runs the regular suite, builds the RetroZilla NSS backend, and runs `test_backend_nss`.

## Prerequisites and configuration

The known development-machine default is `C:\MSVC600-master`. The environment audit resolved the active tools as:

- compiler: `C:\MSVC600-master\VC98\Bin\cl.exe`;
- linker: `C:\MSVC600-master\VC98\Bin\link.exe`;
- build tool: `C:\MSVC600-master\VC98\Bin\nmake.exe`.

Override the root without editing tracked files:

```bat
set PST_VC6_ROOT=D:\path\to\vc6
```

`PST_VC6_ROOT` must contain `VC98\Bin\cl.exe`, `link.exe`, `nmake.exe`, and `VCVARS32.BAT`. The bootstrap calls `VCVARS32.BAT`, then normalizes:

- `PATH`: VC6 Common/Tools and `VC98\Bin`, as established by `VCVARS32.BAT`;
- `INCLUDE`: `VC98\ATL\Include`, `VC98\Include`, and `VC98\MFC\Include`;
- `LIB`: `%PST_VC6_ROOT%\VC98\Lib;%PST_VC6_ROOT%\VC98\MFC\Lib`;
- `LDFLAGS`: `/link /LIBPATH:%PST_VC6_ROOT%\VC98\Lib /LIBPATH:%PST_VC6_ROOT%\VC98\MFC\Lib`.

The recurring “LIBPATH” issue was not a `LIBPATH` environment variable. `link.exe` previously failed with `LNK1104: cannot open file "LIBC.lib"`; `LIBC.lib` is in `VC98\Lib`. The known-good invocation passes explicit `/LIBPATH:` switches through the Makefile's existing `LDFLAGS` macro. The bootstrap also sets `LIB`, but `LDFLAGS` is the explicit fix that reproduced successful links.

No separate Windows/Platform SDK path is used by the current build. VC6 supplies `wsock32.lib` from `VC98\Lib`.

## RetroZilla NSS SDK and runtime

NSS compilation needs a RetroZilla `dist` tree containing `include\nspr\prio.h` and `public\nss\ssl.h`. Override the SDK location without editing tracked files:

```bat
set PST_NSS_DIST=D:\path\to\retrozilla\dist
```

On the current development machine the bootstrap recognizes `C:\PSTW\pr\projects\RetroZilla\obj-rzSuite-release\dist`. It exports the selected path as `NSS_DIST`, which `Makefile.vc6` already consumes.

Build-time headers/library paths and runtime DLL discovery are separate. TLS/NSS processes must prepend only this canonical runtime to their process `PATH`:

```text
third_party\retrozilla-nss\prebuilt\win32-x86-vc6\runtime
```

Do not copy these DLLs to `SYSTEM32` and do not use unrelated NSS installations from the ambient `PATH`. `vc6-env.bat` exposes the absolute repository path as `PST_NSS_RUNTIME`; a test runner should set its process `PATH` to `%PST_NSS_RUNTIME%;%SystemRoot%\System32;%SystemRoot%` before launching the integration executable.

## Commands

To retain the environment for several manual commands:

```bat
call tools\vc6-env.bat
nmake /f Makefile.vc6 clean
nmake /f Makefile.vc6 test
nmake /f Makefile.vc6 test-nss-unit
```

Equivalent convenience-driver commands are:

```bat
tools\build-vc6.bat clean
tools\build-vc6.bat test
tools\build-vc6.bat test-nss-unit
tools\build-vc6.bat runtime-integration
```

Regular `test` runs `test_foundation`, `test_identity`, `test_backend_spi`, `test_diagnostic`, `test_diagnostic_transport`, `test_diagnostic_creation`, `test_public_diagnostic`, and `test_public_header`. `test-nss-unit` runs `test_backend_nss`.

After building, individual tests can be rerun directly:

```bat
build\vc6\test_foundation.exe
build\vc6\test_identity.exe
build\vc6\test_backend_spi.exe
build\vc6\test_diagnostic.exe
build\vc6\test_diagnostic_transport.exe
build\vc6\test_diagnostic_creation.exe
build\vc6\test_public_diagnostic.exe
build\vc6\test_public_header.exe
build\vc6\test_backend_nss.exe
```

TLS integration requires `runtime-integration`, the fixture server in `tests\nt4_tls_server.py`, client fixtures under `build\nt4-validation\client`, and the canonical runtime-only process `PATH`. For TLS 1.2, start the fixture in one shell:

```bat
python tests\nt4_tls_server.py 127.0.0.1 8443 build\nt4-validation\server-fixture\server.pem build\nt4-validation\server-fixture\server.key build\nt4-validation\server-fixture\ca.pem 12 fixture/1 required
```

Then run the client from another bootstrapped shell:

```bat
call tools\vc6-env.bat
set PATH=%PST_NSS_RUNTIME%;%SystemRoot%\System32;%SystemRoot%
build\vc6\test_tls_runtime_integration.exe 127.0.0.1 8443 localhost build\nt4-validation\client\ca.der build\nt4-validation\client\client.der build\nt4-validation\client\client.pk8 12 12 fixture/1
```

Use another free port and replace all three `12` values with `13` for TLS 1.3. The required gate is `WRITE=25 READ=25 CONTENT_MATCH=1`; the server must report authenticated `fixture/1` and `RECV=25 SEND=25 CONTENT_MATCH=True`.

## Bounded stress integration

Phase 7.H keeps the expensive soak runner outside the regular quick suite. Build it with tools\build-vc6.bat stress-integration. The resulting build\vc6\test_stress_stability.exe exposes bounded modes documented in stress-stability.md. Soak artifacts belong under build\phase7h and the normal NSS runtime-only PATH rule remains mandatory.
## Outputs and target identity

The current Makefile writes the Win32 x86 VC6 legacy-compatible target under `build\vc6`. This housekeeping task does not restructure existing outputs. Future architectures, toolchains, configurations, or modern NSS backends must use distinct output directories and must not mix artifacts into `build\vc6`.

## Known failures

- Missing VC6: set `PST_VC6_ROOT`; do not search the whole disk or switch compilers.
- `LNK1104` for `LIBC.lib`: use the bootstrap; it supplies the two explicit `/LIBPATH:` switches.
- `NSS_DIST is required`: set `PST_NSS_DIST` to the RetroZilla `dist` directory.
- NSS DLL load failure: restrict the test process `PATH` to `PST_NSS_RUNTIME` plus Windows system directories.

If the documented bootstrap fails, report the exact missing file/path before broader environment discovery.