# C:\PSTW disposability audit

Audit scope: the historical RetroZilla workspace only. No file was deleted.

| Area | Classification | Preservation decision |
|---|---|---|
| `pr/projects/RetroZilla/RetroZilla` | B: exact source/provenance | Preserved once as the canonical post-patch ZIP under `third_party/retrozilla-nss/source`; `.git` was absent. One embedded compiler-debug directory was excluded. |
| source `mozconfig` and generated `.mozconfig.mk` evidence | B: build configuration | `mozconfig` remains inside the snapshot; exact effective options/tool identities are normalized in `docs/build-retrozilla-nss.md`. |
| `obj-rzSuite-release/dist/include/nspr` and `dist/public/nss` | B/generated required headers | Copied byte-identically to the repository SDK; `SDK_CONTENT_MISMATCHES=0`, 191 files, 1,869,380 bytes. |
| canonical DLL/CHK subset | A: already preserved identically | Repository runtime manifest verifies all 12 files; no runtime file changed. |
| ordered fail-closed RNG delta | A: already preserved | Patch retained; patch/result hashes and application semantics added to its manifest. |
| original RetroZilla `LICENSE` and `LEGAL` | B: notices | Copied verbatim to `third_party/retrozilla-nss/licenses`. Per-file notices remain in the source snapshot. |
| `obj-rzSuite-release` objects, libraries, generated applications and packaging output | C/D | Disposable. PST dynamically loads the canonical DLLs and does not link this output. |
| historical build logs | C/evidence distilled | Required flags/tool/config evidence is documented; raw logs are not build inputs and were not imported. |
| `mb` MozillaBuild installation | C/external tool installation | Not project source. Required tool families/versions/path model are documented; external licensed tool installers are not vendored. |
| unrelated archives, fixtures, credentials, caches and stale experiments | E | Not imported. No private key or credential database was copied. |
| unknown required material | F | None after source, SDK, notices, manifests and build procedure checks. |

Size audit before import: complete extracted RetroZilla tree 28,370 files / 264,560,715 bytes; NSS/coreconf/NSPR/DBM subset about 63.1 MB; canonical runtime 12 files / 2,431,625 bytes; patch 2,216 bytes. Repository impact: one cleaned source ZIP 91,486,321 bytes, generated header SDK 1,869,380 bytes, plus small manifests/notices/docs. No redundant loose source tree was added.

Active path audit: `tools/vc6-env.bat` now defaults `NSS_DIST` to the repository SDK. The successful `/W4` command line used only repository include paths. Remaining `C:\PSTW` mentions are historical statements or explicit claims that it is no longer required; no active build/test command resolves it.

Final result: `REQUIRED_UNIQUE_FILES_IN_PSTW=0`.
