<!-- SPDX-License-Identifier: MPL-2.0 -->

# SS-10 publication preflight

Status: release candidate recertified; publication not performed.

## Canonical packaging policy

The integrated-master reproduction defect was confirmed as checkout EOL
normalization only. There were zero non-EOL package-entry differences and no
binary-byte changes. Canonical staging now reads every tracked package input
from its exact `HEAD` Git blob. Untracked build artifacts and other opaque
binary inputs are copied byte-for-byte. Generated metadata and manifests are
UTF-8 without BOM with LF line endings. Package generation refuses a dirty
tracked index or worktree.

The five ZIPs and package checksum manifest reproduced byte-identically from:

- integrated `master` with `core.autocrlf=true`;
- `feature/server-side`;
- a fresh detached worktree with `core.autocrlf=false`.

## Frozen 0.5.0 candidate hashes

| Artifact | SHA-256 |
| --- | --- |
| `papinho-secure-transport-0.5.0-src.zip` | `815ec2e492e3e35895f674517d3b869242bc553d1e82aaf8fd2df0ca38905bbc` |
| `papinho-secure-transport-0.5.0-win32-x86-vc6-retrozilla-nss.zip` | `9a7e56648bd04a816c80428a05c636707d118c7ed9f31d6238aeec2e39f038c5` |
| `papinho-secure-transport-0.5.0-win32-x64-msvc-19.51-schannel.zip` | `9acd633f348b9bb186066fbb45f0e994aa42be8e9e270813f7bacb8391c85c25` |
| `papinho-secure-transport-0.5.0-win32-x64-msvc-19.51-openssl3.zip` | `5464d8bd0617f01c5bc30b7d36b784c2348580057f469b8928a598f22e8db62b` |
| `papinho-secure-transport-0.5.0-win32-x64-msvc-19.51-schannel-openssl3.zip` | `8006251c7c411a016a7c7097d9c5bb41f8d3206b71bf4c5155bbf0b514a17b07` |
| `SHA256SUMS-packages.txt` | `68e69bace3696be99c1fea248300dfc02a067bd69a07bcc652313afe54f465a9` |

## Recertification gates

- package structure, extraction and internal SHA-256: PASS;
- licensing, provenance, source and corresponding archive: PASS;
- extracted SDK consumers: 8/8 PASS;
- Combined public selection: PASS;
- SS-8 production `.lib`, `.dll` and `.chk` identity: 22/22 PASS;
- clean-machine, Combined real-TLS and NT4 real-TLS reruns: not required
  because production bytes did not change.

No push to `master`, tag, GitHub Release, asset upload or Accelerator handoff
was performed by this preflight.
