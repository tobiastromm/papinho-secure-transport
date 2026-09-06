# PapinhoSecureTransport 0.4.0 clean-machine x64 evidence

Classification: INTERNAL_HISTORY. This directory is excluded from the public source package by the canonical source stager.

Evidence payloads are preserved byte-for-byte in Git. `SHA256SUMS.txt` hashes those preserved bytes; line-ending conversion must not be applied within this release-evidence tree.

## Result

`CLEAN_MACHINE_X64=PASS` on a separate physical Windows 10 Pro 22H2 x64 machine, build 19045.6332, without a PST checkout. The complete gate matrix is in `final-reexecution/consolidated-gates.json`; the readable final report is `final-reexecution/consolidated-report.md`.

## Preserved evidence (KEEP)

- final report, consolidated matrix, final Cloudflare result/stdout/proof/command/request and unchanged-input hashes under `final-reexecution/`;
- sanitized OS/toolchain inventory in this directory plus the original non-personal OS and toolchain reports;
- official package hashes and post-test integrity results;
- compile commands, privacy-redacted compile logs, compile/dependency audit and `dumpbin` dependency records;
- loaded-module paths and OpenSSL DLL hashes;
- Schannel TLS 1.2, OpenSSL TLS 1.2/TLS 1.3 and Combined selection result JSONs plus controlled local server summaries;
- final stage matrix, SDK post-test integrity and runtime commands;
- shutdown classification, conclusion, comparison matrix, investigation results and source-contract excerpts under `shutdown-investigation/`.

## Files not preserved

- REDUNDANT: duplicate `.txt`/`.md` reports, previous consolidation, consumer logs already embedded in result JSONs, prior `online-gate-results.json`, and the `cloudflare-head-attempt`, `online-before-response-drain`, and `online-drain-attempt` trees. Their relevant shutdown history is consolidated in the preserved investigation.
- TEMPORARY: investigation `.exe`, `.obj`, `.map`, harness/trace source copies, preparation/summarization scripts, and empty stderr files. They are not release artifacts and their relevant hashes/behavior are recorded in preserved structured evidence.
- UNNECESSARY: copied fixture inventory/hashes, missing-fixture/bootstrap diagnostics, Python installation diagnostics, extraction inventories, and intermediate package-hash snapshots superseded by the final reports. Canonical fixtures and packages already exist elsewhere.
- PRIVACY-REDUCED: full machine inventories and compile logs contained an unnecessary Windows profile name. Their OS/toolchain facts, commands, inputs, warnings, dependencies and conclusions are preserved in sanitized, redacted, or structured files without that name.

No original file in `C:\evidence` was deleted during ingestion.
