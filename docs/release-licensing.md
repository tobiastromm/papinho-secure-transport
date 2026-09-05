# Release licensing audit

This is a technical inventory, not legal advice.

## Blocking project decision

PST has no root `LICENSE`, no operative SPDX identifier, and no package license declaration. Public headers do not grant a project-wide license. Public redistribution of a PST source or binary release therefore remains **BLOCKED ON EXPLICIT PST LICENSE APPROVAL AND APPLICATION**.

The owner's candidate, MPL-2.0, has completed a technical compatibility audit with the result **confirmed with conditions**. The recommended form is the standard unmodified MPL 2.0 without Exhibit B, preserving its Secondary Licenses mechanism. The audit found it compatible with the static-only NSS, Schannel, and OpenSSL SDK model, provided file-level source, notice, third-party separation, ownership, and corresponding-source conditions are met. See `docs/mpl-2.0-license-audit.md`.

This audit is not authorization to apply the candidate. After explicit owner approval, add the authoritative root license, establish the project-owned file notice policy, update package metadata and notices, and execute the audit's source/package gates. Third-party files retain their existing terms and must not be relicensed as PST code.

## Third-party classification

| Component | Evidence | Package treatment | Status before public release |
|---|---|---|---|
| RetroZilla/NSS 3.42 Beta/NSPR 4.7.7 lineage | `third_party/retrozilla-nss/licenses/`, `PROVENANCE.md`, source/binary manifests and ordered patch | legacy SDK carries preserved license evidence and runtime binaries | REVIEW REQUIRED |
| OpenSSL 3.5.8 LTS | Apache-2.0 text, source/prebuilt manifests and provenance | OpenSSL SDK carries license text, import libraries and two runtime DLLs | VERIFIED evidence; final combined-distribution review required |
| Windows Schannel/CryptoAPI/Winsock/CNG and CRT/UCRT | supplied by Windows or Microsoft redistributable/toolchain | not copied by PST staging | NOT REDISTRIBUTED |

RetroZilla/NSS/NSPR files contain historical MPL and tri-license notices. Preserving texts is necessary but is not itself a conclusion about every source or modification obligation. The fail-closed RNG patch and source/build records remain linked from provenance.

Modern `/MD` consumers require the appropriate Microsoft Visual C++ runtime. PST staging does not copy Microsoft installers or DLLs; deployment must follow Microsoft's applicable terms. Windows system DLLs must not be repackaged as PST files.
