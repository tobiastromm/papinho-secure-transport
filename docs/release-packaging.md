<!-- SPDX-License-Identifier: MPL-2.0 -->

# Release packaging decision

Status: Phase 9.E complete. Standard MPL-2.0 without Exhibit B is applied; the 0.4.0 source package and four static binary SDK candidates are staged locally. Publication and clean-machine validation remain later gates.

## Distribution model

The first release should ship a source archive and four separate, release-only binary SDKs built as static libraries. A source-repository archive is not a substitute for the binary SDKs.

| Target ID | Provider | Architecture/toolchain | CRT | Package status |
|---|---|---|---|---|
| `windows-nt4-x86-vc6-retrozilla-nss` | RetroZilla NSS | x86, VC6, NT4 compatibility floor | compiler-default static CRT | candidate |
| `windows-x64-msvc-schannel` | Schannel | x64, documented MSVC/Windows SDK | `/MD` | candidate |
| `windows-x64-msvc-openssl-3.5.8` | OpenSSL | x64, documented MSVC, OpenSSL 3.5.8 | `/MD` | candidate |
| `windows-x64-msvc-schannel-openssl-3.5.8` | Schannel then OpenSSL | x64 combined target | `/MD` | official optional candidate |

Static-only is the prudent first-release choice: it matches every validated build, minimizes deployment machinery on NT4, and avoids claiming an untested DLL ABI/loader/CRT boundary. Consumers must relink to update PST and supply the documented link/runtime dependencies. DLL-only or static+DLL would add import-library generation, export validation, loader-security policy, NT4 testing, cross-module CRT review, and a doubled release matrix. A future explicit DLL subphase may revisit it; `PST_BUILD_DLL` and `PST_USE_DLL` are preparatory macros, not evidence of a supported DLL today.

Combined is now an official optional binary SDK candidate. It is neither the default nor recommended universally: its larger deployment and OpenSSL runtime obligation remain explicit. Provider order is Schannel then OpenSSL, and there is no post-selection fallback after TLS, authentication, or I/O failure.

## SDK tree

`tools/stage-release-sdk.ps1` recreates the four technical binary candidates under `dist/staging/0.4.0/<target-id>`:

```text
README.md
VERSION
manifest.ini
consumer-link.ini
SHA256SUMS.txt
THIRD_PARTY_NOTICES.md
include/
lib/<target-id>/
runtime/<target-id>/
licenses/
docs/
examples/
```

Staging is not a public release. It includes PST LICENSE and records license_id=MPL-2.0 plus the exact 0.4.0 source-package reference. It contains no objects, private headers, test fixtures, keys, caches, dumps, or provider source. Target IDs avoid time-relative words.

`tools/stage-release-source.ps1` separately creates an allowlisted source-package dry-run, excluding `docs/codex/`, build/staging outputs and repository metadata while preserving tests, tooling and corresponding third-party source. See `repository-and-source-package-hygiene.md`.

The eventual archive name should be `papinho-secure-transport-<package-version>-<target-id>.zip`. ZIP creation and signing are deferred. SHA-256 detects accidental change; it does not establish publisher authenticity. Code signing remains future/review-required.

## Version decision

Package and Library are 0.4.0. API remains 1.3.0 and SPI remains 2.4. The compatible library promotion records the accumulated provider, bootstrap, documentation, packaging, provenance and licensing work without changing API/SPI.

## Consumer placement

An application may vendor an extracted target SDK under `third_party/pst`, or reference an immutable external extraction through a project-defined path. It must not copy arbitrary PST source or depend on checkout paths. Runtime DLLs should be deployed explicitly beside the consuming executable; do not place them in Windows system directories or rely on a global arbitrary PATH.

Clean-machine runtime execution is not performed in 9.E. Isolated compile/link from staged headers and libraries is the structural proof; final clean-machine execution belongs to Phase 9.F.
