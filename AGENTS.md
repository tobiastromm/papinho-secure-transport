<!-- SPDX-License-Identifier: MPL-2.0 -->

# Repository instructions

## ARCHITECTURAL GOVERNANCE — READ THIS FIRST

Before planning or modifying this repository, determine the **active branch/ref/working tree** actually being worked on.

For project-local architectural context, the active working context is authoritative. Do **not** substitute the repository default branch for the active branch/ref when auditing ADRs, Project Context, capabilities, plans, code, tests, build scripts, or implementation state.

If the correct branch/ref/working tree cannot be determined or accessed, report that limitation before concluding that an ADR, document, capability, implementation, or build path does not exist.

### Governance loading order

Load context in this order before implementation:

1. **OrganizationEngineering** accepted ADRs applicable to this project.
   - Canonical repository: `tobiastromm/organization-engineering`
   - Start with the accepted ADRs under `docs/adr/`.
   - Organization-wide ADR governance and Project Context rules are authoritative where applicable.

2. **PapinhoEngineering** accepted ADRs applicable to the `papinho-ecosystem` Product Family.
   - Canonical repository: `tobiastromm/papinho-engineering`
   - Do not assume every PapinhoEngineering ADR is organization-wide; apply it according to its current scope and supersession state.

3. **Project Context** for this repository.
   - Read `PROJECT-CONTEXT.md`.
   - Use it to resolve Project Relationship, Product Family, Technology Profiles, Shared Frameworks, and project/component applicability.

4. **PapinhoSecureTransport local accepted ADRs**.
   - Canonical local path: `docs/adr/` in the active working context.
   - Local ADRs govern PST-specific decisions.

5. **Capability Documents and live architecture/documentation**, where applicable.
   - Read the documents relevant to the task rather than assuming current implementation from an old checkpoint.

6. **Current phase / plan / roadmap / release evidence** relevant to the task.

7. **Code / Tests / Build / scripts**.
   - Audit the implementation and tests before claiming factual implementation state.
   - Code/Tests are evidence of what is actually implemented; documentation may drift.

### Conflict handling

If applicable governance sources disagree, report:

```text
GOVERNANCE CONFLICT
```

before making an architectural change that depends on the conflict.

Do not silently choose a lower-level document over an applicable higher-level ADR, and do not silently reinterpret an accepted ADR to match current code.

### ADR governance

- Do not autonomously modify an `accepted` ADR.
- Changes to an accepted ADR, including scope changes, supersession, or metadata migration, require explicit owner authorization or another governance process that expressly permits the change.
- New ADRs must use the canonical OrganizationEngineering ADR governance/template or a conforming local derivative.
- ADR numbering is local to this repository.
- Inter-repository ADR references should prefer qualified identities such as `OrganizationEngineering/ADR-0002`.
- Do not delete superseded/rejected/deprecated ADRs merely because they are no longer current.
- A durable architectural decision found only in README/checkpoint/plan documentation should be reported as an **ADR candidate** rather than silently treated as canonical.

### Project Context and documentation roles

`PROJECT-CONTEXT.md` identifies which governance contexts apply to PST. It does not replace ADRs, API/SPI docs, capabilities, build docs, or implementation evidence.

Keep these roles distinct:

```text
ADR
→ durable decision and rationale

Project Context
→ applicable governance dimensions

Capability / live documentation
→ current intended behavior, status, limitations, pending work

Code / Tests
→ factual implementation evidence
```

When implementation changes a documented capability or current architecture, update the corresponding live documentation after the code/tests pass.

### Required pre-implementation summary

Before substantial implementation, establish at least:

```text
active branch/ref/working tree
applicable higher-level ADRs
applicable local ADRs
relevant live docs / plan
factual code/test state
known conflicts or drift
planned change and validation
```

Do not rediscover or redesign architecture already fixed by accepted ADRs unless the task explicitly asks to revisit that decision.

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
