# Repository and source-package hygiene

Status: Phase 9.E-H complete. This is the normative separation between repository history, the source package, and target binary SDKs. It does not apply MPL-2.0, promote version 0.4.0, or start Phase 9.F.

## Principles

The Git repository preserves source, tests, tooling, public and normative documentation, provenance, and useful engineering history. The source package is a curated corresponding-source deliverable, not a repository dump. A binary SDK is a target-specific consumer artifact. Packaging policy, rather than `.gitignore`, excludes versioned internal history.

`docs/codex/` is intentionally versioned and excluded from source packages. It holds historical evidence that remains useful to engineering but is not current public documentation. Provenance, patches, source snapshots, license evidence, and current contracts must never be moved there merely because they were produced during a phase.

## Canonical inclusion matrix

| Category | Git | src.zip | binary SDK | Observation |
|---|:---:|:---:|:---:|---|
| public headers | yes | yes | yes | authoritative consumer surface |
| core source | yes | yes | no | corresponding PST source |
| provider source | yes | yes | no | corresponding PST source |
| tests | yes | yes | selected/no | source audit and reproduction; not shipped by default in SDKs |
| examples | yes | yes | yes | SDK receives public examples |
| build scripts/Makefiles | yes | yes | no | required to rebuild source |
| packaging scripts | yes | yes | no | release reproduction |
| public docs | yes | yes | selected | SDK receives consumption/security/release subset |
| technical normative docs | yes | yes | selected | source package preserves current contracts |
| `docs/codex/` | yes | no | no | internal history only |
| third-party binaries | yes when canonical input | yes | target-specific | exact runtime/build provenance |
| third-party corresponding source | yes | yes | source reference only | source snapshots and manifests are mandatory |
| patches | yes | yes | source reference only | includes the NSS fail-closed patch |
| provenance | yes | yes | selected/reference | must remain adjacent to component |
| license evidence | yes | yes | applicable subset | each component retains its own terms |
| build outputs | no | no | only deliberately staged final library | recreated from source |
| `dist/staging/` | no | no | n/a | local reproducible staging output |
| logs/dumps/caches | no | no | no | local transient evidence |
| IDE files | no | no | no | local metadata |

## Documentation classification

Every current file under `docs/` was reviewed. The classifications below are about its present role, not its author or filename.

### PUBLIC_USER

- `consumer-linking.md`, `glossary.md`, `providers.md`, `security-and-limitations.md`
- `en/README.md`, `en/getting-started.md`, `pt-BR/README.md`, `pt-BR/getting-started.md`

### PUBLIC_CONTRIBUTOR

- `build-modern-msvc.md`, `build-openssl.md`, `build-retrozilla-nss.md`, `build-vc6.md`
- `provider-contributions.md`

### TECHNICAL_NORMATIVE

- architecture and backend/runtime: `architecture.md`, `backend-nss.md`, `credentials-trust-peer.md`, `legacy-platform-validation.md`, `nt4-client-package.md`, `schannel-backend.md`, `tls-runtime.md`
- providers: `cross-backend-validation.md`, `openssl-extension-closure.md`, `openssl-failure-hardening.md`, `openssl-provider.md`, `openssl-system-trust.md`, `provider-evolution.md`, `provider-spi.md`
- contracts/hardening: `connection-failure-matrix.md`, `diagnostic-api-design.md`, `error-diagnostics.md`, `interoperability-matrix.md`, `lifecycle-ownership.md`, `logging-design.md`, `readiness-progress.md`, `security-disclosure.md`, `stress-stability.md`, `tls-policy-negative-matrix.md`
- release: `mpl-2.0-license-audit.md`, `phase9-release-scope.md`, `public-api-abi.md`, `public-provider-bootstrap.md`, `release-licensing.md`, `release-packaging.md`, `repository-and-source-package-hygiene.md`, `roadmap.md`
- provenance: `legacy-nss-provenance.md`, `retrozilla-nss-pstw-audit.md`

Some normative documents retain historical evidence because it directly defines tested limits, close/readiness semantics, provider behavior, or provenance. That evidence is useful to consumers and contributors and is not merely a closure record.

### INTERNAL_HISTORY

