# RetroZilla NSS/NSPR provenance

## Canonical identity

PapinhoSecureTransport uses the RetroZilla source lineage at revision `2f274574d3c6ee8769914046920d649bbae9f81b`. The preserved source identifies NSS 3.42 Beta in `security/nss/lib/nss/nss.h` and NSPR 4.7.7 in `nsprpub/pr/include/prinit.h`. The working extraction had no `.git` directory, so the revision is the established investigation/source-extraction record; it is not claimed to be recoverable from embedded Git metadata.

The canonical exact source artifact is `source/retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip`, SHA-256 `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388`. It is a snapshot of the exact post-patch RetroZilla source tree formerly at the historical `C:\PSTW\pr\projects\RetroZilla\RetroZilla`; it excludes the external object tree, MozillaBuild installation, logs, caches and Git metadata. Its embedded `mozconfig` records the configuration used.

Upstream project: RetroZilla (`https://github.com/rn10950/RetroZilla`). Revision identity is exact; the relationship to the NSS/NSPR labels above is verified from the preserved headers. No Internet substitute was used.

## Local patchset

`LOCAL PATCHSET EXISTS`. Apply order and hashes are in `patches/MANIFEST.sha256`. Patch 0001 makes Win32 secure RNG failure fail closed while retaining SystemFunction036 and CryptoAPI sources. It is required by PST's security basis and NT4 validation, not by VC6 syntax, TLS 1.3, or the PST adapter interface. The canonical ZIP is already post-patch: do not apply 0001 to it again. The patch exists to audit the delta against the recorded upstream revision. The resulting `security/nss/lib/freebl/win_rand.c` SHA-256 is recorded in the patch manifest.

## Source-to-binary chain

`revision + ordered patchset + embedded mozconfig + documented VC6/MozillaBuild environment = prebuilt/win32-x86-vc6/runtime`.

The canonical runtime is immutable for this housekeeping and every file is covered by `prebuilt/win32-x86-vc6/MANIFEST.sha256`. The generated header SDK used by the PST compilation is under `prebuilt/win32-x86-vc6/sdk`; its manifest distinguishes it from source and runtime. Object files, import libraries, PDBs, caches and the full external `dist/lib` tree are deliberately not preserved: PST resolves NSS/NSPR dynamically and only needs these generated headers plus the canonical runtime.

Reproducibility is LEVEL B: exact source, local delta, configuration, toolchain identity and build sequence are preserved, and the canonical binaries remain functionally validated. A fresh third-party rebuild was not attempted in this housekeeping, so byte identity is `NOT TESTED`, not claimed. Historical compiler/linker timestamps may prevent identical bytes.

## Runtime component map

- `ssl3.dll`: NSS TLS implementation.
- `nss3.dll`, `nssutil3.dll`: NSS core and utilities.
- `softokn3.dll` plus `softokn3.chk`: NSS software PKCS #11 module and integrity record.
- `freebl3.dll` plus `freebl3.chk`: NSS cryptographic primitives and integrity record.
- `nssdbm3.dll` plus `nssdbm3.chk`: NSS legacy DBM database module and integrity record.
- `nspr4.dll`: NSPR runtime.
- `plc4.dll`, `plds4.dll`: NSPR portable library utilities/data structures.

PST has functionally exercised TLS 1.2/TLS 1.3, mTLS, ALPN, incremental readiness, secure I/O and close classification with this runtime. The runtime exports relied upon include NSS/NSPR initialization, `PR_ImportTCPSocket`, `PR_Poll`, `SSL_ImportFD`, `SSL_ForceHandshake`, secure I/O, peer/cipher queries and `SSL_AlertReceivedCallback`; the last is the provider-local close-notify observation used by Phase 7.B.

## Licensing and redistribution material

Original top-level RetroZilla `LICENSE` and `LEGAL` are preserved as `licenses/RetroZilla-LICENSE.txt` and `licenses/RetroZilla-LEGAL.txt`. `licenses/NSS-MPL-2.0.txt` and `licenses/NSPR-license-evidence.h` preserve the NSS/NSPR evidence already audited. The exact source snapshot retains original per-file headers and notices. This is an inventory, not a legal conclusion.

## Canonical locations and disposability

- exact source: `source/`
- ordered patches: `patches/`
- build procedure: `../../docs/build-retrozilla-nss.md`
- original notices: `licenses/`
- generated compile SDK: `prebuilt/win32-x86-vc6/sdk/`
- canonical runtime and hashes: `prebuilt/win32-x86-vc6/runtime/` and adjacent manifest

Active PST build and runtime resolution use only those repository paths. `C:\PSTW` is a historical workspace and may be deleted after this repository change is safely retained. `REQUIRED_UNIQUE_FILES_IN_PSTW=0`; no deletion was performed.
