# PST 0.6.5 RetroZilla NSS close-notify candidate

This directory is isolated qualification evidence. It is not a normal PST SDK
and must not replace any governed package without a separate owner decision.

- PST source commit: `da7190930700f54ecc92611f5edbcb0932caa5ec`
- Historical compiler: Microsoft Visual C++ 6.0 SP6, `cl 12.00.8804`
- Historical linker: `6.00.8447`
- Processor Pack target: `i586`, Windows NT 4.0-compatible headers
- Mozilla environment: MozillaBuild 1.2-compatible MSYS/make tools
- NSS CRT: `/MD` (the PST consumer library remains separately tested as `/ML` and `/MD`)
- Candidate `ssl3.dll`: `68F20EDE3C71CBAA132EF9E1DA9F54CCF8FA5D96DA761E29892070511FED0FCE`
- Canonical patch: `EE6A127D06E59351CEFDB63CE8B85312FF99CC773AC39910739ECAE1C097E65F`

The baseline reproduction has identical i386 architecture, PE/subsystem 4.0,
107 exports, imports, section layout and CRT dependencies. Its remaining bytes
differ only in embedded source-path capitalization and PE/COFF timestamps.

Real TLS 1.2 and 1.3 mTLS tests pass with both PST `/ML` and `/MD`. The focal
TLS 1.3 backpressure test observes three consecutive factual `NEED_WRITE`
shutdown operations with WRITE interest, then drains 49,152 bytes and completes
reciprocal close-notify after readiness. A separate pending-session destruction
test does not report a false shutdown completion.

The existing fail-closed RNG patch is already represented in governed source
as `0001-win32-secure-rng-fail-closed-nt4.patch`; it changes `win_rand.c` and
`freebl3.dll`, not `ssl3.dll`.