- `codex/design-history/runtime-api-design.md`: provisional Phase 0.C design superseded by the implemented public ABI/runtime documents.
- `codex/design-history/backend-spi.md`: historical SPI 2.1/2.3 contract superseded by normative `provider-spi.md` at SPI 2.4.
- `codex/phase-history/phase3-functional-proof.md`: dated one-time functional proof retained as engineering evidence.
- `codex/phase-history/phase7-closure.md`: completed phase closure snapshot.
- `codex/phase-history/phase8-closure.md`: completed phase closure snapshot.

### REDUNDANT_OR_OBSOLETE and UNCERTAIN

No document was safe to delete, and no file remained unclassifiable. Superseded documents with historical value were moved rather than removed.

## Repository-root and tree audit

The root contains only repository policy, Makefiles, public notices/readme, and one ignored build object found during the audit. `test_foundation.obj` was a recreatable untracked artifact and was removed. No tracked logs, dumps, IDE databases, object files, rejected patches, backups, or temporary files were found.

`include/`, `src/`, `tests/`, `examples/`, `tools/`, `packaging/`, and `third_party/` contain intentional source, fixtures, build/release tooling, or canonical dependencies. Test strings containing paths/secrets are deliberate redaction fixtures, not credentials. `build/` and `dist/` are large recreated local output trees and remain ignored.

The global `*.zip` ignore rule incorrectly hid the exact RetroZilla corresponding-source snapshot. `.gitignore` now exempts only `third_party/*/source/*.zip`; ordinary release ZIPs remain ignored. `docs/codex/`, provenance, patches, source snapshots, licenses, manifests, packaging scripts, and release policy remain versionable.

## Source-package policy

`tools/stage-release-source.ps1` implements an allowlist. It stages the root README, notices and four Makefiles, then the explicit trees `include/`, `src/`, `tests/`, `examples/`, `tools/`, `packaging/`, and `third_party/`. It selects all current `docs/` files except `docs/codex/`.

The dry-run always excludes `.git/`, `build/`, nested `dist/staging/`, IDE metadata, logs, and temporary outputs. It verifies required public headers, PST source, build scripts, notices, OpenSSL source/provenance, and the exact RetroZilla snapshot, patch, and provenance. The absent root `LICENSE` is recorded as the expected pre-application gate; the script will include it automatically after the approved licensing step.

The source package deliberately includes canonical third-party binaries alongside source, manifests, patches and provenance because they are inputs to the supported/reproducible builds. This is distinct from blindly copying build outputs.

## Corresponding source gates

The RetroZilla source archive must remain:

- revision `2f274574d3c6ee8769914046920d649bbae9f81b`;
- SHA-256 `5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388`;
- paired with `0001-win32-secure-rng-fail-closed-nt4.patch`, provenance, manifests, and license evidence.

OpenSSL 3.5.8 source archive, manifests, provenance, license, headers, import libraries and runtime remain identifiable under `third_party/openssl/`. The source package preserves them; each binary SDK receives only its applicable runtime/build files and license material.

## Binary SDK policy

The official candidates are three provider-specific SDKs plus one optional Combined SDK:

- `windows-nt4-x86-vc6-retrozilla-nss`;
- `windows-x64-msvc-schannel`;
- `windows-x64-msvc-openssl-3.5.8`;
- `windows-x64-msvc-schannel-openssl-3.5.8` — official optional candidate, never implied to be default or recommended.

The Combined SDK includes the single validated combined PST static library, OpenSSL import/runtime files and licenses, and declares provider order `schannel,openssl`. It preserves exact/ordered/automatic selection semantics and never performs post-failure fallback.

## Generated files and checks

Staging creates `VERSION`, `manifest.ini`, `consumer-link.ini`, `SHA256SUMS.txt`, and the source-package status record. These generated files belong only to `dist/staging/` and are not versioned. Final ZIP creation, signing, MPL application, and version 0.4.0 promotion remain Phase 9.E closure work.

Markdown links must resolve relative to the moved document's new directory. Historical path text may remain only when explicitly discussing the old context. Absolute paths are permitted in build/provenance instructions where they identify deliberate local tool overrides; they are forbidden as hidden runtime/package dependencies. Obvious-secret scanning must distinguish deliberate negative-test fixtures from actual credentials.