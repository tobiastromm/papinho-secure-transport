# RetroZilla NSS/NSPR legacy provider

This directory preserves the exact historical source, local security patch, generated compile SDK, canonical Win32 x86 VC6 runtime, hashes, and original notices for the `retrozilla-nss` backend.

- Revision: `2f274574d3c6ee8769914046920d649bbae9f81b`
- NSS: 3.42 Beta
- NSPR: 4.7.7
- Legacy target: Win32 x86 / `i586-pc-msvc`, built with VC6; validated on Windows NT 4.0 SP6
- Exact source: `source/` (post-patch snapshot; verify its manifest)
- Patch order: `patches/MANIFEST.sha256`
- Build instructions: `../../docs/build-retrozilla-nss.md`
- Generated headers: `prebuilt/win32-x86-vc6/sdk/`
- Canonical binaries: `prebuilt/win32-x86-vc6/runtime/`
- Runtime hashes: `prebuilt/win32-x86-vc6/MANIFEST.sha256`
- Licenses/notices: `licenses/`
- Full provenance: `PROVENANCE.md`

Runtime files are `ssl3`, `nss3`, `nssutil3`, `softokn3`, `freebl3`, `nssdbm3`, `nspr4`, `plc4`, and `plds4`, including the three integrity `.chk` files. S/MIME and SQLite DLLs are intentionally outside this provider package.

Known limitation: the legacy provider permits one active NSS backend state per process. This is provider-local; the generic PST core supports concurrent different backends.
