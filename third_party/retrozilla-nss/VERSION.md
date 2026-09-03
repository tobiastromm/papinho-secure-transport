# Version record

- Upstream: RetroZilla
- Source commit (investigation record; ZIP has no .git metadata): 2f274574d3c6ee8769914046920d649bbae9f81b
- NSS source: 3.42 Beta (security/nss/lib/nss/nss.h)
- NSPR source: 4.7.7 (nsprpub/pr/include/prinit.h)
- Patch: secure Windows RNG fail-closed, preserving SystemFunction036 and Win32 CryptoAPI
- Compiler: VC6 / _MSC_VER=1200, VS6 SP5 and Processor Pack
- Target: Win32 x86 / i586-pc-msvc
- Validated OS: Windows NT 4.0 SP6

- Exact post-patch source snapshot: source/retrozilla-2f274574d3c6ee8769914046920d649bbae9f81b-patched.zip
- Source snapshot SHA-256: 5371ce6fb2fd0df909faaed4cf92dc9c112844e1d1bedd7a8dc7f598b900d388
- Reproducibility classification: Level B; byte-identical rebuild not tested
