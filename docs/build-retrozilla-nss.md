<!-- SPDX-License-Identifier: MPL-2.0 -->

# Rebuilding the historical RetroZilla NSS/NSPR runtime

This is a third-party reproduction procedure, separate from the normal PST build. It reconstructs the environment formerly staged under `C:\PSTW`; that path is not required.

## Inputs

1. Verify `third_party/retrozilla-nss/source/MANIFEST.sha256` and extract the single ZIP to a short, writable path such as `C:\rzbuild\RetroZilla`.
2. Do not reapply patch 0001: the archive is the exact post-patch tree. Use `patches/MANIFEST.sha256` to audit the upstream delta and resulting `win_rand.c`.
3. Use Microsoft Visual C++ 6.0 (`_MSC_VER=1200`), Visual Studio 6 SP5 plus Processor Pack. The compiler and linker are VC6; `win32-x86-vc6` does not merely mean that PST consumed the DLLs with VC6.
4. Use the historical MozillaBuild-compatible MSYS shell recorded by `.mozconfig.mk`: GNU `/bin/make.exe`, POSIX shell/configure tools, Perl and Python 2.5-era tooling, plus `moztools-180compat`. These tools are external build dependencies, not redistributed by PST. No separate modern Windows SDK is part of the recorded build.

## Configuration

The archive's `mozconfig` is authoritative:

```text
mk_add_options MOZ_MAKE_FLAGS="-j4"
mk_add_options MOZ_OBJDIR=/c/rzbuild/obj-rzSuite-release
ac_add_options --target=i586-pc-msvc
ac_add_options --enable-application=suite
ac_add_options --enable-optimize
ac_add_options --disable-debug
ac_add_options --disable-tests
ac_add_options --without-system-jpg
ac_add_options --without-system-zlib
ac_add_options --enable-extensions=default,tasks
ac_add_options --enable-crypto
ac_add_options --enable-svg
ac_add_options --enable-canvas
```

Only the object-directory prefix may be changed to the chosen short path. Historical evidence used `/c/projects/RetroZilla/obj-rzSuite-release`, target `i586-pc-msvc`, `/MD`, optimized non-debug compilation, and VC6 `link.exe`.

## Build sequence

From the MozillaBuild-compatible shell with the VC98 compiler/bin/include/lib directories initialized:

```sh
cd /c/rzbuild/RetroZilla
export MOZCONFIG=/c/rzbuild/RetroZilla/mozconfig
/bin/make.exe -f client.mk configure
/bin/make.exe -j4 -C /c/rzbuild/obj-rzSuite-release
```

The generated NSPR configuration must retain `--target=i586-pc-msvc --with-mozilla --disable-debug --enable-optimize` and use the same object `dist` prefix. Confirm `dist/include/nspr/prio.h`, `dist/public/nss/ssl.h`, and the runtime DLL set before comparison.

## Comparison and use

Hash rebuilt files with SHA-256 and compare names, exports, architecture, NSS/NSPR versions, and functional behavior with `prebuilt/win32-x86-vc6/MANIFEST.sha256`. Do not overwrite the canonical runtime automatically. A byte-identical rebuild has not been demonstrated; report `BYTE_IDENTICAL=0/1` only after an actual comparison. PST normal builds use the preserved generated SDK and do not invoke this third-party build.
